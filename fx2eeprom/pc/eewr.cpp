#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
//#include "Util\Conutil.h"
#include "cyapi.h"

#define EEP_SIZE        0x80000 //MX25L4005 is 512K
#define EEP_BLK_SIZE    256

CCyUSBDevice *USBDevice;
CCyControlEndPoint *ep0 = 0;

bool UsbInit()
{
  USBDevice = new CCyUSBDevice(NULL);   // Create an instance of CCyUSBDevice
//  USBDevice->SetAltIntfc(1);
  ep0 = USBDevice->ControlEndPt;

  if (ep0==0)
  {
        printf("Can't get EP0 !\n");
        USBDevice->Close();
        return false;
  }
  return true;
}

void UsbClose()
{
  USBDevice->Close();
}

static bool WriteMem(unsigned short addr, const void *data, unsigned short len)
{
  ep0->Target   = TGT_DEVICE;
  ep0->ReqType  = REQ_VENDOR;
  ep0->ReqCode  = 0xA0;
  ep0->Value    = addr;
  ep0->Index    = 0;
  LONG bytesToSend =  len;
  return ep0->Write((PUCHAR)data,  bytesToSend);
}

static bool ReadEep(unsigned addr, void *data, unsigned short len)
{
  ep0->Target   = TGT_DEVICE;
  ep0->ReqType  = REQ_VENDOR;
  ep0->ReqCode  = 0xA2;
  ep0->Value    = addr;
  ep0->Index    = addr>>16;
  LONG bytesToSend =  len;
  return ep0->Read((PUCHAR)data,  bytesToSend);
}

static bool WriteEep(unsigned addr, const void *data, unsigned short len)
{
  ep0->Target   = TGT_DEVICE;
  ep0->ReqType  = REQ_VENDOR;
  ep0->ReqCode  = 0xA2;
  ep0->Value    = addr;
  ep0->Index    = addr>>16;
  LONG bytesToSend =  len;
  return ep0->Write((PUCHAR)data,  bytesToSend);
}

static bool EraseEep()
{
  ep0->Target   = TGT_DEVICE;
  ep0->ReqType  = REQ_VENDOR;
  ep0->ReqCode  = 0xA9;
  ep0->Value    = 0;
  ep0->Index    = 0;
  unsigned char res = 0xFF;
  LONG bytesToSend =  1;
  if (!ep0->Read((PUCHAR)&res,  bytesToSend))
  	return false;
  return res==0;
}

bool UploadFw(const char *fname)
{
        int hf = open(fname, O_RDONLY | O_BINARY);
        if (hf==-1)
                return false;
        unsigned char cmd = 1;
        if (!WriteMem(0xE600, &cmd, 1))
                return false;
        unsigned short len = filelength(hf);
        unsigned short addr = 0;
        while(len)
        {
                unsigned len_b = (len>0x40) ? 0x40 : len;
                unsigned char buf[0x40];
                read(hf, buf, len_b);
                if (!WriteMem(addr, buf, len_b))
                        return false;
                addr+=len_b;
                len-=len_b;
        }
        close(hf);
        cmd = 0;
        if (!WriteMem(0xE600, &cmd, 1))
                return false;
        return true;
}

bool WriteEeprom(const char *fname, unsigned size)
{
  unsigned char *buf = new unsigned char[size];
  int hf = open(fname, O_RDONLY | O_BINARY);
  if (read(hf, buf, size)!=size)
  {
  	puts("Invalid file size !");
  	close(hf);
  	return false;
  }
  close(hf);
  for(unsigned addr = 0; addr<size; addr+=EEP_BLK_SIZE)
  {
    printf("%X\n", addr);
    if (!WriteEep(addr, buf+addr, EEP_BLK_SIZE))
    {
      printf("Eeprom write failed at %X !\n", addr);
      delete buf;
      return false;
    }
  }
  delete buf;
  return true;
}

void main(int argc, char *argv[])
{
  if (argc!=3)
  {
    printf("Usage: %s <eep.bin> <size>\n", argv[0]);
    return;
  }
  unsigned size=0;
  if (sscanf(argv[2], "%x", &size)!=1)
  {
    puts("Size: invalid format !");
    return;
  }

  UsbInit();
  // upload FX2Fw
  if (!UploadFw("vend_ax.bix"))
  {
    puts("FX2 upload failed !");
    return;
  }

  // erase eeprom
  if (!EraseEep())
  {
    puts("EEPROM erasing failed !");
    return;
  }

  // write into eeprom
  WriteEeprom(argv[1], size);
  UsbClose();
}
