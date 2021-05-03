#pragma once

#include "device_base.h"
#include "sram.h"
#include "flags.h"

//___________________________________________________________________________________
// A file register is a memory location having special significance.
class Register : public Device {
	WORD m_idx;
	std::string m_name;

  public:
	DeviceEventQueue eq;

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
		BYTE old = a_sram.read(index());
		BYTE changed = old ^ value; // all changing bits.
		if (changed)
			eq.queue_event(new DeviceEvent<Register>(*this, m_name, {old, changed, value}));
		a_sram.write(m_idx, value);
	}
};

//___________________________________________________________________________________
//  A model for a single port for pins RA0/AN0, RA1/AN1
class SinglePortA_Analog: public Device {
	Connection &Pin;
	Connection Comparator;
	Connection CMCON;
	Schmitt *trigger;
	Connection Data;
	Connection PortA;
	Connection TrisA;

  protected:
	std::map<std::string, SmartPtr<Device> > components;

	void set_comparator(bool on) {
		if (on) {
			trigger->set_impeded(true);
			Comparator.set_value(Comparator.rd(), false);
		} else {
			trigger->set_impeded(false);
			Comparator.set_value(Comparator.rd(), true);
		}
	}

	void set_comparators_for_an0_and_an1(BYTE cmcon) {
		switch (cmcon & 0b111) {
		case 0b000 :    // Comparators reset
			trigger->set_impeded(true);
			Comparator.set_value(Vss, false);
			break;
		case 0b001 :    // 3 inputs Multiplexed 2 Comparators
			if (name() == "AN0" || name() == "RA0")
				set_comparator((cmcon & Flags::CMCON::CIS) == 0);
			else
				set_comparator(true);
			break;
		case 0b010 :    // 4 inputs Multiplexed 2 Comparators
			set_comparator((cmcon & Flags::CMCON::CIS) == 0);
			break;
		case 0b011 :    // 2 common reference comparators
		case 0b100 :    // Two independent comparators
			set_comparator(true);
			break;
		case 0b101 :    // One independent comparator
			if (this->name() == "AN0" || this->name() == "RA0")
				set_comparator(false);
			else
				set_comparator(true);
			break;
		case 0b110 :    // Two common reference comparators with outputs
			set_comparator(true);
			break;
		case 0b111 :    // Comparators off
			set_comparator(false);
			break;
		}
	}

	void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		if        (name == "CMCON") {
			BYTE cmcon = data[2];
			set_comparators_for_an0_and_an1(cmcon);
		} else if (name == "VRCON") {
		} else if (name == "PORTA") {
			if (name=="AN0" || name == "RA0") {
				bool input = ((data[2] & Flags::PORTA::RA0) == Flags::PORTA::RA0);
				PortA.set_value(input * Vdd, true);
			} else {
				bool input = ((data[2] & Flags::PORTA::RA1) == Flags::PORTA::RA1);
				PortA.set_value(input * Vdd, true);
			}
		} else if (name == "TRISA") {
			if (name=="AN0" || name == "RA0") {
				bool input = ((data[2] & Flags::TRISA::TRISA0) == Flags::TRISA::TRISA0);
				PortA.set_value(input * Vdd, true);
			} else {
				bool input = ((data[2] & Flags::TRISA::TRISA1) == Flags::TRISA::TRISA1);
				PortA.set_value(input * Vdd, true);
			}
		}
	}

  public:
	SinglePortA_Analog(Connection &a_Pin, const std::string &a_name): Device(a_name),
		Pin(a_Pin), Comparator(Vss, true), CMCON(Vss, true), trigger(0)
	{
		Wire *DataBus = new Wire();
		Wire *PinWire = new Wire();

		Latch *DataLatch = new Latch(Data, PortA);
		Latch *TrisLatch = new Latch(Data, TrisA);

		Tristate *Tristate1 = new Tristate(DataLatch->Q(), TrisLatch->Q(), true);
		Clamp * PinClamp = new Clamp(Pin);

		PinWire->connect(Pin);
		PinWire->connect(Tristate1->rd());
		PinWire->connect(Comparator);

		Schmitt *trigger = new Schmitt(Comparator);

		Inverter *not_porta = new Inverter(PortA);
		Latch *SR1 = new Latch(trigger->rd(), not_porta->rd());
		Tristate *Tristate2 = new Tristate(SR1->Q(), PortA);
		DataBus->connect(Tristate2->rd());

		Tristate *Tristate3 = new Tristate(TrisLatch->Qc(), TrisA, false, true);
		DataBus->connect(Tristate3->rd());

		components["Data bus"] = DataBus;
		components["Pin wire"] = PinWire;
		components["Pin clamp"] = PinClamp;
		components["Data Latch"] = DataLatch;
		components["Tris Latch"] = TrisLatch;
		components["Tristate1"] = Tristate1;
		components["Tristate2"] = Tristate2;
		components["Tristate3"] = Tristate3;
		components["Schmitt Trigger"] = trigger;
		components["SR1"] = SR1;

		DeviceEvent<Register>::subscribe<SinglePortA_Analog>(this, &SinglePortA_Analog::on_register_change);
	}
};

//___________________________________________________________________________________
class Comparator: public Device {

};

//___________________________________________________________________________________
class Timer0: public Device {
	bool assigned_to_wdt;
	bool falling_edge;
	bool use_RA4;
	BYTE prescale_rate;

  public:
	void clock_source_select(bool a_use_RA4){
		use_RA4 = a_use_RA4;
	};
	void clock_transition(bool a_falling_edge){
		falling_edge = a_falling_edge;
	};
	void prescaler(bool a_assigned_to_wdt){
		assigned_to_wdt = a_assigned_to_wdt;
	};
	void prescaler_rate_select(BYTE a_prescale_rate){
		// bits   000   001   010   011   100    101    110     111
		// TMR0   1:2   1:4   1:8   1:16  1:32   1:64   1:128   1:256
		// WDT    1:1   1:2   1:4	1:8   1:16   1:32   1:64    1:128
		prescale_rate = a_prescale_rate;
	};
};

class Timer1: public Device {

};

class Timer2: public Device {

};

class VREF: public Device {

};

class CCP1: public Device {

};

class USART: public Device {

};

class WDT: public Device {
  public:
	class Event {
  	  public:
		std::string name;
		Event(std::string a_name) : name(a_name){}
	};
	std::queue<Event> events;

	void clear() { events.push(Event("cleared")); }
	void sleep() { events.push(Event("sleep")); }
};


class EEPROM: public Device {
  public:
	BYTE data[EEPROM_SIZE];

	void load(const std::string &a_file);

	void clear() {
		memset(data, 0, sizeof(data));
	}
	void set_data(WORD address, const std::string &ds) {
		for(WORD n=0; n<ds.length() && n+address<EEPROM_SIZE; ++n)
			data[n+address] = ds[n];
	}
};

class Clock: public Device {
  public:
	bool high;
	BYTE phase;
	BYTE Q1;
	BYTE Q2;
	BYTE Q3;
	BYTE Q4;

	Clock(): high(false), phase(0), Q1(1), Q2(0), Q3(0), Q4(0) {}

	void toggle();
};


class PINS: public Device {

	Connection pins[PIN_COUNT];

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
	static const BYTE pin_RAS6   = 15;   // Bidirectional I/O port
	static const BYTE pin_CLKIN  = 16;   // External clock source input. RC biasing pin.
	static const BYTE pin_OSC1   = 16;   // Oscillator crystal input.
	static const BYTE pin_RA7    = 16;   // Bidirectional I/O port
	static const BYTE pin_RA0    = 17;   // Bidirectional I/O port
	static const BYTE pin_AN0    = 17;   // Analog comparator input
	static const BYTE pin_RA1    = 18;   // Bidirectional I/O port
	static const BYTE pin_AN1    = 18;   // Analog comparator input


// These are the 'external signals' we configure this device to show for inputs or outputs.
//	float get_pin(BYTE pin_no) { return pins[pin_no-1]; }
//	void  set_pin(BYTE pin_no, bool high) { pins[pin_no-1] = high?5.0:0.0; }

	void reset() {
		for (unsigned int i = 0; i < sizeof(pins); ++i) {
			pins[i].set_value(Vss, true);
		}
		pins[pin_Vdd-1].set_value(Vdd, false);
	}

	void clock_event(Clock *r, const std::string &name, const std::vector<BYTE> &data) {
		if (name == "oscillator") {
			pins[pin_OSC2-1].set_value(((bool)data[0]) * Vdd, false);
		} else if (name == "CLKOUT") {
			pins[pin_CLKOUT-1].set_value(((bool)data[0]) * Vdd, false);
		}
	}

	void register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	}

	PINS() {
		DeviceEvent<Clock>::subscribe<PINS>(this, &PINS::clock_event);
		DeviceEvent<Register>::subscribe<PINS>(this, &PINS::register_changed);
	}

};


class PORTA: public Device {

};

class PORTB: public Device {
  public:
	void recalc_pullups(PINS &pins, bool RBPU) {}
	void rising_rb0_interrupt(PINS &pins, bool rising) {}
};


class Flash: public Device {
  public:
	WORD data[FLASH_SIZE];

	void load(const std::string &a_file);

	WORD fetch(WORD PC) {
		return data[PC % FLASH_SIZE];
	}

	void clear() {
		memset(data, 0, sizeof(data));
	}

	void set_data(WORD address, const std::string &ds) {
		for(WORD n=0; n<ds.length() && n+address<FLASH_SIZE; n += 2)
			data[(n+address)/2] = (((WORD)ds[n+1]) << 8) | (BYTE)ds[n];
	}

};

