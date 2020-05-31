#ifndef FLASH_DATASTOR_H
#define FLASH_DATASTOR_H

#define FLASHDATASTOR_MINLEN            128       // minimum acceptable length of EEPROM

enum FlashDataStor_Status
{
  FlashDataStor_Success=0,
  FlashDataStor_BadStorLength,    // length of EEPROM < FLASHDATASTOR_MINLEN bytes
  FlashDataStor_BadHeaderLen,     // header length as read is less than expected (length of ProductInfo structure)
  FlashDataStor_CorruptData,      // (headerLen + savedBytes + (lenStrings * numStrings)) != flashLen
  FlashDataStor_Count
};

class FlashDataStor
{
public:
  typedef struct // this is read-only data for the caller:
  {
    char name[4];                 // 4 ASCII chars
    byte headerLen;               // length of this structure (16)
    byte lenStrings;              // length of each string in table
    byte numStrings;              // number of string slots in the table
    byte VersionNum;              // string table/software version number
    uint16_t productID;           // product ID or serial number
    uint16_t flashLen;            // length of entire flash storage
    uint16_t savedBytes;          // number of bytes for saved values
    uint16_t bootCount;           // number of times getHeaderInfo() called

    // what follows this in EEPROM is the saved values, followed by the string table
  }
  HeaderInfo;
  HeaderInfo headInfo; // filled out after getHeaderInfo() is called

  // Fills in above header info by reading data from flash, and
  // performs value checks. Returns an error if the format is bad,
  // in which case it is assumed that no other calls get made.
  // If successful, the bootCount value is incremented.
  FlashDataStor_Status getHeaderInfo(void);

  // Saves/retrieves data values of a length determined by the caller.
  // Returns false if offset not in range: 0...numSavedBytes-length
  bool setSavedValue(int offset, int length, byte *pvalue);
  bool getSavedValue(int offset, int length, byte *pvalue);

  // Saves/retrieves strings to the string table, assuming a maximum
  // length of lenStrings (thus preventing overflow on the write, and
  // adding the string terminator on the read. Returns false if index
  // is not in the range of 0...numStrings-1.
  bool setString(int index, char *pstr);
  bool getString(int index, char *pstr);

private:
  int lenStorage;                 // size in bytes of entire EEPROM
  byte lenHeader;                 // size of actual header in bytes

};

extern FlashDataStor flashDataStor; // single statically allocated instance
#endif
