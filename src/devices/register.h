#pragma once

#include "device_base.h"
#include "sram.h"
#include "flags.h"
#include "../utils/utility.h"

//___________________________________________________________________________________
// A file register is a memory location having special significance.
// Registers also map directly to hardware devices.  So writing to a register
// is the same as writing to the hardware, and reading a register reads from
// hardware.
class Register : public Device {
	WORD m_idx;
	std::string m_name;
	std::string m_doc;
	BYTE m_value;

  public:
	DeviceEventQueue eq;

	Register(const WORD a_idx, const std::string &a_name, const std::string &a_doc = "")
  	  : m_idx(a_idx), m_name(a_name), m_doc(a_doc), m_value(0) {
	}
	WORD index() { return m_idx; }
	virtual ~Register() {}

	bool set_value(BYTE a_value, BYTE a_old=0) {
		BYTE changed = a_old ^ a_value; // all changing bits.
		m_value = a_value;
		if (changed) {
			eq.queue_event(new DeviceEvent<Register>(*this, m_name, {a_old, changed, a_value}));
			return true;
		}
		return false;
	}

	virtual const BYTE read(SRAM &a_sram) {         // default read for registers
		BYTE old_value = a_sram.read(m_idx);
		m_value = old_value;
		eq.queue_event(new DeviceEvent<Register>(*this, m_name+".read", {0, 0, 0}));
		eq.process_events();         // perform the device read, update m_value
		if (old_value != m_value) {
			a_sram.write(m_idx, m_value);
		}
		return m_value;
	}

	virtual void write(SRAM &a_sram, const unsigned char value) {  // default write
		BYTE old = a_sram.read(index());
		if (set_value(value, old))
			a_sram.write(m_idx, value);
	}
};


