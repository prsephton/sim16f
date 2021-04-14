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
			std::cerr << "Terminating because: " << error << "\n";
			active=false;
		}
	}

	void toggle_clock() { data.clock.toggle(data.pins); }

	bool running() const { return active; }

	void configure(const std::string &a_filename) {};
	void load_eeprom(const std::string &a_filename) {};
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

	void run(unsigned long delay_us, bool a_debug=false) {
		debug = a_debug;
		try {
			while (running()) {
				usleep(delay_us);
				toggle_clock();
			}
		} catch(const std::string &message) {
			std::cerr << "Exiting: " << message << "\n";
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
				std::cerr << error << "\n";
			}
		}
	} catch(const char *message) {
		std::cerr << "Machine Exit: " << message << "\n";
	} catch(const std::string &message) {
		std::cerr << "Machine Exit: " << message << "\n";
	}

	pthread_exit(NULL);
}

#include <cstdlib>
#include "cmdline.h"
//___________________________________________________________________________________
// Host thread.
int main(int argc, char *argv[]) {
	CPU cpu;
	std::string outfile = "-";
	unsigned long frequency = 8;

	CommandLine cmdline(argc, argv);
	if (cmdline.cmdOptionExists("-h") || argc==1) {
		std::cout << "A PIC16f6xxx simulator\n";
		std::cout << "   Disclaimer:  things may break.  It's not my fault.\n";
		std::cout << "\n";
		std::cout << "sim16f <options>\n";
		std::cout << "  available options:\n";
		std::cout << "    -a filename     - assemble a list of instructions read from <filename>.\n";
		std::cout << "    -x filename     - read a .hex file and configure the CPU.\n";
		std::cout << "    -c config_words - configure the CPU. eg: 'sim16f -c 0x10,0x20 [,...]\n";
		std::cout << "    -d filename     - read a hex file and output disassembled instructions.\n";
		std::cout << "    -o filename     - output to file instead of stdout.\n";
		std::cout << "    -e eeprom_bytes - read in eeprom data. eg: 'sim16f -e 0x10,0x20,[,...]'\n";
		std::cout << "    -f frequency    - set the clock frequency in Hz.\n";
		std::cout << "    -u filename     - dump the current CPU configuration into a hex file.\n";
		std::cout << "    -r              - run the emulator\n";
		std::cout << "    -g              - run the emulator in debug mode\n";
		std::cout << "\n";
		std::cout << "Options may be used together.  For example,\n";
		std::cout << "  'sim16f -c 0x10,0x20 -a test.a -e 0x10,0x20 -u -o test.hex'\n";
		std::cout << "  will produce a hex file that can be read by most PIC programmers,\n";
		std::cout << "and './sim16f -x test.hex -g' will load a hex file and execute with debug.\n";
		std::cout << "\n";
		std::cout << "Note that although frequency is in Hz, the CPU needs 4 clock cycles\n";
		std::cout << "per instruction, so for example, a frequency of 8 should process two \n";
		std::cout << "instructions cycles per second.  Some instructions (eg. goto) consume\n";
		std::cout << "more than one instruction cycle.\n";
		std::cout << "\n";
	}

	try {
		if (cmdline.cmdOptionExists("-f")) {
			char *p;
			std::string freq = cmdline.getCmdOption("-f");
			frequency = strtoul(freq.c_str(), &p, 10);
			if (!p) throw(std::string("Invalid frequency argument: ") + freq);
		}
		if (cmdline.cmdOptionExists("-x")) {
			std::string fn = cmdline.getCmdOption("-x");
			if (!FileExists(fn.c_str())) throw(std::string("File does not exist: ")+fn);
			cpu.load_hex(fn);
		}
		if (cmdline.cmdOptionExists("-a")) {
			std::string fn = cmdline.getCmdOption("-a");
			if (!FileExists(fn.c_str())) throw(std::string("File does not exist: ")+fn);
			try {
				cpu.assemble(fn);
			} catch (std::string &err) {
				std::cout << "error in assembly: " << err << "\n";
			}
		}
		if (cmdline.cmdOptionExists("-c")) {
			cpu.configure(cmdline.getCmdOption("-c"));
		}
		if (cmdline.cmdOptionExists("-e")) {
			cpu.load_eeprom(cmdline.getCmdOption("-e"));
		}
		if (cmdline.cmdOptionExists("-o")) {
			outfile = cmdline.getCmdOption("-o");
		}

		//_______________________________________________________
		// commands with possible output must come after this point
		if (cmdline.cmdOptionExists("-d")) {
			std::string fn = cmdline.getCmdOption("-d");
			if (fn.length()) cpu.disassemble(fn); else cpu.disassemble(fn);
		}
		if (cmdline.cmdOptionExists("-u")) {
			std::string fn = cmdline.getCmdOption("-u");
			if (fn.length()) cpu.dump_hex(fn);
		}

		if (cmdline.cmdOptionExists("-r") || cmdline.cmdOptionExists("-g")) {
			pthread_t machine;
			pthread_create(&machine, NULL, run_machine, &cpu);
			unsigned long clock_speed_hz = frequency;
			unsigned long delay_us = 1000000 / clock_speed_hz;
			std::cout << "Running CPU clock: delay is: " << delay_us << "\n";
			bool debug = cmdline.cmdOptionExists("-g");
			cpu.run(delay_us, debug);
		}
	} catch (std::string &err) {
		std::cerr << "Error: " << err << "\n";
		return 0;
	}

	pthread_exit(NULL);
}
