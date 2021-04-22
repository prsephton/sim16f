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
	bool paused;
	bool skip;
	int  cycles;
	int  nsteps;

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
			data.execPC = PC;
			++PC; PC = PC % FLASH_SIZE;
			data.sram.set_PC(PC);
		}
	}

	void execute() {
		if (cycles) --cycles;
		if (current) {
			skip = current->execute(opcode, data);
			CpuEvent(data.execPC, data.SP, data.W, current->disasm(opcode, data));
		} else {
			CpuEvent(data.execPC, data.SP, data.W, std::string("->fetch ") + int_to_hex(data.sram.get_PC(), "0x") + "                  ");
		}
	}

	static void show_status(void *ob, const CpuEvent &e) {
		std::cout << std::setfill('0') << std::hex << std::setw(4) <<  (int)e.PC << ":\t";
		std::cout << e.disassembly << "\t W:" << std::setw(2) << (int)e.W << "\tSP:" << (int)e.SP << "\n";
	}

  public:
	void reset() {
		paused = true;
		data.SP = 7;
		data.W = 0;
		cycles = 0;
		skip = 0;
		nsteps = 2;        // fetch & execute the first instruction
		data.sram.reset();
	}

	void cycle() {
		if (paused) {
			if (!nsteps) return;
			--nsteps;
		}
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
	void stop() { active = false; }

	void configure(const std::string &a_filename) {};
	void load_eeprom(const std::string &a_filename) {};
	bool load_hex(const std::string &a_filename) { return ::load_hex(a_filename, data); };
	bool dump_hex(const std::string &a_filename) { return ::dump_hex(a_filename, data); };

	bool assemble(const std::string &a_filename) { return ::assemble(a_filename, data, instructions); }
	void disassemble(const std::string &a_filename) { ::disassemble(a_filename, data, instructions); }
	void disassemble() { ::disassemble(data, instructions); }

	CPU_DATA &cpu_data() { return data; }

	void process_queue() {

		while (!data.control.empty()) {
			ControlEvent e = data.control.front(); data.control.pop();
			if (e.name == "pause") paused = true;
			if (e.name == "play") paused = false;
			if (e.name == "next" and paused) nsteps += 1;
			if (e.name == "back") {
				reset();
			} else {
				throw(std::string("Unhandled control event: ") + e.name);
			}
		}

		while (!data.sram.events.empty()) {
			SRAM::Event e = data.sram.events.front(); data.pins.events.pop();
			std::cout << "SRAM: " << e.name << "\n";
			data.process_sram_event(e);
		}

		while (!data.wdt.events.empty()) {
			WDT::Event e = data.wdt.events.front(); data.pins.events.pop();
			std::cout << "WDT: " << e.name << "\n";
		}

		while (!data.pins.events.empty()) {
			PINS::Event e = data.pins.events.front(); data.pins.events.pop();
			if (e.name == "oscillator") {     // positive edge.  4 of these per cycle.

			} else if (e.name == "cycle") {   // an instruction cycle.
				cycle();
			} else {
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

	CPU(): active(true), debug(true), paused(false), skip(false), cycles(0), nsteps(0) {
		memset(data.flash.data, 0, sizeof(data.flash.data));
		memset(data.eeprom.data, 0, sizeof(data.eeprom.data));
		if (debug) {
			CpuEvent::subscribe((void *)this, &CPU::show_status);
		}
		reset();
	}
};

