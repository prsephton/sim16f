#include <cstdio>
#include <cstdlib>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <queue>
#include <sys/types.h>
#include <fcntl.h>

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
				std::cout << "PC: " << PC << ":\t";
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
				std::cout << current->disasm(opcode, data) << "\t\t W:" << int_to_hex(data.W) << "\tSP:" << data.SP << "\n";
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

	void configure(const std::string &data) {

	}

	bool load_hex(const std::string &a_filename) {
		// :BBAAAATT[DDDDDDDD]CC
		// BB = length
		// AAAA = Address
		// TT=0: data;  TT=01: EOF; TT=02: linear address; TT=04: extended address
		// DD = data
		// CC = checksum (2's complement of length+address+data)
		FILE *f = fopen(a_filename.c_str(), "r");
		char buf[128];

		while (fgets(buf, sizeof(buf), f)) {
			if (buf[0] != ':')
				throw(std::string("Invalid file format. ") +a_filename+" is not a standard .hex file.");
			int bb, aaaa, tt, cc;
			char *p;
			bb = strtoul(std::string(buf, 1, 2).c_str(), &p, 16);
			aaaa = strtoul(std::string(buf, 3, 4).c_str(), &p, 16);
			tt = strtoul(std::string(buf, 7, 2).c_str(), &p, 16);
			cc = strtoul(std::string(buf, 9+bb*2, 2).c_str(), &p, 16);

//			BYTE crc = 0;
			char ds[bb+1]; ds[bb] = 0;
			for (int n = 0; n < bb; ++n) {
				BYTE d = strtoul(std::string(buf, 9+n*2, 2).c_str(), &p, 16);
				ds[n] = d;
			}

			std::cout <<std::dec<<"bb="<<bb<<"; aaaa="<<std::hex<<aaaa<<"; tt="<<tt<<"; cc="<<cc<<"; data="<<std::string(buf+9,bb*2) << "\n";
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
		return false;
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
	unsigned long clock_speed_hz = 8;
//	unsigned long clock_speed_hz = 20000000;
	unsigned long delay_us = 1000000 / clock_speed_hz;
	std::cout << "delay is: " << delay_us << "\n";
	pthread_t machine;
	pthread_create(&machine, NULL, run_machine, &cpu);

	cpu.load_hex("test.hex");
//	cpu.tests();

	try {
		while (cpu.running()) {
			usleep(delay_us);
			cpu.toggle_clock();
		}
	} catch(const std::string &message) {
		std::cout << "Exiting: " << message << "\n";
	}
	pthread_exit(NULL);
}
