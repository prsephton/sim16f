#pragma once

typedef unsigned short WORD;
typedef unsigned char BYTE;

struct device_config {
	short flash_size;
	short eeprom_size;
	short ram_banks;
	short max_memory;
};

//auto PIC16f627A = device_config{1024, 128, 2, 2*80+48};
//auto PIC16f628A = device_config{2048, 128, 2, 2*80+48};
//auto PIC16f648A = device_config{4096, 256, 4, 3*80};

const short STACK_SIZE = 8;
const short PIN_COUNT = 18;       // pins on the chip
const short BANK_SIZE = 0x80;     // size of one memory bank

const short FLASH_SIZE = 1024;    //
const short EEPROM_SIZE = 128;    //
const short RAM_BANKS = 2;        // 1 for 16f627, 2 for 16f628 and 4 for 16f648
const short MAX_MEMORY = 2*80+48; // for 16f648, it is 3x80

