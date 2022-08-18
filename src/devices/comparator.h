#pragma once

#include "device_base.h"
#include "sram.h"
#include "flags.h"
#include "register.h"

//___________________________________________________________________________________
class Comparator: public Device {   // the comparator module
	float inputs[4] = {0,0,0,0};    // Inputs from RA0 -> RA3
	float vref = 0;                 // the last value of VREF
	BYTE  cmcon = 0;                // the last value of CMCON
	DeviceEventQueue eq;            // fires Comparator change events
	Connection c1, c2;              // outputs named Comparator1 and Comparator2

	BYTE mode() const { return cmcon & 7; }  // mode 0..7

	void queue_change(BYTE old_cmcon);
	void recalc();                  // recalculate c1, c2 and CMCON

	// look for CMCON changes and recalculate
	void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data);
	// look for input changes and recalculate
	void on_connection_change(Connection *c, const std::string &name, const std::vector<BYTE> &data);

  public:
	struct DVALUE {
		static const int NEW      = 0;
		static const int OLD      = 1;
		static const int CHANGED  = 2;
	};

	Comparator();
	~Comparator();
};

