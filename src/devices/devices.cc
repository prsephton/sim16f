#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include "devices.h"
#include "../utils/smart_ptr.cc"

//_______________________________________________________________________________________________
// Timer 0


//_______________________________________________________________________________________________
// Comparator

	void Comparator::queue_change(BYTE old_cmcon) {
		if (cmcon != old_cmcon) {
			if (debug()) {
				std::cout << "Mode=" << (int)(cmcon & 7) << ((cmcon & Flags::CMCON::CIS)?"c":"") << ": inputs=[";
				for (int n=0; n<4; ++n) {
					std::cout << (n>0?", ":"") << inputs[n];
				}
				std::cout << "]  Calculated C1OUT=" << ((cmcon & Flags::CMCON::C1OUT)?"true":"false");
				std::cout << ", C2OUT=" << ((cmcon & Flags::CMCON::C2OUT)?"true":"false") << std::endl;
			}
			eq.queue_event(new DeviceEvent<Comparator>(*this, "Comparator Change", {cmcon, old_cmcon, (BYTE)(old_cmcon ^ cmcon)}));
			eq.process_events();
		}
	}

	void Comparator::recalc() {
		float c1_ref = inputs[0];
		float c1_vin = inputs[3];
		float c2_ref = inputs[1];
		float c2_vin = inputs[2];

		switch (mode()) {
		case 0:              // Comparators Reset
			c1_vin = c1_ref;
			c2_vin = c2_ref;
			break;
		case 1:       // Three Inputs Multiplexed to Two Comparators
			c1_ref = (cmcon & Flags::CMCON::CIS)?inputs[3]:inputs[0];
			c1_vin = c2_vin;
			break;
		case 2:
			c1_ref = (cmcon & Flags::CMCON::CIS)?inputs[3]:inputs[0];
			c2_ref = (cmcon & Flags::CMCON::CIS)?inputs[2]:inputs[1];
			c1_vin = c2_vin = vref;
			break;
		case 3:                 // Two Common Reference Comparators
			c1_vin = c2_vin;
			break;
		case 4:                 // Default 2 independent comparators
			break;
		case 5:                 // turn off c1
			c1_vin = c1_ref = 0;
			break;
		case 6:                 // Two Common Reference Comparators with Outputs
			c1_vin = c2_vin;
			break;
		case 7:                 // Comparators Off
			c1_vin = c1_ref = 0;
			c2_vin = c2_ref = 0;
		}
		bool c1_compare = c1_vin > c1_ref;
		bool c2_compare = c2_vin > c2_ref;

		if (cmcon & Flags::CMCON::C1INV) c1_compare = !c1_compare;
		if (cmcon & Flags::CMCON::C2INV) c2_compare = !c2_compare;

		if (c1_compare) cmcon = cmcon | Flags::CMCON::C1OUT; else cmcon = cmcon & ~Flags::CMCON::C1OUT;
		if (c2_compare) cmcon = cmcon | Flags::CMCON::C2OUT; else cmcon = cmcon & ~Flags::CMCON::C2OUT;

		if (c1_vin == c1_ref) c1.set_value(0, true); else c1.set_value(c1_compare*Vdd, mode()==6);
		if (c2_vin == c2_ref) c2.set_value(0, true); else c2.set_value(c2_compare*Vdd, mode()==6);

//		std::cout << (c1_compare?"c1=true":"c1=false") << "  " << (c2_compare?"c2=true":"c2=false") << std::endl;
	}

	void Comparator::on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		if (name=="CMCON"){   // We will be changing CMCON::C1OUT and CMCON::C2OUT in recalc()
			BYTE old_cmcon = cmcon;
			cmcon = data[Register::DVALUE::NEW];
			recalc();
			queue_change(old_cmcon);
		}
	}

	void Comparator::on_connection_change(Connection *c, const std::string &name, const std::vector<BYTE> &data) {
		if        (c->name() == "RA0::Comparator") {
			inputs[0] = c->rd(); recalc();
		} else if (c->name() == "RA1::Comparator") {
			inputs[1] = c->rd(); recalc();
		} else if (c->name() == "RA2::Comparator") {
			inputs[2] = c->rd(); recalc();
		} else if (c->name() == "RA3::Comparator") {
			inputs[3] = c->rd(); recalc();
		} else if (c->name() == "VREF") {
			vref = c->rd(); recalc();
		} else if (std::string(":Comparator1:Comparator2:").find(c->name()) != std::string::npos){
			BYTE old_cmcon = cmcon;
			if (c->name() == c1.name()) {
				if (c1.signal() && !(cmcon & Flags::CMCON::C1OUT)) {
					cmcon = cmcon | Flags::CMCON::C1OUT;
				} else if (!c1.signal() && (cmcon & Flags::CMCON::C1OUT)) {
					cmcon = cmcon & ~Flags::CMCON::C1OUT;
				}
			} else if (c->name() == c2.name()) {
				if (c2.signal() && !(cmcon & Flags::CMCON::C2OUT)) {
					cmcon = cmcon | Flags::CMCON::C2OUT;
				} else if (!c2.signal() && (cmcon & Flags::CMCON::C2OUT)) {
					cmcon = cmcon & ~Flags::CMCON::C2OUT;
				}
			}
			queue_change(old_cmcon);
		}
	}

	Comparator::Comparator(): Device()  {
		c1.name("Comparator1");
		c2.name("Comparator2");
		cmcon = 0;
		DeviceEvent<Connection>::subscribe<Comparator>(this, &Comparator::on_connection_change);
		DeviceEvent<Register>::subscribe<Comparator>(this, &Comparator::on_register_change);
	}

	Comparator::~Comparator()  {
		DeviceEvent<Connection>::unsubscribe<Comparator>(this, &Comparator::on_connection_change);
		DeviceEvent<Register>::unsubscribe<Comparator>(this, &Comparator::on_register_change);
	}





//_______________________________________________________________________________________________
// EEPROM
void EEPROM::load(const std::string &a_file) {
	clear();
	int fd = open(a_file.c_str(), O_RDONLY);
	int c = read(fd, data, sizeof(data));
	if (c < 0)
		throw(std::string("Cannot read EEPROM data from file: ") + a_file);
}
//_______________________________________________________________________________________________
// Clock
void Clock::stop() { stopped=true; phase=0; high=false;}
void Clock::start() { stopped=false; }
void Clock::toggle() {
	if (stopped) return;
	high = !high;
	if (high) {
		phase = phase % 4; ++phase;
	}

//	std::cout << "toggle clock; high:" << high << "; phase:" << (int)phase << "\n";
	DeviceEventQueue eq;
	eq.queue_event(new DeviceEvent<Clock>(*this, "oscillator", {(BYTE)high}));

	Q1 = phase == 1; if (Q1 and high) eq.queue_event(new DeviceEvent<Clock>(*this, "Q1"));
	Q2 = phase == 2; if (Q2 and high) eq.queue_event(new DeviceEvent<Clock>(*this, "Q2"));
	Q3 = phase == 3; if (Q3 and high) eq.queue_event(new DeviceEvent<Clock>(*this, "Q3"));
	Q4 = phase == 4; if (Q4 and high) eq.queue_event(new DeviceEvent<Clock>(*this, "Q4"));

	if (phase % 2)
		eq.queue_event(new DeviceEvent<Clock>(*this, "CLKOUT", {(BYTE)(phase/2?1:0)}));

	if (high and Q1) {
		eq.queue_event(new DeviceEvent<Clock>(*this, "cycle"));
	}
}


//_______________________________________________________________________________________________
// Flash
void Flash::load(const std::string &a_file) {
	clear();
	int fd = open(a_file.c_str(), O_RDONLY);
	int c = read(fd, data, sizeof(data));
	if (c < 0)
		throw(std::string("Cannot read flash data from file: ") + a_file);
}


//_______________________________________________________________________________________________
//  Event queue static definitions
bool DeviceEventQueue::debug = false;
std::mutex DeviceEventQueue::mtx;
std::queue< SmartPtr<QueueableEvent> >DeviceEventQueue::events;

template <class Clock> class
	DeviceEvent<Clock>::registry  DeviceEvent<Clock>::subscribers;

