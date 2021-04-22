#ifndef __cpu_data_h__
#define __cpu_data_h__
#include <map>

#include "constants.h"
#include "flags.h"
#include "devices.h"
#include "flags.h"
#include "sram.h"
#include "utility.h"
#include "smart_ptr.h"


//___________________________________________________________________________________
// A file register is a memory location having special significance.
class Register {
	WORD m_idx;
	std::string m_name;

  public:
	Register(const WORD a_idx, const std::string &a_name)
  	  : m_idx(a_idx), m_name(a_name) {
	}
	WORD index() { return m_idx; }
	virtual ~Register() {
	}

	virtual const BYTE read(const SRAM &a_sram) {         // default read for registers
		return(a_sram.read(m_idx));
	}

	virtual void write(SRAM &a_sram, const unsigned char value) {  // default write
		a_sram.write(m_idx, value);
	}
};

//___________________________________________________________________________________
// This implements a pub-sub pattern which provides current CPU execution status
class CpuEvent {

  public:
	WORD PC;                  // program counter
	BYTE SP;                  // stack pointer
	BYTE W;                   // contents of W register
	std::string disassembly;  // disassembled statement

  private:
	typedef void (*CpuStatus)(void *ob, const CpuEvent &event);
	typedef std::map<void *, CpuStatus> registry;
	typedef registry::iterator each_subscriber;
	static registry subscribers;

  public:
	CpuEvent(): PC(0), SP(0), W(0), disassembly("") {}
	CpuEvent(WORD a_pc, BYTE a_sp, BYTE a_w, const std::string &a_disasm):
		PC(a_pc), SP(a_sp), W(a_w), disassembly(a_disasm) {
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
// Contains the current machine state.  Includes stack, memory and devices.
class CPU_DATA {
  public:
	SRAM  sram;
	Flash flash;

	WORD execPC;  // PC of currently executing instruction.
	WORD SP;      // SP after execute
	WORD W;       // W after execute

	WORD stack[STACK_SIZE];
	std::string configuration;
	std::map<std::string, SmartPtr<Register> > Registers;
	std::map<BYTE, std::string> RegisterNames;

	PINS   pins;
	Clock  clock;
	EEPROM eeprom;
	WDT    wdt;     // watch dog timer
	PORTA  porta;
	PORTB  portb;
	Timer0 tmr0;
	Timer1 tmr1;
	Timer2 tmr2;

	std::queue<ControlEvent> control;

	void push(WORD value) {
		--SP;
		SP = (SP + STACK_SIZE) % STACK_SIZE;
		stack[SP] = value;
	}

	WORD pop() {
		WORD value = stack[SP];
		++SP;
		SP = SP % STACK_SIZE;
		return value;
	}

	std::string register_name(BYTE idx) {
		BYTE bank, ofs;
		if (!sram.calc_bank_ofs(idx, bank, ofs, false))
			return int_to_hex(idx);

		idx = bank * BANK_SIZE + ofs;

		if (RegisterNames.find(idx) == RegisterNames.end())
			return int_to_hex(idx);
		else
			return RegisterNames[idx];
	}

	void process_option(const SRAM::Event &e);
	void process_sram_event(const SRAM::Event &e);


	CPU_DATA();
};

#endif
