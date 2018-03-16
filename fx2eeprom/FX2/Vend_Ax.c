#pragma NOIV									// Do not generate interrupt vectors
//-----------------------------------------------------------------------------
//	  File:		   periph.c
//	  Contents:	   Hooks required to implement USB peripheral function.
//
//	  Copyright (c) 2000 Cypress Semiconductor All rights reserved
//-----------------------------------------------------------------------------
#include "fx2.h"
#include "Fx2regs.h"
#include "eeprom.h"

extern BOOL	 GotSUD; // Received setup data flag
extern BOOL	 Sleep;
extern BOOL	 Rwuen;
extern BOOL	 Selfpwr;

BYTE	Configuration; // Current configuration
BYTE	AlternateSetting; // Alternate settings

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
#define VR_UPLOAD			0xc0
#define VR_DOWNLOAD			0x40

#define VR_EEPROM			0xa2 // loads (uploads) EEPROM

#define VR_GET_EEP_ID		0xa5 

#define VR_EEP_ERASE		0xa9

#define EP0BUFF_SIZE		0x40

//-----------------------------------------------------------------------------
// Task Dispatcher hooks
//	  The following hooks are called by the task dispatcher.
//-----------------------------------------------------------------------------

void TD_Init(void) // Called once at startup
{
	IOC = 0x11; // nWP=1, nCS=1, rest are 0
	OEC = 0x37;

	IOE = 0x60;
	OEE = 0xE0;

	Rwuen = TRUE; // Enable remote-wakeup
}

void TD_Poll(void) // Called repeatedly while the device is idle
{
}

BOOL TD_Suspend(void) // Called before the device goes into suspend mode
{
	return(TRUE);
}

BOOL TD_Resume(void) // Called after the device resumes
{
	return(TRUE);
}

//-----------------------------------------------------------------------------
// Device Request hooks
//	  The following hooks are called by the end point 0 device request parser.
//-----------------------------------------------------------------------------


BOOL DR_GetDescriptor(void)
{
	return(TRUE);
}

BOOL DR_SetConfiguration(void) // Called when a Set Configuration command is received
{
	Configuration = SETUPDAT[2];
	return(TRUE); // Handled by user code
}

BOOL DR_GetConfiguration(void) // Called when a Get Configuration command is received
{
	EP0BUF[0] = Configuration;
	EP0BCH = 0;
	EP0BCL = 1;
	return(TRUE); // Handled by user code
}

BOOL DR_SetInterface(void) // Called when a Set Interface command is received
{
	AlternateSetting = SETUPDAT[2];
	return(TRUE); // Handled by user code
}

BOOL DR_GetInterface(void) // Called when a Set Interface command is received
{
	EP0BUF[0] = AlternateSetting;
	EP0BCH = 0;
	EP0BCL = 1;
	return(TRUE); // Handled by user code
}

BOOL DR_GetStatus(void)
{
	return(TRUE);
}

BOOL DR_ClearFeature(void)
{
	return(TRUE);
}

BOOL DR_SetFeature(void)
{
	return(TRUE);
}

BOOL DR_VendorCmnd(void)
{
	WORD	addr, len, bc;
	BYTE	addr_h; // for spi eeprom
	WORD	ChipRev;
	WORD 	i;


	switch(SETUPDAT[1])
	{
		case VR_GET_EEP_ID:
			while(EP0CS & bmEPBUSY);
			EEPROMReadId((WORD)EP0BUF);
			EP0BCH = 0;
			EP0BCL = 3;
			break;

		case VR_EEP_ERASE: // We need a separate function for this. Erasing at write beginning will destroy previous data (we can't write full 512KB in a single EP0 transfer)
			*EP0BUF = EEPROMEraseChip();
			EP0BCH = 0;
			EP0BCL = 1; // Arm endpoint with # bytes to transfer
			EP0CS |= bmHSNAK; // Acknowledge handshake phase of device request
			break;

		case VR_EEPROM:
			addr = SETUPDAT[2]; // Get address and length
			addr |= SETUPDAT[3] << 8;
			addr_h = SETUPDAT[4];
			len = SETUPDAT[6];
			len |= SETUPDAT[7] << 8;
			// Is this an upload command ?
			if(SETUPDAT[0] == VR_UPLOAD)
			{
				while(len) // Move requested data through EP0IN
				{ // one packet at a time.

					while(EP0CS & bmEPBUSY);

					if(len < EP0BUFF_SIZE)
						bc = len;
					else
						bc = EP0BUFF_SIZE;

					EEPROMRead(addr_h, addr, (WORD)bc, (WORD)EP0BUF);

					EP0BCH = 0;
					EP0BCL = (BYTE)bc; // Arm endpoint with # bytes to transfer

					addr += bc;
					len -= bc;

				}
			}
			// Is this a download command ?
			else if(SETUPDAT[0] == VR_DOWNLOAD)
			{
				if (len!=EEPROM_PAGE_SIZE)
					return (TRUE); //error. We write full pages only
				while(len)
				{
					EP0BCH = 0;
					EP0BCL = 0; // Clear bytecount to allow new data in; also stops NAKing

					while(EP0CS & bmEPBUSY); //wait for new data

					bc = EP0BCL; // Get the new bytecount

					// write data to EEPROM
					EEPROMWrite(addr_h, addr, bc, (WORD)EP0BUF);

					addr += bc;
					len -= bc;
				}
			}
			break;
		}
		return(FALSE); // no error; command handled OK
}

//-----------------------------------------------------------------------------
// USB Interrupt Handlers
//	  The following functions are called by the USB interrupt jump table.
//-----------------------------------------------------------------------------

// Setup Data Available Interrupt Handler
void ISR_Sudav(void) interrupt 0
{
	// enable the automatic length feature of the Setup Data Autopointer
	// in case a previous transfer disbaled it
	SUDPTRCTL |= bmSDPAUTO;

	GotSUD = TRUE; // Set flag
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUDAV; // Clear SUDAV IRQ
}

// Setup Token Interrupt Handler
void ISR_Sutok(void) interrupt 0
{
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUTOK; // Clear SUTOK IRQ
}

void ISR_Sof(void) interrupt 0
{
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSOF; // Clear SOF IRQ
}

void ISR_Ures(void) interrupt 0
{
	if (EZUSB_HIGHSPEED())
	{
		pConfigDscr = pHighSpeedConfigDscr;
		pOtherConfigDscr = pFullSpeedConfigDscr;
	}
	else
	{
		pConfigDscr = pFullSpeedConfigDscr;
		pOtherConfigDscr = pHighSpeedConfigDscr;
	}

	EZUSB_IRQ_CLEAR();
	USBIRQ = bmURES; // Clear URES IRQ
}

void ISR_Susp(void) interrupt 0
{
	Sleep = TRUE;
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUSP;
}

void ISR_Highspeed(void) interrupt 0
{
	if (EZUSB_HIGHSPEED())
	{
		pConfigDscr = pHighSpeedConfigDscr;
		pOtherConfigDscr = pFullSpeedConfigDscr;
	}
	else
	{
		pConfigDscr = pFullSpeedConfigDscr;
		pOtherConfigDscr = pHighSpeedConfigDscr;
	}

	EZUSB_IRQ_CLEAR();
	USBIRQ = bmHSGRANT;
}

void ISR_Ep0ack(void) interrupt 0
{
}
void ISR_Stub(void) interrupt 0
{
}
void ISR_Ep0in(void) interrupt 0
{
}
void ISR_Ep0out(void) interrupt 0
{
}
void ISR_Ep1in(void) interrupt 0
{
}
void ISR_Ep1out(void) interrupt 0
{
}
void ISR_Ep2inout(void) interrupt 0
{
}
void ISR_Ep4inout(void) interrupt 0
{
}
void ISR_Ep6inout(void) interrupt 0
{
}
void ISR_Ep8inout(void) interrupt 0
{
}
void ISR_Ibn(void) interrupt 0
{
}
void ISR_Ep0pingnak(void) interrupt 0
{
}
void ISR_Ep1pingnak(void) interrupt 0
{
}
void ISR_Ep2pingnak(void) interrupt 0
{
}
void ISR_Ep4pingnak(void) interrupt 0
{
}
void ISR_Ep6pingnak(void) interrupt 0
{
}
void ISR_Ep8pingnak(void) interrupt 0
{
}
void ISR_Errorlimit(void) interrupt 0
{
}
void ISR_Ep2piderror(void) interrupt 0
{
}
void ISR_Ep4piderror(void) interrupt 0
{
}
void ISR_Ep6piderror(void) interrupt 0
{
}
void ISR_Ep8piderror(void) interrupt 0
{
}
void ISR_Ep2pflag(void) interrupt 0
{
}
void ISR_Ep4pflag(void) interrupt 0
{
}
void ISR_Ep6pflag(void) interrupt 0
{
}
void ISR_Ep8pflag(void) interrupt 0
{
}
void ISR_Ep2eflag(void) interrupt 0
{
}
void ISR_Ep4eflag(void) interrupt 0
{
}
void ISR_Ep6eflag(void) interrupt 0
{
}
void ISR_Ep8eflag(void) interrupt 0
{
}
void ISR_Ep2fflag(void) interrupt 0
{
}
void ISR_Ep4fflag(void) interrupt 0
{
}
void ISR_Ep6fflag(void) interrupt 0
{
}
void ISR_Ep8fflag(void) interrupt 0
{
}
void ISR_GpifComplete(void) interrupt 0
{
}
void ISR_GpifWaveform(void) interrupt 0
{
}
