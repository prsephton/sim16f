/*
 * The 16fxxx CPU executes instructions at a given clock frequency by fetching an instruction
 * indexed by the current value of the Program Counter (PC), and simultaneously executing a previously
 * fetched instruction if available, then incrementing the PC.  We do the fetch and execute sequentially
 * rather than in parallel, but that should not matter to the simulation.
 *
 * Some instructions take two, rather than one clock cycle to execute, in which case the fetch is delayed
 * for one cycle.   In the case of branch (on sign or carry) instructions, execution continues at the
 * provided address by loading it into the PC, or skips the provided address and continues directly after,
 * which we simulate by turning the next instruction into a NOP.
 *
 * The input clock is divided into four stages, so that a 4Mhz clock executes instructions at 1MHz.
 *
 */

#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <exception>
#include <unistd.h>
#include <pthread.h>

#include "devices/constants.h"
#include "devices/devices.h"
#include "utils/hex.h"
#include "utils/assembler.h"
#include "utils/utility.h"
#include "cpu_data.h"
#include "instructions.h"

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

	std::string disassembled;

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
			disassembled = current->disasm(opcode, data);
			CpuEvent(opcode, data.execPC, data.SP, data.W, disassembled);
		} else if (disassembled.length()) {   // fetch/flush cycle
			auto st = disassembled.substr(0, 10) + "*" + disassembled.substr(11);
			CpuEvent(opcode, data.execPC, data.SP, data.W, st);
		}
	}

	static void show_status(void *ob, const CpuEvent &e) {
		std::cout << std::setfill('0') << std::hex << std::setw(4) <<  (int)e.PC << ":\t";
		std::cout << e.disassembly << "\t W:" << std::setw(2) << (int)e.W << "\tSP:" << (int)e.SP << "\n";
	}

  public:
	void reset() {
		data.clock.stop();

		data.device_events.clear();
		while (! data.control.empty()) data.control.pop();

		nsteps = 0;
		paused = true;
		current = NULL;
		data.SP = 8;
		data.W = 0;
		cycles = 0;
		skip = 0;

		data.sram.reset();

		data.Registers["STATUS"]-> write(data.sram, 0b00011000);
		data.Registers["OPTION"]-> write(data.sram, 0b11111111);
		data.Registers["TRISA"] -> write(data.sram, 0b11111111);
		data.Registers["TRISB"] -> write(data.sram, 0b11111111);
		data.Registers["PCON"]  -> write(data.sram, 0b00001000);
		data.Registers["PR2"]   -> write(data.sram, 0b11111111);
		data.Registers["TXSTA"] -> write(data.sram, 0b00000010);

		nsteps = 2;        // fetch & execute the first instruction
		data.clock.start();
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

	void toggle_clock() { data.clock.toggle(); }

	bool running() const { return active; }
	void stop() {
		data.device_events.clear();
		while (! data.control.empty()) data.control.pop();
		active = false;
	}

	void configure(const std::string &a_filename) {};
	void load_eeprom(const std::string &a_filename) {};
	bool load_hex(const std::string &a_filename) { return ::load_hex(a_filename, data); };
	bool dump_hex(const std::string &a_filename) { return ::dump_hex(a_filename, data); };

	bool assemble(const std::string &a_filename) { return ::assemble(a_filename, data, instructions); }
	void disassemble(const std::string &a_filename) { ::disassemble(a_filename, data, instructions); }
	void disassemble() { ::disassemble(data, instructions); }

	CPU_DATA &cpu_data() { return data; }

	void process_queue() {
		try {
			data.device_events.process_events();
			while (!data.control.empty()) {
				ControlEvent e = data.control.front(); data.control.pop();
				if (e.name == "pause") paused = true;
				if (e.name == "play") paused = false;
				if (e.name == "next" and paused) nsteps += 1;
				if (e.name == "back") reset();
				if (e.name == "reset") {

					reset();
				}
			}
		} catch (std::exception &e) {
			std::cout << e.what() << std::endl;
		} catch (...) {
		}
	}

	void clock_event(Clock *device, const std::string &name, const std::vector<BYTE> &data){
		if (name == "oscillator") {     // positive edge.  4 of these per cycle.

		} else if (name == "cycle") {   // an instruction cycle.
			cycle();
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

	CPU(): active(true), debug(true), paused(true), skip(false), cycles(0), nsteps(0) {
		memset(data.flash.data, 0, sizeof(data.flash.data));
		memset(data.eeprom.data, 0, sizeof(data.eeprom.data));

		DeviceEvent<Clock>::subscribe<CPU>(this, &CPU::clock_event);

		if (debug) {
			CpuEvent::subscribe((void *)this, &CPU::show_status);
		}
		reset();
	}
};
