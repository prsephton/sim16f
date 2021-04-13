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
// Contains the current machine state.  Includes stack, memory and devices.
class CPU_DATA {
  public:
	SRAM  sram;
	Flash flash;

	WORD SP;
	WORD W;

	WORD stack[STACK_SIZE];
	std::map<std::string, SmartPtr<Register> > Registers;
	std::map<BYTE, std::string> RegisterNames;


	PINS pins;
	Clock clock;
	EEPROM eeprom;

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
		idx += sram.bank() * BANK_SIZE;
		if (RegisterNames.find(idx) == RegisterNames.end())
			return int_to_hex(idx);
		else
			return RegisterNames[idx];
	}

	CPU_DATA();
};

#endif
