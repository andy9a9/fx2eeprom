#ifndef EEPROM_H_INCLUDED
#define EEPROM_H_INCLUDED

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
#define EEPROM_PAGE_SIZE 256

bit EEPROMWrite(BYTE addr_h, WORD addr, BYTE length, BYTE xdata *buf);
bit EEPROMRead(BYTE addr_h, WORD addr, BYTE length, BYTE xdata *buf);
bit EEPROMEraseChip();
void EEPROMReadId(BYTE xdata *buf);

#endif
