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
#include <chrono>
#include <thread>
#include <pthread.h>
#include <queue>

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
	bool interrupt_pending;

	std::queue<std::string> instruction_cycles;
	unsigned long clock_delay_us;
	std::string disassembled;

	void fetch() {
		if (cycles > 0) {
			current = NULL;  // flush
		} else {
			WORD PC = data.sram.get_PC();
			if (skip)
				current = instructions.find(0);
			else {
//				if (debug) std::cout << "Fetch instruction @" << std::hex << (int)PC << std::endl;
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
//			if (debug) std::cout << "Execute instruction " << current->mnemonic << std::endl;
			disassembled = current->disasm(opcode, data);
			CpuEvent(opcode, data.execPC, data.SP, data.W, disassembled, "start");
			skip = current->execute(opcode, data);
			CpuEvent(opcode, data.execPC, data.SP, data.W, disassembled, "after");
		} else if (disassembled.length()) {   // fetch/flush cycle
//			if (debug) std::cout << "Cycle" << std::endl;
			auto st = disassembled.substr(0, 10) + "*" + disassembled.substr(11);
			CpuEvent(opcode, data.execPC, data.SP, data.W, st, "flush");
		}
	}

	void interrupt() {
		// clear GIE, push PC, set PC = 0x4;
		SmartPtr<Register>INTCON = data.Registers["INTCON"];
		INTCON->write(INTCON->get_value() & ~Flags::INTCON::GIE);

		WORD PC = data.sram.get_PC();
		if (current) PC -= 1;
		cycles = 0;
		current = NULL;
		disassembled = "";
		data.push(PC);
		data.execPC = PC;
		PC = 0x4;   // Interrupt vector
		data.sram.set_PC(PC);
		fetch();
		cycle();
	}

	static void show_status(void *ob, const CpuEvent &e) {
		if (e.etype == "after") {
			std::cout << std::setfill('0') << std::hex << std::setw(4) <<  (int)e.PC << ":\t";
			std::cout << e.disassembly << "\t W:" << std::setw(2) << (int)e.W << "\tSP:" << (int)e.SP << "\n";
		}
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
		disassembled = "";
		data.sram.reset();
		interrupt_pending = false;

		data.Registers["STATUS"]-> write(0b00011000);
		data.Registers["OPTION"]-> write(0b11111111);
		data.Registers["TRISA"] -> write(0b11111111);
		data.Registers["TRISB"] -> write(0b11111111);
		data.Registers["PCON"]  -> write(0b00001000);
		data.Registers["PR2"]   -> write(0b11111111);
		data.Registers["TXSTA"] -> write(0b00000010);

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

	bool process_queue() {
		try {
			if (not instruction_cycles.empty()) {
				const std::string &name = instruction_cycles.front();
				if (name == "INTERRUPT") {
					interrupt();
				} else {
					cycle();
				}
				instruction_cycles.pop();
				if (not instruction_cycles.empty()) {  // cycling too fast- we will need to slow the clock speed down
					std::cout << "Cannot process instructions fast enough: Slowing down clock" << std::endl;
					clock_delay_us += instruction_cycles.size() * 10;   // just add a small delay per cycle
					std::cout << "Clock frequency is now: " << 1/clock_delay_us << " MHz" << std::endl;
				}
				return true;
			} else if (data.device_events.size()) {
				data.device_events.process_events();
				return true;
			} else if (!data.control.empty()) {
				while (!data.control.empty()) {
					ControlEvent e = data.control.front(); data.control.pop();
					if (e.name == "pause") paused = true;
					if (e.name == "play") paused = false;
					if (e.name == "next" and paused) nsteps += 1;
					if (e.name == "back") reset();
					if (e.name == "reset") reset();
				}
				return true;
			}
		} catch (std::exception &e) {
			std::cout << e.what() << std::endl;
		} catch (...) {
		}
		return false;
	}

	// We monitor certain registers to perform interrupts
	void register_event(Register *r, const std::string &name, const std::vector<BYTE> &rdata){
		bool generate_interrupt = false;
		if (name == "INTCON") {
			BYTE t0_mask  = (Flags::INTCON::T0IE | Flags::INTCON::T0IF | Flags::INTCON::GIE);
			BYTE rb_mask  = (Flags::INTCON::RBIE | Flags::INTCON::RBIF | Flags::INTCON::GIE);
			BYTE int_mask = (Flags::INTCON::INTE | Flags::INTCON::INTF | Flags::INTCON::GIE);
			if        ((r->get_value() & t0_mask) == t0_mask)   {   // Timer0 interrupt triggered
				generate_interrupt = true;
			} else if ((r->get_value() & rb_mask) == rb_mask)   {   // Port_B[4..7] interrupt triggered
				generate_interrupt = true;
			} else if ((r->get_value() & int_mask) == int_mask) {   // External interrupt [RB0] triggered
				generate_interrupt = true;
			}
		} else if (name == "PIR1") {
			SmartPtr<Register>INTCON = data.Registers["INTCON"];
			BYTE enabled = Flags::INTCON::GIE | Flags::INTCON::PEIE;
			if ((INTCON->get_value() & enabled) == enabled) {
				SmartPtr<Register>PIE1 = data.Registers["PIE1"];
				BYTE mask = r->get_value() & PIE1->get_value(); // PIR1 & PIE1 map bit for bit over each other
				if        (mask & Flags::PIR1::EEIF) {          // EEProm write complete interrupt triggered
					generate_interrupt = true;
				} else if (mask & Flags::PIR1::CMIF) {          // Comparator interrupt triggered
					generate_interrupt = true;
				} else if (mask & Flags::PIR1::RCIF) {          // USART Received interrupt triggered
					generate_interrupt = true;
				} else if (mask & Flags::PIR1::TXIF) {          // USART Transmit interrupt triggered
					generate_interrupt = true;
				} else if (mask & Flags::PIR1::CCP1IF) {        // CCP1 interrupt triggered
					generate_interrupt = true;
				} else if (mask & Flags::PIR1::TMR2IF) {        // Timer2 PR2 Match interrupt triggered
					generate_interrupt = true;
				} else if (mask & Flags::PIR1::TMR1IF) {        // Timer1 Overflow interrupt triggered
					generate_interrupt = true;
				}
			}
		}
		if (generate_interrupt)
			interrupt_pending = true;
	}

	void clock_event(Clock *device, const std::string &name, const std::vector<BYTE> &data){
		//   This is called from within the clock thread.  If we process instructions directly
		// from this thread, then there will be a conflict between instruction processing
		// and device events.  So here we need to place the clock event on a queue and
		// cycle instructions as clock events appear.
		if (name == "oscillator") {     // positive edge.  4 of these per cycle.
		} else if (name == "cycle") {   // an instruction cycle.
			if (!paused or nsteps) {
//				if (cycles > 1)
//					instruction_cycles.push(name);
				if (interrupt_pending) {
					instruction_cycles.push("INTERRUPT");
					interrupt_pending = false;
				} else {
					instruction_cycles.push(name);
				}
			}
//		} else {
//			std::cout << name << ":" << std::endl;
		}
	}

	void run_clock(unsigned long delay_us, bool a_debug=false) {  // run the clock
		clock_delay_us = delay_us;
		debug = a_debug;
		while (running()) {
			sleep_for_us(clock_delay_us);
			toggle_clock();
		}
	}

	virtual ~CPU() {
		DeviceEvent<Clock>::unsubscribe<CPU>(this, &CPU::clock_event);
		DeviceEvent<Register>::unsubscribe<CPU>(this, &CPU::register_event);
	}

	CPU(): active(true), debug(true), paused(true), skip(false), cycles(0), nsteps(0) {
		memset(data.flash.data, 0, sizeof(data.flash.data));
		memset(data.eeprom.data, 0, sizeof(data.eeprom.data));

		DeviceEvent<Clock>::subscribe<CPU>(this, &CPU::clock_event);
		DeviceEvent<Register>::subscribe<CPU>(this, &CPU::register_event);

		if (debug) {
			CpuEvent::subscribe((void *)this, &CPU::show_status);
		}
		reset();
	}
};
