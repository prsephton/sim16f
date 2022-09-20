#pragma once

typedef unsigned short WORD;
typedef unsigned char BYTE;

struct Params {
	const char *name;
	short flash_size;
	short eeprom_size;
	short ram_banks;
	short bank_size;
	short pin_count;
	short stack_size;

};

