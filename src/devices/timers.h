#include "device_base.h"
#include "register.h"
#include "clock.h"

//___________________________________________________________________________________
class Timer0: public Device {
	bool assigned_to_wdt;
	bool falling_edge;
	bool use_RA4;
	BYTE prescale_rate;
	WORD counter;
	BYTE timer;
	DeviceEventQueue eq;

	void register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		if (name=="TMR0"){    // a write to TMR0
			prescale_rate = 1;
			timer = data[Register::DVALUE::NEW];
		} else if (name=="OPTION"){
			BYTE changed = data[Register::DVALUE::CHANGED];
			BYTE new_value = data[Register::DVALUE::NEW];

			if (changed & Flags::OPTION::T0CS) {
				clock_source_select(new_value & Flags::OPTION::T0CS);
			}
			if (changed & Flags::OPTION::T0SE) {
				clock_transition(new_value & Flags::OPTION::T0SE);
			}
			if (changed & Flags::OPTION::PSA) {
				prescaler(new_value & Flags::OPTION::PSA);
			}
			if (changed & (Flags::OPTION::PS0 | Flags::OPTION::PS1 | Flags::OPTION::PS2)) {
				prescaler_rate_select(new_value & 0x7);
			}
		}
	}

	void on_clock(Clock *c, const std::string &name, const std::vector<BYTE> &data) {
		if (name == "CLKOUT") {
			if (data[0] ^ falling_edge)
				++counter;
			if (counter == prescale_rate) {
				if (++timer == 0) {   // overflow from FF to 0
					eq.queue_event(new DeviceEvent<Timer0>(*this, "Overflow", {}));
				}
				counter = 0;
			}
		}
	}

  public:
	Timer0(): Device("TMR0"), assigned_to_wdt(false), falling_edge(false), use_RA4(false),
				prescale_rate(1), counter(0), timer(0) {
		DeviceEvent<Register>::subscribe<Timer0>(this, &Timer0::register_changed);
		DeviceEvent<Clock>::subscribe<Timer0>(this, &Timer0::on_clock);
	}
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
		if (use_RA4) // counter mode
			prescale_rate = 1;
		else if (assigned_to_wdt)
			prescale_rate = 1 << (a_prescale_rate);
		else  // timer mode
			prescale_rate = 1 << (a_prescale_rate+1);
	};
};

class Timer1: public Device {

};

class Timer2: public Device {

};

