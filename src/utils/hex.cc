#include <cstdio>
#include "hex.h"


bool load_hex(const std::string &a_filename, CPU_DATA &cpu) {
	// :BBAAAATT[DDDDDDDD]CC
	// BB = length
	// AAAA = Address
	// TT=0: data;  TT=01: EOF; TT=02: linear address; TT=04: extended address
	// DD = data
	// CC = checksum (2's complement of length+address+data)
	FILE *f = fopen(a_filename.c_str(), "r");
	char buf[256];

	cpu.eeprom.clear();
	cpu.flash.clear();
	while (fgets(buf, sizeof(buf), f)) {
		if (buf[0] != ':')
			throw(std::string("Invalid file format. ") +a_filename+" is not a standard .hex file.");
		int bb, aaaa, tt, cc;
		char *p;
		bb = strtoul(std::string(buf, 1, 2).c_str(), &p, 16);
		aaaa = strtoul(std::string(buf, 3, 4).c_str(), &p, 16);
		tt = strtoul(std::string(buf, 7, 2).c_str(), &p, 16);
		cc = strtoul(std::string(buf, 9+bb*2, 2).c_str(), &p, 16);

		WORD sum = 0;
		for (int n=0; n<4; ++n) {
			BYTE d = strtoul(std::string(buf, 1+n*2, 2).c_str(), &p, 16);
			sum += d;
		}

		char ds[bb+1]; ds[bb] = 0;
		for (int n = 0; n < bb; ++n) {
			BYTE d = strtoul(std::string(buf, 9+n*2, 2).c_str(), &p, 16);
			sum += d;
			ds[n] = d;
		}
		sum = (~sum+1) & 0xff;
		if (sum != cc)
			throw(std::string("Checksum failure while loading HEX file"));

//			std::cout <<std::dec<<"bb="<<bb<<"; aaaa="<<std::hex<<aaaa<<"; tt="<<tt<<"; cc="<<cc<<"["<<(WORD)sum<< "]; data="<<std::string(buf+9,bb*2) << "\n";
		if (tt==0) {
			if (aaaa == 0x400e)                      // config word
				cpu.configure(std::string(ds, bb));
			else if (aaaa >= 0x4200)                 // eeprom
				cpu.eeprom.set_data(aaaa - 0x4200, std::string(ds, bb));
			else                                     // flash data
				cpu.flash.set_data(aaaa, std::string(ds, bb));
		} else if (tt==1) {                 // eof
			break;
		}
	}
	fclose(f);
	return true;
}


void write_hex_records(FILE *f, int bb, WORD aaaa, const char *data, int dlen) {
	char buf[256];

	for (int ofs=0; ofs<dlen; ofs += bb) {
		WORD cc = 0, tt = 0;
		if (ofs+bb > dlen) bb=dlen-ofs;

		snprintf(buf, sizeof(buf), ":%02X%04X%02X", bb, aaaa, tt);
		cc += bb; cc += aaaa >> 8; cc += aaaa & 0xff; cc += tt;
		for (int n=0; n<bb; ++n) {
			BYTE d=data[ofs+n]; cc += d;
			snprintf(buf+9+2*n, 4, "%02X", d);
		}
		aaaa += bb;
		cc = (~cc + 1) & 0xff;
		snprintf(buf+9+bb*2, 4, "%02X\n", cc);
		size_t written=fwrite(buf, 9+bb*2+3, 1, f);
		if (written < 0)
			throw(std::string("Cannot write to hex file"));
	}
}


bool dump_hex(const std::string &a_filename, CPU_DATA &cpu) {
	FILE *f = fopen(a_filename.c_str(), "w+");

	char *flash = (char *)cpu.flash.data;
	int limit = FLASH_SIZE*sizeof(WORD);
	while (limit>0 && flash[limit-1]==0) --limit;

	write_hex_records(f, 0x10, 0, flash, limit);

	char *eeprom = (char *)cpu.eeprom.data;
	limit = EEPROM_SIZE;
	while (limit>0 && eeprom[limit-1]==0) --limit;

	auto config = cpu.configuration();
	write_hex_records(f, 0x10, 0x4200, eeprom, limit);
	write_hex_records(f, 0x10, 0x400E, config.c_str(), config.length());

	std::string eof(":00000001FF\n");
	fwrite(eof.c_str(), eof.length(), 1, f);

	fclose(f);
	return true;
}
