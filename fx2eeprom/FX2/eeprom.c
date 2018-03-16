//-----------------------------------------------------------------------------
//   File:      eeprom.c
//   Contents:   EEPROM update firmware source.  (Write only)
//
//   indent 3.  NO TABS!
//
//   Copyright (c) 2002 Cypress Semiconductor
//
// $Workfile: eeprom.c $
// $Date: 9/07/05 2:54p $
// $Revision: 1 $
//-----------------------------------------------------------------------------
#include "fx2.h"
#include "fx2regs.h"
#include "eeprom.h"
#include "Ports.h"

#define EEPROM_PP 0x02 // page programm
#define EEPROM_RB 0x03 // read BYTE
#define EEPROM_WD 0x04 // write disable
#define EEPROM_RS 0x05 // read status register
#define EEPROM_WE 0x06 // write enable
#define EEPROM_CE 0x60 // chip erase
#define EEPROM_ID 0x9F // eeprom ID

///////////////////////////////////////////////////////////////////////////////////////

bit EEPROMSendCmd(BYTE cmd);
static bit EEPROMWaitForComplete();

BYTE SpiTrans(BYTE vo)
{
	BYTE i, vi;
	
	for(i=0; i<8; i++)
	{
		vi <<= 1;
		vi |= IO_DOUT;
		IO_DIN = (vo & 0x80) ? 1 : 0;
		IO_SCK = 1;
		vo <<= 1;
		IO_SCK = 0;
	}

	return vi;
}

bit EEPROMSendCmd(BYTE cmd)
{
	BYTE i;

	IO_nCS = 0;

	i = SpiTrans(cmd); // Write enable

	IO_nCS = 1;

	// return i; needs to be tested
	return 0;
}

void EEPROMReadId(BYTE xdata *buf)
{
	IO_nCS = 0;

	SpiTrans(EEPROM_ID); // READ ID
	buf[0] = SpiTrans(0);
	buf[1] = SpiTrans(0);
	buf[2] = SpiTrans(0);

	IO_nCS = 1;
}

// Returns 0 on success, 1 on failure
bit EEPROMRead(BYTE addr_h, WORD addr, BYTE length, BYTE xdata *buf)
{
	BYTE i;

	IO_nCS = 0;

	SpiTrans(EEPROM_RB); // READ BYTES
	SpiTrans(addr_h);
	SpiTrans(addr>>8);
	SpiTrans(addr);

	for (i=0; i < length; i++)
	{
		*(buf+i) = SpiTrans(0);
	}

	IO_nCS = 1;

	return 0;
}

// Returns 0 on success, 1 on failure
bit EEPROMWrite(BYTE addr_h, WORD addr, BYTE length, BYTE xdata *buf)
{
	BYTE i;

	// write enable
	if (EEPROMSendCmd(EEPROM_WE))
	{
		return 1;
	}

	IO_nCS = 0;

	SpiTrans(EEPROM_PP); // PP
	SpiTrans(addr_h);
	SpiTrans(addr>>8);
	SpiTrans(addr);

	for(i=0; i<length; i++)
	{
		SpiTrans(*(buf+i));
	}

	IO_nCS = 1;

	if (EEPROMWaitForComplete()) //tPPmax=7ms
	{
		return 1;
	}

	return 0;
}

static bit EEPROMWaitForComplete()
{
	BYTE status, cmd = EEPROM_RS; //RDSR

	IO_nCS = 0;

	SpiTrans(cmd); // read status register

	do //TODO: add timeout
	{
		// read status
		status=SpiTrans(cmd);
		if (status == 0xFF)
		{
			break;
		}
	} while(status&0x01); //WIP

	status=SpiTrans(cmd); // read status register

	IO_nCS = 1;

	if (status&0x01)
	{
		return 1;
	}

	return 0;
}

bit EEPROMEraseChip()
{
	// write enable
	if (EEPROMSendCmd(EEPROM_WE))
	{
		return 1;
	}

	// erase whole chip
	if (EEPROMSendCmd(EEPROM_CE))
	{
		return 1;
	}

	// wait for complete tCEmax=25000ms
	if (EEPROMWaitForComplete())
	{
		return 1;
	}

	return 0;
}
