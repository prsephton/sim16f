/*
 * This module defines the data representation of the CPU, including all memory
 * and other devices.
 *
 * Memory for the 16f62x series consits of flash memory, eeprom and registers (referred to as "file" registers in the
 * data sheet).  The amounts of memory available depends on the microprocessor model.
 *
 * File registers are segmented into four banks, and most of these registers are mapped to a specific function. For
 * example, I/O ports, timers and clock registers are all mapped to file registers.
 *
 * The CPU has direct access to register memory by either first selecting the appropriate bank (another register) and then
 * reading the memory location, or by indirect access via register zero (INDF) and FSR (file select register).
 *
 * Some registers, such as STATUS map to the same location regardless of bank selection, and some registers may be used as
 * arbitrary RAM storage, although very little volatile RAM is available this way.
 *
 * Flash memory is 14 bits wide, and is used exclusively for storage of code to be executed.  The program counter register
 * contains an offset into the flash memory bank.  Addressing of flash memory is constrained to the size of the program
 * counter, which is itself a 13 bit value.
 *
 * EEPROM is memory available to the CPU via EEDATA, EEADR1, EECON1 and EECON2 registers.  This data is non-volatile and
 * 128 bytes (8 bit).  It can be written by the chip programmer, or at run time using EECON1 & EECON2.
 *
 * On a real PIC micro controller, writing to file registers directly results in actions to various devices, such as port pins,
 * timers, and so forth.  In this simulator, we implement a publish-subscribe pattern with an internal queue, which allows
 * for various "device" implementation instances to react to such changes.
 *
 * This same event driven model is followed throughout the rest of this program, providing various event types,
 * such as CPU events (reflecting current CPU execution status changes), UI control events (from the UI to the CPU), and
 * register change events normally triggered by execution of CPU instructions.
 *
 * Devices have their own event queue (see device_base.h) which reflect external changes to actual devices.  For example,
 * we generate clock events by placing events in this queue at regular intervals, or handle single stepping by controlling
 * when clock events are placed in the queue.
 *
 */

#ifndef __cpu_data_h__
#define __cpu_data_h__
#include <map>

#include "devices/constants.h"
#include "devices/flags.h"
#include "devices/devices.h"
#include "devices/sram.h"
#include "utils/smart_ptr.h"
#include "utils/utility.h"


//___________________________________________________________________________________
// This implements a pub-sub pattern which provides current CPU execution status
class CpuEvent {

  public:
	WORD OPCODE;               // OP Code at PC
	WORD PC;                  // program counter
	BYTE SP;                  // stack pointer
	BYTE W;                   // contents of W register
	std::string disassembly;  // disassembled statement
	std::string etype;        // event type

  private:
	typedef void (*CpuStatus)(void *ob, const CpuEvent &event);
	typedef std::map<void *, CpuStatus> registry;
	typedef registry::iterator each_subscriber;
	static registry subscribers;

  public:
	CpuEvent(): OPCODE(0), PC(0), SP(0), W(0), disassembly(""), etype("auto") {}
	CpuEvent(WORD a_opcode, WORD a_pc, BYTE a_sp, BYTE a_w, const std::string &a_disasm, const std::string &a_type):
		OPCODE(a_opcode), PC(a_pc), SP(a_sp), W(a_w), disassembly(a_disasm), etype(a_type) {
		for(each_subscriber s = subscribers.begin(); s!= subscribers.end(); ++s) {
			void *ob = s->first;
			const CpuStatus &cb = s->second;
			cb(ob, *this);
		}
	}

	static void subscribe(void *ob,  CpuStatus callback) {
		subscribers[ob] = callback;
	};

	static void unsubscribe(void *ob) {
		if (subscribers.find(ob) != subscribers.end())
			subscribers.erase(ob);
	};
};

//____________________________________________________________________________________
// A control event is initiated from the UI to change something in the CPU
class ControlEvent {
  public:
	std::string name;
	std::string filename;
	WORD m_data;

	ControlEvent(const std::string &a_name) : name(a_name), filename(""), m_data(0) {}
	ControlEvent(const std::string &a_name, const std::string &a_filename):
			name(a_name), filename(a_filename), m_data(0) {}
};

//___________________________________________________________________________________
class CONFIG: public Register {
  public:
	CONFIG(const std::string &a_name): Register(0, a_name) {}
	virtual const BYTE read(SRAM &a_sram) { return get_value(); }
	virtual void write(SRAM &a_sram, const unsigned char value)	{ set_value(value); }
};

//___________________________________________________________________________________
// Contains the current machine state.  Includes stack, memory and devices.
class CPU_DATA {
  public:
	Params params;   // Parameters for this CPU

	Flash flash;

	WORD execPC;  // PC of currently executing instruction.
	WORD SP;      // SP after execute
	WORD W;       // W after execute
	WORD Config;  // Configuration word

	WORD *stack = NULL;
	std::map<std::string, SmartPtr<Register> > Registers;
	std::map<BYTE, std::string> RegisterNames;

	SRAM       sram;
	PINS       pins;
	Clock      clock;
	EEPROM     eeprom;
	WDT        wdt;     // watch dog timer
	PORTA      porta;
	PORTB      portb;
	Comparator cmp0;
	Timer0     tmr0;
	Timer1     tmr1;
	Timer2     tmr2;
	CONFIG     cfg1;
	CONFIG     cfg2;

	std::queue<ControlEvent> control;
	DeviceEventQueue device_events;
	unsigned long clock_delay_us = 1000000;

	void configure(const std::string &a_configuration) {
		if (a_configuration.length() >= 2) {  // set configuration word
			Config = *(WORD *)a_configuration.c_str();
			cfg1.write(sram, a_configuration.c_str()[0]);  // registers allow device config
			cfg2.write(sram, a_configuration.c_str()[1]);
			std::cout << "config loaded: " << std::hex << Config << "\n";
		}
	}

	const std::string configuration() {  // return config word as a string
		std::cout << "config now: " << std::hex << Config << "\n";
		return std::string((char *)&Config, sizeof(Config));
	}

	void push(WORD value) {
		--SP;
		SP = (SP + params.stack_size) % params.stack_size;
		stack[SP] = value;
	}

	// Event handlers to update machine state, including SRAM
	void register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data);
	void comparator_changed(Comparator *c, const std::string &name, const std::vector<BYTE> &data);
	void timer0_changed(Timer0 *t, const std::string &name, const std::vector<BYTE> &data);
	void portB_changed(PORTB *p, const std::string &name, const std::vector<BYTE> &data);

	WORD pop() {
		WORD value = stack[SP];
		SP = SP % params.stack_size;
		++SP;
		return value;
	}

	std::string register_name(BYTE idx) {
		WORD index = sram.calc_index(idx, false);

		if (RegisterNames.find(idx) == RegisterNames.end())
			return int_to_hex(idx);
		else
			return RegisterNames[index];
	}

	void write_sram(BYTE idx, BYTE v) {
		std::string regname = register_name(idx);
		auto reg = Registers.find(regname);
		if (reg == Registers.end()) {
			sram.write(idx, v, false);
		} else {
			reg->second->write(sram, v);
		}
	}

	const BYTE read_sram(BYTE idx) {
		std::string regname = register_name(idx);
		auto reg = Registers.find(regname);
		if (reg == Registers.end()) {
			return sram.read(idx);
		} else {
			return reg->second->read(sram);
		}
	}

	void reset_registers() {
		for (auto reg: Registers) {
			reg.second->reset(sram);
		}
	}

	void model(const std::string &a_model) {
		auto PIC16f627a = Params{"PIC16f627a", 1024, 128, 4, 0x80, 18, 8};
		auto PIC16f628a = Params{"PIC16f628a", 2048, 128, 4, 0x80, 18, 8};
		auto PIC16f648a = Params{"PIC16f648a", 4096, 256, 4, 0x80, 18, 8};

		if (a_model.find("16f627") != std::string::npos) {
			set_params(PIC16f627a);
		} else  if (a_model.find("16f628") != std::string::npos) {
			set_params(PIC16f628a);
		} else  if (a_model.find("16f648") != std::string::npos) {
			set_params(PIC16f648a);
		} else {
			throw(std::string("Invalid processor choice: "+a_model));
		}
	}

	void set_params(const Params &a_params) {
		std::cout << "setting parameters ...\n";
		params = a_params;
		stack = (WORD *)realloc(stack, (params.stack_size+1) * sizeof(WORD));
		std::cout << "stack allocated ...\n";

		flash.size(params.flash_size);
		std::cout << "flash memory initialised ...\n";
		eeprom.size(params.eeprom_size);
		std::cout << "eeprom memory initialised ...\n";
		sram.init_params(params.ram_banks, params.bank_size);
		std::cout << "SRAM initialised ...\n";
		flash.clear();
		eeprom.clear();
		std::cout << "parameters are set\n";
	}

	CPU_DATA();
	~CPU_DATA();
};

#endif
