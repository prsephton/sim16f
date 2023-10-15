#pragma once

#include "device_base.h"
#include "device_queue.h"
#include "sram.h"
#include "clock.h"
#include "flags.h"
#include "../utils/utility.h"

//___________________________________________________________________________________
// A file register is a memory location having special significance.
// Registers also map directly to hardware devices.  So writing to a register
// is the same as writing to the hardware, and reading a register reads from
// hardware.
class Register : public Device {
	WORD m_idx;
	std::string m_doc;
	BYTE m_value;
	volatile bool m_busy;
  public:

	struct DVALUE {
		static const int OLD      = 0;
		static const int CHANGED  = 1;
		static const int NEW      = 2;
	};
	DeviceEventQueue eq;

	Register(const WORD a_idx, const std::string &a_name, const std::string &a_doc = "")
  	  : Device(a_name), m_idx(a_idx), m_doc(a_doc), m_value(0), m_busy(false) {
	}
	WORD index() { return m_idx; }
	void busy(bool flag) { m_busy = flag; }
	virtual bool busy() const { return m_busy; }
	virtual ~Register() {}

	void trigger_change(BYTE a_new, BYTE a_old, BYTE a_changed) {
		eq.queue_event(new DeviceEvent<Register>(*this, name(), {a_old, a_changed, a_new}));
	}

	BYTE get_value() {
		return m_value;
	}

	bool set_value(BYTE a_value, BYTE a_old=0) {
		BYTE changed = a_old ^ a_value; // all changing bits.
		m_value = a_value;
		if (debug()) {
			std::cout << "Register " << name() << " setting value from " << std::hex << int(a_old) << " to " << std::hex << (int)a_value;
			std::cout << "; changed=" << (int)changed << std::endl;
		}
		if (changed) {
			trigger_change(a_value, a_old, changed);
			return true;
		}
		return false;
	}

	void reset(const SRAM &a_sram) {     // refresh m_value from sram
		m_value = a_sram.read(m_idx);
	}

	virtual const BYTE read(const SRAM &a_sram) {         // default read for registers
		busy(true);
		eq.queue_event(new DeviceEvent<Register>(*this, name()+".read", {m_value, 0, 0}));
		eq.process_events();             // perform the device read, update m_value
		while (busy()) {
			sleep_for_us(10);
			eq.process_events();         // perform the device read, update m_value
		}
		return m_value;
	}

	virtual void write(SRAM &a_sram, const BYTE value) {  // default write
		set_value(value, m_value);
	}
};


