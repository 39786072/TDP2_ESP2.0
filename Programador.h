#ifndef PROGRAMADOR_H
  #define PROGRMADOR_H
  #define DEBUG 1
  #define RST_Slave 5
  #define ACTUALPOS (pagePos + pageIndex * pageSize)
  #define PAGEENDS ((pageIndex + 1) *pageSize)
  class Line {
    private:
      uint8_t len;
      uint16_t address;
      uint8_t type;
      uint8_t * data;
      uint8_t checksum;
    public:
       Line(String); // constructor 
      ~Line();      // destructor
      bool isEOF();
      bool verify();
      uint16_t getAddress();
      uint8_t getLenght();
      uint8_t getData(uint8_t); 
  };
  
  class Page {
    private:
      uint8_t*  page;
      uint8_t  pageIndex;
      uint16_t pageSize;
      uint16_t pagePos;
    public:
        Page(int);
        ~Page();
        bool isComplete();
        bool program();
        bool addLine(Line);
  };
  typedef enum{
  Code,  NotUsed,  ProgramMode,  ParallelMode,  PollingMode,  SelfTimed, LockBytes, FuseBytes, FlashPolVal1, FlashPolVal2, EEMPROMPolVal1, EEPROMPolVal2, PageSizeHigh, PageSizeLow, EEPROMSizeHigh, EEPROMSizeLow, FlashSize4, FlashSize3, FlashSize2, FlashSize1
  } DeviceField;
const uint8_t device[20] = {
  0x86,//1 Device Code
  0x00,//2 Not Used
  0x00,//3 Program Mode
  0x01,//4 Parallel Mode
  0x01,//5 Polling Mode
  0x01,//6 SelfTimed
  0x01,//7 LockBytes
  0x03,//8 FuseBytes
  0xFF,//9 FlashPollVal1
  0xFF,//10 FlashPollVal2
  0xFF,//11 EEPROMPollVal1
  0xFF,//12 EEPROMPollVal2
  0x00,//13 PageSizeHigh
  0x80,//14 PageSizeLow
  0x04,//15 EEPROMSizeHigh
  0x00,//16 EEPROMSizeLow
  0x00,//17 FlashSize4
  0x00,//18 FlashSize3
  0x80,//19 FlashSize2
  0x00,//20 FlashSize1
  };
const uint8_t device_ext[5] = {
  0x05, //1 Command Length
  0x04, //EEPROM Page Size
  0xD7, //Signal Pagel
  0xC2, //Signal BS2
  0x00  //No especificado
};


unsigned char strtobyte(char value);
bool WaitFor(uint8_t c, uint16_t timeout);
bool InitProg ();
#endif
