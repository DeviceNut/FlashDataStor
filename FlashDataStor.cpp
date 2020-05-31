#if defined(SPARK)
#include "application.h"
#else
#include <Arduino.h>
#include <EEPROM.h>
#endif
#include "FlashDataStor.h"

#define DEBUG_OUTPUT 0 // 1 to debug this file
#if DEBUG_OUTPUT
#define DBG(x) x
#define DBGOUT(x) MsgFormat x
#else
#define DBG(x)
#define DBGOUT(x)
#endif

FlashDataStor flashDataStor; // only one instance

#if defined(ESP8266)
#define EEPROM_LENGTH   4096
#else
#define EEPROM_LENGTH   EEPROM.length()
#endif

#if DEBUG_OUTPUT
static char outstr[101];
static void MsgFormat(const char *fmtstr, ...)
{
  va_list va;
  va_start(va, fmtstr);
  vsnprintf(outstr, 100, fmtstr, va);
  va_end(va);

  Serial.println(outstr);
}

static void DumpData(int len)
{
  DBGOUT(("Contents: (len=%d)", len));
  for (int i = 0; i < len; ++i)
  {
    char ch = EEPROM.read(i);
    DBGOUT(("%3d: (%c) %d", i, ((32<=ch && ch<128) ? ch : 32), ch));
  }
}
#endif // DEBUG_OUTPUT

// fill out internal product information header
// returns FlashDataStor_Status value (>0 if not in valid format)
FlashDataStor_Status FlashDataStor::getHeaderInfo(void)
{
  lenStorage = EEPROM_LENGTH;
  if (lenStorage < FLASHDATASTOR_MINLEN) return FlashDataStor_BadStorLength;

  int defsize = sizeof(FlashDataStor::HeaderInfo);

  int addr = offsetof(HeaderInfo, headerLen);
  lenHeader = EEPROM.read(addr++);
  if (lenHeader != defsize)
  {
    DBGOUT(("HeadLen: %d(actual) != %d(defined)", lenHeader, headlen));
    DBG( DumpData(lenStorage) );
    return FlashDataStor_BadHeaderLen;
  }

  addr = 0;
  byte *pdata = (byte*)&headInfo;
  for (int i = 0; i < lenHeader; ++i, ++pdata)
    *pdata = EEPROM.read(addr++);

  DBGOUT(("Lengths: Total=%d Header=%d SavedBytes=%d", lenStorage, lenHeader, headInfo.savedBytes));
  DBGOUT(("Strings: Len=%d Count=%d", headInfo.lenStrings, headInfo.numStrings));

  if (headInfo.savedBytes != (lenStorage - lenHeader - (headInfo.numStrings * headInfo.lenStrings)))
  {
    DBGOUT(("Data corrupted: lengths don't match"));
    DBG( DumpData(lenStorage) );
    return FlashDataStor_CorruptData;
  }

  EEPROM.write(offsetof(HeaderInfo, bootCount), headInfo.bootCount+1); // increment boot count

  #if defined(ESP8266)
  EEPROM.commit();
  #endif

  return FlashDataStor_Success;
}

bool FlashDataStor::setSavedValue(int offset, int length, byte *pvalue)
{
  if (offset > headInfo.savedBytes-length)
    return false;

  int i = 0;
  int addr = lenHeader + offset;
  while (length--) EEPROM.write(addr++, *(pvalue + i++));

  #if defined(ESP8266)
  EEPROM.commit();
  #endif

  return true;
}

bool FlashDataStor::getSavedValue(int offset, int length, byte *pvalue)
{
  if (offset > headInfo.savedBytes-length)
    return false;

  int i = 0;
  int addr = lenHeader + offset;
  while (length--) *(pvalue + i++) = EEPROM.read(addr++);

  return true;
}

bool FlashDataStor::setString(int index, char *pstr)
{
  if (index >= headInfo.numStrings)
    return false;

  bool done = false;
  int addr = lenHeader + headInfo.savedBytes + (index * headInfo.lenStrings);
  for (int i = 0; i < headInfo.lenStrings; ++i, ++addr)
  {
    if (!done)
    {
      EEPROM.write(addr, *pstr);
      if (*pstr++ == 0) done = true;
    }
    else EEPROM.write(addr, 0);
  }

  #if defined(ESP8266)
  EEPROM.commit();
  #endif

  return true;
}

bool FlashDataStor::getString(int index, char *pstr)
{
  if (index >= headInfo.numStrings)
    return false;

  int addr = lenHeader + headInfo.savedBytes + (index * headInfo.lenStrings);
  for (int i = 0; i < headInfo.lenStrings-1; ++i, ++addr, ++pstr)
  {
    *pstr = EEPROM.read(addr);
    if (*pstr == 0) break;
  }

  *pstr = 0; // insure string termination
  return true;
}
