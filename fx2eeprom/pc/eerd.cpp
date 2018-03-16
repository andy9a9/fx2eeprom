#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
//#include "Util\Conutil.h"
#include "cyapi.h"

#define EEP_SIZE        0x80000

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

static bool ReadEepId(void *data)
{
  ep0->Target   = TGT_DEVICE;
  ep0->ReqType  = REQ_VENDOR;
  ep0->ReqCode  = 0xA5;
  ep0->Value    = 0;
  ep0->Index    = 0;
  LONG bytesToSend = 3;
  return ep0->Read((PUCHAR)data,  bytesToSend);
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

#define EEP_BLK_SIZE    0x100

bool ReadEeprom(const char *fname, unsigned _addr, unsigned size)
{
  unsigned char *buf = new unsigned char[size];
  for(unsigned addr = 0; addr<size; addr+=EEP_BLK_SIZE)
  {
    printf("%X\n", addr);
    if (!ReadEep(_addr+addr, buf+addr, EEP_BLK_SIZE))
    {
      printf("Eeprom read failed at %X !\n", _addr+addr);
      delete buf;
      return false;
    }
  }
  int hf = open(fname, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, S_IWRITE);
  write(hf, buf, size);
  close(hf);
  delete buf;
  return true;
}

void main(int argc, char *argv[])
{
  if (argc!=4)
  {
    printf("Usage: %s <file> <addr> <size>\n", argv[0]);
    return;
  }
  unsigned size=0, addr=0;
  if (sscanf(argv[2], "%x", &addr)!=1)
  {
    puts("Addr: invalid format !");
    return;
  }

  if (sscanf(argv[3], "%x", &size)!=1)
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
  unsigned char id[3];
  ReadEepId(id);
  printf("EEPROM ID: %02X %02X %02X\n", id[0], id[1], id[2]);
  ReadEeprom(argv[1], addr, size);
  UsbClose();
}
