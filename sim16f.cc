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
#include "hex.h"
#include "assembler.h"

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

	void cycle() {
		try {
			execute();
			fetch();
		} catch (std::string &error) {
			std::cout << "Terminating because: " << error << "\n";
			active=false;
		}
	}

	void toggle_clock() { data.clock.toggle(data.pins); }

	bool running() const { return active; }

	bool load_hex(const std::string &a_filename) { return ::load_hex(a_filename, data); };
	bool dump_hex(const std::string &a_filename) { return ::dump_hex(a_filename, data); };

	bool assemble(const std::string &a_filename) { return ::assemble(a_filename, data, instructions); }
	void disassemble(const std::string &a_filename) { ::disassemble(a_filename, data, instructions); }
	void disassemble() { ::disassemble(data, instructions); }


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


//	cpu.load_hex("test.hex");
//	cpu.dump_hex("dumped.hex");
//	cpu.disassemble("dumped.hex");
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
