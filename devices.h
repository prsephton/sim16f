#ifndef __devices_h__
#define __devices_h__

#include <queue>
#include <string>

#include "constants.h"


//__________________________________________________________________________________________________
// Device definitions
class Device {
};

class Comparator: public Device {

};

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
	short data[EEPROM_SIZE];

	void load(const std::string &a_file);

	void set_data(WORD address, const std::string &ds) {
		for(WORD n=0; n<ds.length() && n+address<EEPROM_SIZE; ++n)
			data[n+address] = ds[n];
	}
};

class PINS: public Device {

	float pins[PIN_COUNT];

  public:
	class Event {
  	  public:
		BYTE pin;
		std::string name;
		float old_value;
		float new_value;

		Event(BYTE a_pin, std::string a_name, float a_new=0.0, float a_old=0.0)
			: pin(a_pin), name(a_name), old_value(a_old), new_value(a_new) {}
	};
	std::queue<Event> events;

	static const BYTE RA2    = 1;    // Analog comparator input
	static const BYTE AN2    = 1;    // Bidirectional I/O port
	static const BYTE VREF   = 1;    // VREF output
	static const BYTE RA3    = 2;    // Bidirectional I/O port
	static const BYTE AN3    = 2;    // Analog comparator input
	static const BYTE CMP1   = 2;    // Comparator 1 output
	static const BYTE RA4    = 3;    // Bidirectional I/O port
	static const BYTE CMP2   = 3;    // Comparator 2 output
	static const BYTE TOCKI  = 3;    // Timer0 clock input
	static const BYTE RA5    = 4;    // Input port
	static const BYTE MCLR   = 4;    // Master clear. When configured as MCLR, this pin is an active low Reset to the device.
	static const BYTE Vpp    = 4;    // Programming voltage input
	static const BYTE Vss    = 5;    // ground
	static const BYTE RB0    = 6;    // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE INT    = 6;    // External interrupt
	static const BYTE RB1    = 7;    // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE DT     = 7;    // Synchronous data I/O
	static const BYTE RB2    = 8;    // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE CK     = 8;    // Synchronous clock I/O
	static const BYTE RB3    = 9;    // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE CCP1   = 9;    // Capture/Compare/PWM I/O
	static const BYTE RB4    = 10;   // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE PGM    = 10;   // Low-voltage programming input pin.
	static const BYTE RB5    = 11;   // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE RB6    = 12;   // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE T1OSO  = 12;   // Timer1 oscillator output
	static const BYTE T1CKI  = 12;   // Timer1 clock input
	static const BYTE PGC    = 12;   // ICSP™ programming clock
	static const BYTE RB7    = 13;   // Bidirectional I/O port. Can be software programmed for internal weak pull-up.
	static const BYTE T1OSI  = 13;   // Timer1 oscillator input
	static const BYTE PGD    = 13;   // ICSP data I/O
	static const BYTE Vdd    = 14;   // positive supply
	static const BYTE CLKOUT = 15;   // CLKOUT, which has 1/4 the frequency of OSC1.
	static const BYTE OSC2   = 15;   // Oscillator crystal output. Connects to crystal or resonator in Crystal Oscillator mode.
	static const BYTE RAS6   = 15;   // Bidirectional I/O port
	static const BYTE CLKIN  = 16;   // External clock source input. RC biasing pin.
	static const BYTE OSC1   = 16;   // Oscillator crystal input.
	static const BYTE RA7    = 16;   // Bidirectional I/O port
	static const BYTE RA0    = 17;   // Bidirectional I/O port
	static const BYTE AN0    = 17;   // Analog comparator input
	static const BYTE RA1    = 18;   // Bidirectional I/O port
	static const BYTE AN1    = 18;   // Analog comparator input


	// These are the 'external signals' we configure this device to show for inputs or outputs.
	float get_pin(BYTE pin_no) { return pins[pin_no-1]; }
	void  set_pin(BYTE pin_no, bool high) { pins[pin_no-1] = high?5.0:0.0; }

	void reset() {
		for (unsigned int i = 0; i < sizeof(pins); ++i) {
			pins[i] = 0;
		}
		while (!events.empty()) events.pop();
		set_pin(Vss, (float)0.0);
		set_pin(Vdd, (float)5.0);
	}
};

class PORTA: public Device {

};

class PORTB: public Device {
  public:
	void recalc_pullups(PINS &pins, bool RBPU) {}
	void rising_rb0_interrupt(PINS &pins, bool rising) {}
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

	void toggle(PINS &pins);
};


class Flash {
  public:
	WORD data[FLASH_SIZE];

	void load(const std::string &a_file);

	WORD fetch(WORD PC) {
		return data[PC % FLASH_SIZE];
	}

	void set_data(WORD address, const std::string &ds) {
		for(WORD n=0; n<ds.length() && n+address<FLASH_SIZE; n += 2)
			data[(n+address)/2] = (((WORD)ds[n+1]) << 8) | (BYTE)ds[n];
	}

};

#endif
