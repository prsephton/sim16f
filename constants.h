#ifndef __data_types_h__
#define __data_types_h__

typedef unsigned short WORD;
typedef unsigned char BYTE;

const short FLASH_SIZE = 1024;    //
const short EEPROM_SIZE = 128;    //
const short RAM_BANKS = 2;        // 1 for 16f627, 2 for 16f628 and 4 for 16f648
const short BANK_SIZE = 0x80;     // size of one memory bank
const short MAX_MEMORY = 2*80+48; // for 16f648, it is 3x80
const short STACK_SIZE = 8;
const short PIN_COUNT = 18;       // pins on the chip

#endif

