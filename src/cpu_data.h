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
// Contains the current machine state.  Includes stack, memory and devices.
class CPU_DATA {
  public:
	SRAM  sram;
	Flash flash;

	WORD execPC;  // PC of currently executing instruction.
	WORD SP;      // SP after execute
	WORD W;       // W after execute
	WORD Config;  // Configuration word

	WORD stack[STACK_SIZE];
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
	DeviceEventQueue device_events;

	void configure(const std::string &a_configuration) {
		if (a_configuration.length() >= 2) {  // set configuration word
			Config = *(WORD *)a_configuration.c_str();
			std::cout << "config loaded: " << std::hex << Config << "\n";
		}
	}

	const std::string configuration() {  // return config word as a string
		std::cout << "config now: " << std::hex << Config << "\n";
		return std::string((char *)&Config, sizeof(Config));
	}

	void push(WORD value) {
		--SP;
		SP = (SP + STACK_SIZE) % STACK_SIZE;
		stack[SP] = value;
	}

	void register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data);

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

	void write_sram(BYTE idx, BYTE v) {
		std::string regname = register_name(idx);
		auto reg = Registers.find(regname);
		if (reg == Registers.end()) {
			sram.write(idx, v, false);
		} else {
			reg->second->write(sram, v);
		}
	}

	CPU_DATA();
};

#endif
