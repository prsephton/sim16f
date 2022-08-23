#pragma once
#include "../src/devices/clock.h"
#include "../src/devices/register.h"

class ClockCycler {
	Clock clock;  // we will need one of these to fire clock events for our ports
	DeviceEventQueue device_events;

  public:
	void toggle() {
		clock.toggle();
		device_events.process_events();
	}
	void q() {
		clock.toggle();
		device_events.process_events();
		clock.toggle();
		device_events.process_events();
	}
	void cycle() {
		clock.toggle();   // Q1
		clock.toggle();   // Q1
		device_events.process_events();
		clock.toggle();   // Q2
		clock.toggle();   // Q2
		device_events.process_events();
		clock.toggle();   // Q3
		clock.toggle();   // Q3
		device_events.process_events();
		clock.toggle();   // Q4
		clock.toggle();   // Q4
		device_events.process_events();
	}
	ClockCycler() {
		clock.start();   // not in its own thread; we call clock.toggle directly.
	}
};

class ClockedRegister: public Register {    // we use ClockedRegister for testing since there is no clock thread
	ClockCycler clock;

  public:
	void do_cycle() { clock.cycle(); }

	virtual const BYTE read(SRAM &a_sram) {
		Register::read(a_sram);
		clock.cycle();
		return a_sram.read(index());
	}

	virtual void write(SRAM &a_sram, const unsigned char value) {
		Register::write(a_sram, value);
		clock.cycle();
	}

	ClockedRegister(const WORD a_idx, const std::string &a_name, const std::string &a_doc = ""):
		Register(a_idx, a_name, a_doc) {
	}
};

