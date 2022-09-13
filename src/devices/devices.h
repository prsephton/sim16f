#pragma once

#include "device_base.h"
#include "sram.h"
#include "flags.h"
#include "../utils/utility.h"
#include "register.h"
#include "clock.h"
#include "comparator.h"
#include "timers.h"
#include "simulated_ports.h"

class VREF: public Device {

};

class CCP1: public Device {

};

class USART: public Device {

};

class WDT: public Device {
	DeviceEventQueue eq;
  public:
	void wdt_changed(WDT *r, const std::string &name, const std::vector<BYTE> &data) {
	}

	WDT() {
		DeviceEvent<WDT>::subscribe<WDT>(this, &WDT::wdt_changed);
	}

	void clear() { eq.queue_event(new DeviceEvent<WDT>(*this, "clear")); }
	void sleep() { eq.queue_event(new DeviceEvent<WDT>(*this, "sleep")); }
};


class EEPROM: public Device {
	DeviceEventQueue eq;
	WORD m_size = 0;

  public:
	BYTE *data = NULL;

	void load(const std::string &a_file);

	WORD size() {
		return m_size;
	}

	void size(size_t a_size) {
		m_size = a_size;
		data = (BYTE *)realloc((void *)data, a_size * sizeof(BYTE));
		clear();
	}

	void clear() {
		memset(data, 0, m_size*sizeof(BYTE));
		eq.queue_event(new DeviceEvent<EEPROM>(*this, "clear", {}));
		eq.process_events();
	}

	void set_data(WORD address, const std::string &ds) {
		for(WORD n=0; n<ds.length() && n+address<size(); ++n)
			data[n+address] = ds[n];

		eq.queue_event(new DeviceEvent<EEPROM>(*this, "init", {}));
		eq.process_events();
	}
};


class PINS: public Device {
	static const BYTE PIN_COUNT=18;

	Terminal pins[PIN_COUNT];

  public:

	static const BYTE pin_RA2    = 1;    // Analog comparator input
	static const BYTE pin_AN2    = 1;    // Bidirectional I/O port
	static const BYTE pin_VREF   = 1;    // VREF output
	static const BYTE pin_RA3    = 2;    // Bidirectional I/O port
	static const BYTE pin_AN3    = 2;    // Analog comparator input
	static const BYTE pin_CMP1   = 2;    // Comparator 1 output
	static const BYTE pin_RA4    = 3;    // Bidirectional I/O port
	static const BYTE pin_CMP2   = 3;    // Comparator 2 output
	static const BYTE pin_TOCKI  = 3;    // Timer0 clock input
	static const BYTE pin_RA5    = 4;    // Input port
	static const BYTE pin_MCLR   = 4;    // Master clear. When configured as MCLR, this pin is an active low Reset to the device.
	static const BYTE pin_Vpp    = 4;    // Programming voltage input
	static const BYTE pin_Vss    = 5;    // ground
	static const BYTE pin_RB0    = 6;    // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE pin_INT    = 6;    // External interrupt
	static const BYTE pin_RB1    = 7;    // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE pin_DT     = 7;    // Synchronous data I/O
	static const BYTE pin_RB2    = 8;    // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE pin_CK     = 8;    // Synchronous clock I/O
	static const BYTE pin_RB3    = 9;    // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE pin_CCP1   = 9;    // Capture/Compare/PWM I/O
	static const BYTE pin_RB4    = 10;   // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE pin_PGM    = 10;   // Low-voltage programming input pin.
	static const BYTE pin_RB5    = 11;   // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE pin_RB6    = 12;   // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE pin_T1OSO  = 12;   // Timer1 oscillator output
	static const BYTE pin_T1CKI  = 12;   // Timer1 clock input
	static const BYTE pin_PGC    = 12;   // ICSP™ programming clock
	static const BYTE pin_RB7    = 13;   // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE pin_T1OSI  = 13;   // Timer1 oscillator input
	static const BYTE pin_PGD    = 13;   // ICSP data I/O
	static const BYTE pin_Vdd    = 14;   // positive supply
	static const BYTE pin_CLKOUT = 15;   // CLKOUT, which has 1/4 the frequency of OSC1.
	static const BYTE pin_OSC2   = 15;   // Oscillator crystal output. Connects to crystal or resonator in Crystal Oscillator mode.
	static const BYTE pin_RA6    = 15;   // Bidirectional I/O port
	static const BYTE pin_CLKIN  = 16;   // External clock source input. RC biasing pin.
	static const BYTE pin_OSC1   = 16;   // Oscillator crystal input.
	static const BYTE pin_RA7    = 16;   // Bidirectional I/O port
	static const BYTE pin_RA0    = 17;   // Bidirectional I/O port
	static const BYTE pin_AN0    = 17;   // Analog comparator input
	static const BYTE pin_RA1    = 18;   // Bidirectional I/O port
	static const BYTE pin_AN1    = 18;   // Analog comparator input

	std::map<BYTE, std::string> pin_names  = {
			{17, "RA0/AN0"},                          // 17
			{18, "RA1/AN1"},                          // 18
			{ 1, "RA2/AN2/Vref"},                     // 1
			{ 2, "RA3/AN3/CMP1"},                     // 2
			{ 3, "RA4/T0CK1/CMP2"},                   // 3
			{ 4, "RA5/MCLR/Vpp"},                     // 4
			{15, "RA6/OSC2/CLKOUT"},                  // 15
			{16, "RA7/OSC1/CLKIN"},                   // 16

			{ 6, "RB0/INT"},                          // 6
			{ 7, "RB1/RX/DT"},                        // 7
			{ 8, "RB2/TX/CK"},                        // 8
			{ 9, "RB3/CCP1"},                         // 9
			{10, "RB4/PGM"},                          // 10
			{11, "RB5"},                              // 11
			{12, "RB6/T1OSO/T1CK/PGC"},               // 12
			{13, "RB7/T1OSI/PGD"},                    // 13

			{ 5, "Vss"},                              // 5
			{14, "Vdd"}                               // 14
	};

	Terminal &operator[] (BYTE idx) {
		if (idx==0 or idx > 18)
			throw(std::string("PIN Index is out of range: ") + int_to_string(idx));
		return pins[idx-1];
	}

	const Terminal &operator[] (BYTE idx) const {
		if (idx==0 or idx > 18)
			throw(std::string("PIN Index is out of range: ") + int_to_string(idx));
		return pins[idx-1];
	}

	void reset() {
		for (unsigned int i = 0; i < PIN_COUNT; ++i) {
			pins[i].set_value(Vss, true);
		}
		pins[pin_Vdd-1].set_value(Vdd, false);
	}

	void register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data) {
//		std::cout << "Register changed " << name << std::endl;
	}

	PINS() {
		for (int n = 0; n < PIN_COUNT; ++n) {
			pins[n].name(pin_names[n+1]);
		}
		reset();
		DeviceEvent<Register>::subscribe<PINS>(this, &PINS::register_changed);
	}

};


class PORTA: public Device {
	Connection Comp1;
	Connection Comp2;
	PINS &pins;
	DeviceEventQueue eq;

	void register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	}

  public:
	std::vector< SmartPtr<Device> > RA;
	PORTA(PINS &a_pins): pins(a_pins) {
		RA.resize(8);
		DeviceEvent<Register>::subscribe<PORTA>(this, &PORTA::register_changed);
		RA[0] = new SinglePortA_Analog(pins[PINS::pin_RA0], "RA0");
		RA[1] = new SinglePortA_Analog(pins[PINS::pin_RA1], "RA1");
		RA[2] = new SinglePortA_Analog_RA2(pins[PINS::pin_RA2], "RA2");
		RA[3] = new SinglePortA_Analog_RA3(pins[PINS::pin_RA3], "RA3");
		RA[4] = new SinglePortA_Analog_RA4(pins[PINS::pin_RA4], "RA4");
		RA[5] = new SinglePortA_MCLR_RA5(pins[PINS::pin_RA5], "RA5");
		RA[6] = new SinglePortA_RA6_CLKOUT(pins[PINS::pin_RA6], "RA6");
		RA[7] = new PortA_RA7(pins[PINS::pin_RA7], "RA7");
	}

	std::vector<BYTE> pin_numbers = {
			PINS::pin_RA0,
			PINS::pin_RA1,
			PINS::pin_RA2,
			PINS::pin_RA3,
			PINS::pin_RA4,
			PINS::pin_RA5,
			PINS::pin_RA6,
			PINS::pin_RA7
	};

};


class PORTB: public Device {
	PINS &pins;
	DeviceEventQueue eq;
	bool rising_rb0_interrupt;

	void INT_changed(Connection *c, const std::string &name, const std::vector<BYTE> &data) {
		if (c->signal() ^ not rising_rb0_interrupt){
			eq.queue_event(new DeviceEvent<PORTB>(*this, "PORTB::INTF", {}));
		}
	}

	void register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		if (name=="OPTION"){
			BYTE changed = data[Register::DVALUE::CHANGED];
			BYTE new_value = data[Register::DVALUE::NEW];

			if (changed & Flags::OPTION::RBPU) {
				recalc_pullups(pins, new_value & Flags::OPTION::RBPU);
			}

			if (changed & Flags::OPTION::INTEDG) {
				rising_rb0_interrupt = (new_value & Flags::OPTION::INTEDG) != 0;
			}
		}
	}

  public:
	std::vector< SmartPtr<Device> > RB;

	PORTB(PINS &a_pins): pins(a_pins) {
		rising_rb0_interrupt = false;
		RB.resize(8);
		DeviceEvent<Register>::subscribe<PORTB>(this, &PORTB::register_changed);
		RB[0] = new PortB_RB0(pins[PINS::pin_RB0], "RB0");
		RB[1] = new PortB_RB1(pins[PINS::pin_RB1], "RB1");
		RB[2] = new PortB_RB2(pins[PINS::pin_RB2], "RB2");
		RB[3] = new PortB_RB3(pins[PINS::pin_RB3], "RB3");
		RB[4] = new PortB_RB4(pins[PINS::pin_RB4], "RB4");
		RB[5] = new PortB_RB5(pins[PINS::pin_RB5], "RB5");
		RB[6] = new PortB_RB6(pins[PINS::pin_RB6], "RB6");
		RB[7] = new PortB_RB7(pins[PINS::pin_RB7], "RB7");

		Connection &INT =  dynamic_cast< PortB_RB0 *>(RB[0].operator ->()) -> INT();
		DeviceEvent<Connection>::subscribe<PORTB>(this, &PORTB::INT_changed, &INT);
	}
	~PORTB() {

	}

	std::vector<BYTE> pin_numbers = {
			PINS::pin_RB0,
			PINS::pin_RB1,
			PINS::pin_RB2,
			PINS::pin_RB3,
			PINS::pin_RB4,
			PINS::pin_RB5,
			PINS::pin_RB6,
			PINS::pin_RB7
	};


	void recalc_pullups(PINS &pins, bool RBPU) {}
};


class Flash: public Device {
//	std::vector<WORD>data;
	DeviceEventQueue eq;
	WORD m_size = 0;
  public:
	Flash() : Device("FLASH") {}
	WORD *data = NULL;

	void load(const std::string &a_file);

	WORD fetch(WORD PC) {
		return data[PC % size()];
	}

	void clear() {
		memset(data, 0, m_size * sizeof(WORD));
		eq.queue_event(new DeviceEvent<Flash>(*this, "clear", {}));
		eq.process_events();
	}

	void reset() {  // tell everything to reread flash data
		eq.queue_event(new DeviceEvent<Flash>(*this, "reset", {}));
		eq.process_events();
	}

	WORD size() {
		return m_size;
	}

	void size(size_t a_size) {
		m_size = a_size;
		data = (WORD *)realloc((void *)data, a_size * sizeof(WORD));
		clear();
	}

	void set_data(WORD address, const std::string &ds) {
		for(WORD n=0; n<ds.length() && n+address<size(); n += 2)
			data[(n+address)/2] = (((WORD)ds[n+1]) << 8) | (BYTE)ds[n];
		eq.queue_event(new DeviceEvent<Flash>(*this, "init", {}));
		eq.process_events();
	}

};

