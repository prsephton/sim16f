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

class PORTA: public Device {

};

class PORTB: public Device {

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

	static const BYTE RA2    = 1;
	static const BYTE AN2    = 1;
	static const BYTE VREF   = 1;
	static const BYTE RA3    = 2;
	static const BYTE AN3    = 2;
	static const BYTE CMP1   = 2;
	static const BYTE RA4    = 3;
	static const BYTE CMP2   = 3;
	static const BYTE TOCKI  = 3;
	static const BYTE RA5    = 4;
	static const BYTE MCLR   = 4;
	static const BYTE Vpp    = 4;
	static const BYTE Vss    = 5;
	static const BYTE RB0    = 6;
	static const BYTE INT    = 6;
	static const BYTE RB1    = 7;
	static const BYTE DT     = 7;
	static const BYTE RB2    = 8;
	static const BYTE CK     = 8;
	static const BYTE RB3    = 9;
	static const BYTE CCP1   = 9;
	static const BYTE RB4    = 10;
	static const BYTE PGM    = 10;
	static const BYTE RB5    = 11;
	static const BYTE RB6    = 12;
	static const BYTE T1OSO  = 12;
	static const BYTE T1CKI  = 12;
	static const BYTE PGC    = 12;
	static const BYTE RB7    = 13;
	static const BYTE T1OSI  = 13;
	static const BYTE PGD    = 13;
	static const BYTE Vdd    = 14;
	static const BYTE CLKOUT = 15;
	static const BYTE OSC2   = 15;
	static const BYTE RAS6   = 15;
	static const BYTE CLKIN  = 16;
	static const BYTE OSC1   = 16;
	static const BYTE RA7    = 16;
	static const BYTE RA0    = 17;
	static const BYTE AN0    = 17;
	static const BYTE RA1    = 18;
	static const BYTE AN1    = 18;

	void reset() {
		for (unsigned int i = 0; i < sizeof(pins); ++i) {
			pins[i] = 0;
		}
		while (!events.empty()) events.pop();
	}

	float get_pin(BYTE pin_no) { return pins[pin_no-1]; }
	void  set_pin(BYTE pin_no, float v) { pins[pin_no-1] = v; }
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
