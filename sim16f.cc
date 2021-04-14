#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <pthread.h>

#include "constants.h"
#include "devices.h"
#include "cpu_data.h"
#include "instructions.h"
#include "utility.h"


//___________________________________________________________________________________
// Models the 16fxxx CPU
class CPU {
	CPU_DATA data;
	InstructionSet instructions;
	SmartPtr<Instruction>current;
	WORD opcode;
	bool active;
	bool debug;
	bool skip;
	int  cycles;

	void fetch() {
		if (cycles > 0) {
			current = NULL;
		} else {
			WORD PC = data.sram.get_PC();
			if (skip)
				current = instructions.find(0);
			else {
				opcode = data.flash.fetch(PC);
				current = instructions.find(opcode);
			}
			if (current) cycles = current->cycles;
			if (debug) {
				std::cout << std::setfill('0') << std::hex << std::setw(4) <<  PC << "\t";
			}
			++PC; PC = PC % FLASH_SIZE;
			data.sram.set_PC(PC);
		}
	}

	void execute() {
		if (cycles) --cycles;
		if (current) {
			skip = current->execute(opcode, data);
			if (debug) {
				std::cout << current->disasm(opcode, data) << "\t W:" << int_to_hex(data.W) << "\tSP:" << data.SP << "\n";
			}
		}
	}

  public:
	void reset() {
		data.SP = 7;
		data.W = 0;
		cycles = 0;
		data.sram.set_PC(0);
		data.sram.write(data.sram.STATUS, 0);
	}


	void tests() {
		int n = 0;
		data.W = 0x7e;
		try {
			data.flash.data[n++] = instructions.assemble("ADDWF", 0x4a, 0, false);
			data.flash.data[n++] = instructions.assemble("ADDWF", 0x4a, 0, true);

			data.flash.data[n++] = instructions.assemble("CALL",  14, 0, false);

			data.flash.data[n++] = instructions.assemble("CLRF",  0x4a, 0, false);
			data.flash.data[n++] = instructions.assemble("ANDWF", 0x4a, 0, false);
			data.flash.data[n++] = instructions.assemble("ANDWF", 0x4a, 0, true);
			data.flash.data[n++] = instructions.assemble("BTFSC", 0x4a, 1, true);
			data.flash.data[n++] = instructions.assemble("CLRW", 0x05, 0, false);

			data.flash.data[n++] = instructions.assemble("COMF", 0x42, 0, true);
			data.flash.data[n++] = instructions.assemble("COMF", 0x05, 0, false);
			data.flash.data[n++] = instructions.assemble("DECFSZ", 0x05, 0, false);
			data.flash.data[n++] = instructions.assemble("CLRW", 0, 0, false);
			data.flash.data[n++] = instructions.assemble("DECF", 0x05, 0, false);
			data.flash.data[n++] = instructions.assemble("GOTO", 0, 0, false);

			data.flash.data[n++] = instructions.assemble("MOVLW", 0x22, 0, false);
			data.flash.data[n++] = instructions.assemble("RETURN", 0, 0, false);
		} catch (std::string &error) {
			std::cout << error << "\n";
			throw("Terminating");
		}
	}

	void cycle() {
		try {
			execute();
			fetch();
		} catch (std::string &error) {
			std::cout << "Terminating because: " << error << "\n";
			active=false;
		}
	}

	void toggle_clock() {
		data.clock.toggle(data.pins);
	}

	bool running() const {
		return active;
	}

	void configure(const std::string &config) {
		data.configuration = config;
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


	bool dump_hex(const std::string &a_filename) {
		FILE *f = fopen(a_filename.c_str(), "w+");

		char *flash = (char *)data.flash.data;
		int limit = FLASH_SIZE*sizeof(WORD);
		while (limit>0 && flash[limit-1]==0) --limit;

		write_hex_records(f, 0x10, 0, flash, limit);

		char *eeprom = (char *)data.eeprom.data;
		limit = EEPROM_SIZE;
		while (limit>0 && eeprom[limit-1]==0) --limit;

		write_hex_records(f, 0x10, 0x4200, eeprom, limit);

		write_hex_records(f, 0x10, 0x400E, data.configuration.c_str(), data.configuration.length());

		std::string eof(":00000001FF\n");
		fwrite(eof.c_str(), eof.length(), 1, f);

		fclose(f);
		return true;
	}


	bool translate(const char *in,
			std::string &label,  std::string &mnemonic, std::string &address, std::string &arg) {
		char *pt = (char *)in, *sep;

		while (*pt && *pt <= 32) pt++;             // white space skip
		if (*pt) {                                 // found something
			if (*pt==';') return false;            // just a comment line

		} else {
			return false;                          // blank line
		}

		if ((sep = strchr(pt, ':')) != NULL) {
			label = to_upper(std::string(pt, sep-pt)); pt = sep+1;
		}
		while (*pt && *pt <= 32) pt++;             // white space skip
		if (*pt) {
			if (*pt==';') return true;             // comment and label
		} else {
			return true;                           // just a label on a line.
		}
		for (sep=pt; *sep && *sep > 32; ++sep);    // look for next whitespace
		if (sep > pt) {                            // either ws or zero terminator
			mnemonic = std::string(pt, sep-pt);
			pt = sep+1;
			while (*pt && *pt <= 32) pt++;                 // white space skip
			if (*pt) {                                     // found something
				if (*pt == ';') return true;               // a comment
				if ((sep = strchr(pt, ','))!=NULL) {
					address = to_upper(std::string(pt, sep-pt)); pt = sep+1;
					if (*pt == ';') return false;              // a comment
					if (*pt == 0) return false;                // no arg
					for (sep=pt; *sep && *sep > 32; ++sep);    // look for next whitespace
					arg = to_upper(std::string(pt, sep-pt));
					return true;
				} else {  // no arg
					for (sep=pt; *sep && *sep > 32; ++sep);    // look for next whitespace
					if (sep > pt) {
						address = to_upper(std::string(pt, sep-pt));
						arg = "";
						return true;
					}
				}
			}
		}
		return false;
	}


	bool assemble(const std::string &a_filename) {   // assembly file read into flash
		// expected format:
		//     [label:] mnemonic [args] [; comments] \n
		//
		// <label> may be on the preceding line. and may be preceded by whitespace.
		// <label> may not be a number or hex number.  If it is, it will be ignored.
		// In an instruction, prefix a label with ':'; for example, GOTO :mylabel
		// <mnemonic> may be preceded by whitespace.
		// The W or Flag register destination argument is separated by a comma and
		// is the small letter 'w' or 'f'.  For example:  "ADDWF  0x33,f".
		// Instead of ,w or ,f, ,0 or ,1 are also accepted.
		// It is valid to use register names instead of values.  Eg. BTS TRISA,2
		// anything on a line after a semicolon is considered to be a comment
		// Case is not important. XORWF STATUS,w is equivalent to xorwf StaTus, W

		FILE *f = fopen(a_filename.c_str(), "r");
		char buf[256];
		WORD PC = 0;
		std::map<std::string, WORD> labels;

		for (int pass=0; pass<2; ++pass) {
			PC = 0;
			fseek(f, 0, SEEK_SET);
			if (pass)  // clear flash on second pass
				memset(data.flash.data, 0, sizeof(data.flash.data));
			while (fgets(buf, sizeof(buf), f)) {
				std::string label, mnemonic, address, arg;
				WORD waddr;
				BYTE warg=0;
				bool to_file=false;


				if (translate(buf, label, mnemonic, address, arg)) {
//					std::cout << "label: ["<<label<<"]; mnemonic: ["<<mnemonic<<"]; address: ["<<address<<"]; arg: ["<<arg<<"]\n";
					label = to_upper(label);
					mnemonic = to_upper(mnemonic);
					address = to_upper(address);
					arg = to_upper(arg);
					if (label.length()) {
						if (!(is_decimal(label) || is_hex(label))) {    // Not a real label
							if (labels.find(to_upper(label))==labels.end()) {
								labels[label] = PC;  // record current address as a label
							} else if (!pass) {
								throw(std::string("Format error: Non-unique label: ")+label);
							}
						}
					}
					if (mnemonic.length()) {
						if (address.length()) {
							if (address.substr(0,2) == "0X") { // a Hex address
								char *p;
								waddr = strtoul(address.c_str()+2, &p, 16);
								if (!p) throw(std::string("Invalid hex digits found: [")+address+"] @"+int_to_hex(PC));
							} else {   // a label or file register name
								if (address[0]==':') {
									waddr = 0;
									if (labels.find(label)==labels.end()) {  // should be seen in pass 2
										if (pass) throw(std::string("Unknown label referenced: ") + label + " @"+int_to_hex(PC));
									} else {
										waddr = labels[label];
									}
								} else {
									if (data.Registers.find(address)==data.Registers.end())
										throw(std::string("Unknown file register name [")+address+"] @"+int_to_hex(PC));
									waddr = data.Registers[address]->index();
								}
							}
							if (arg.length()) {
								if (arg=="1" || arg=="F") {
									warg = 1;
									to_file = true;
								} else if (arg=="0" || arg=="W") {
									warg = 0;
									to_file = false;
								} else {
									char *p;
									warg = strtoul(arg.c_str(), &p, 10);
									if (!p) throw(std::string("Invalid argument value: [")+arg+"] @"+int_to_hex(PC));
								}
							}
						}
						if (pass) { // write flash on second pass
							WORD opcode = instructions.assemble(mnemonic, waddr, warg, to_file);
							SmartPtr<Instruction> op = instructions.find(opcode);   // check
							if (!op || op->mnemonic != mnemonic) throw(std::string("Error while checking assembly: ") + mnemonic);
							data.flash.data[PC] = opcode;
						}
					} else {
						std::cout << "Problem decoding line: " << buf << "\n";
					}
				} else {
					throw(std::string("Cannot decode assembly line: ")+buf);
				}
				++PC;  // all instructions are single word
			}
		}
		active=false;
		return false;
	}

	void disassemble() {
		int limit = FLASH_SIZE;
		while (limit>0 && data.flash.data[limit-1]==0) --limit;
		for (int pc=0; pc < limit; ++pc) {
			WORD opcode = data.flash.data[pc];
			current = instructions.find(opcode);
			std::cout <<  std::setfill('0') << std::hex << std::setw(4) << pc << ":\t" << current->disasm(opcode, data) << "\n";
		}
		active=false;
	}

	void disassemble(const std::string &a_filename) {
		load_hex(a_filename);
		disassemble();
	}

	bool load_hex(const std::string &a_filename) {
		// :BBAAAATT[DDDDDDDD]CC
		// BB = length
		// AAAA = Address
		// TT=0: data;  TT=01: EOF; TT=02: linear address; TT=04: extended address
		// DD = data
		// CC = checksum (2's complement of length+address+data)
		FILE *f = fopen(a_filename.c_str(), "r");
		char buf[256];

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
					configure(std::string(ds, bb));
				else if (aaaa >= 0x4200)                 // eeprom
					data.eeprom.set_data(aaaa - 0x4200, std::string(ds, bb));
				else                                     // flash data
					data.flash.set_data(aaaa, std::string(ds, bb));
			} else if (tt==1) {                 // eof
				break;
			}
		}
		fclose(f);
		return true;
	}

	void process_queue() {
		while (!data.pins.events.empty()) {
			PINS::Event e = data.pins.events.front(); data.pins.events.pop();
			if (e.name == "cycle")
				cycle();
			else {
				throw(std::string("Unhandled pin event: ") + e.name);
				active = false;
			}
		}
	}

	CPU(): active(true), debug(true), skip(false), cycles(0) {
		memset(data.flash.data, 0, sizeof(data.flash.data));
		memset(data.eeprom.data, 0, sizeof(data.eeprom.data));
		reset();
	}
};

//___________________________________________________________________________________
// Runtime thread for the machine
void *run_machine(void *a_cpu) {
	CPU &cpu = *((CPU *)(a_cpu));
	try {
		while (cpu.running()) {
			usleep(5);
			try {
				cpu.process_queue();
			} catch (std::string &error) {
				std::cout << error << "\n";
			}
		}
	} catch(const char *message) {
		std::cout << "Machine Exit: " << message << "\n";
	} catch(const std::string &message) {
		std::cout << "Machine Exit: " << message << "\n";
	}

	pthread_exit(NULL);
}


//___________________________________________________________________________________
// Host thread.
int main(int argc, char *argv[]) {
	CPU cpu;
	unsigned long clock_speed_hz = 8;    // 2 cycles per second
	unsigned long delay_us = 1000000 / clock_speed_hz;
//	std::cout << "delay is: " << delay_us << "\n";
	pthread_t machine;
	pthread_create(&machine, NULL, run_machine, &cpu);

	cpu.load_hex("test.hex");
//	cpu.dump_hex("dumped.hex");
//	cpu.disassemble("dumped.hex");
//	cpu.tests();
	try {
		cpu.assemble("test.a");
	} catch (std::string &err) {
		std::cout << "error in assembly: " << err << "\n";
		return 0;
	}
	cpu.disassemble();

//	try {
//		while (cpu.running()) {
//			usleep(delay_us);
//			cpu.toggle_clock();
//		}
//	} catch(const std::string &message) {
//		std::cout << "Exiting: " << message << "\n";
//	}
	pthread_exit(NULL);
}
