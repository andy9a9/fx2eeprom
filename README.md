## Read and write the eeprom of an FX2 chips

### Compiling
```
- Install min req VS_2008 or Microsoft .NET Framework 3.5(msbuild) with Windows SDK 7.1(vcbuild)
- go to msvc directory and run:
 - build_fxload.bat for fxload build
 - build_pc.bat for read/write eeprom utility
- Output artefacts will be available in Win32 directory
```

### Using
1. Connect device and install WinUSB driver eg. via (http://zadig.akeo.ie/) utility
2. load fx2 FW into device
`fxload.exe -t fx2 -d 0x04b5:0x4032 -i Vend_Ax.hex`
3. Run read/write utility
  * read 256B from address 0x00 `eerd.exe eeprom.raw 0x00 0x100`
  * write 256B from file `eewr.exe eeprom.raw 0x100`

**Current version of Vend_Ax.hex firmware could be used for read/write 512kB of Hantek 4032L FPGA over SPI FLASH.**
