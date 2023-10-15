#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cassert>
#include "../utils/smart_ptr.cc"
#include "core_devices.h"

template <class T> class
	DeviceEvent<T>::registry  DeviceEvent<T>::subscribers;

//_______________________________________________________________________________________________
// Timer 0
	void Timer0::sync_timer() {   // timer increments with every second call
		++m_counter;
		if (m_assigned_to_wdt || (m_counter & (1 << m_prescale_rate))) {
			m_sync = !m_sync;
			if (m_sync) {
				if (++m_timer == 0) {   // overflow from FF to 0
					eq.queue_event(new DeviceEvent<Timer0>(*this, "Overflow", {}));
				} else {
					eq.queue_event(new DeviceEvent<Timer0>(*this, "Value", {m_timer})); // update sram & register
				}
			} else {
				eq.queue_event(new DeviceEvent<Timer0>(*this, "Sync", {}));
			}
		} else {
			eq.queue_event(new DeviceEvent<Timer0>(*this, "Sync", {}));
		}
	}

	void Timer0::register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		if (name=="TMR0"){    // a write to TMR0
			m_counter = 0;
			m_timer = data[Register::DVALUE::NEW];
			eq.queue_event(new DeviceEvent<Timer0>(*this, "Reset", {data[Register::DVALUE::NEW]}));
		} else if (name=="CONFIG1"){
			m_wdt_en = data[Register::DVALUE::NEW] & Flags::CONFIG::WDTE;
		} else if (name=="INTCON"){
			BYTE new_value = data[Register::DVALUE::NEW];
			eq.queue_event(new DeviceEvent<Timer0>(*this, "INTCON", {new_value}));
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
				assign_prescaler(new_value & Flags::OPTION::PSA);
			}
			if (changed & (Flags::OPTION::PS0 | Flags::OPTION::PS1 | Flags::OPTION::PS2)) {
				prescaler_rate_select(new_value & 0x7);
			}
		} else if (name=="PORTA"){
			if (m_use_RA4) {
				bool signal = (data[Register::DVALUE::NEW] & Flags::PORTA::RA4) != 0;
				if (signal != m_ra4_signal) {
					if ((signal ^ m_falling_edge)) {
						sync_timer();
					}
					m_ra4_signal = signal;
				}
			}
		}
	}

	void Timer0::on_clock(Clock *c, const std::string &name, const std::vector<BYTE> &data) {
		if (name == "CLKOUT") {
			if (not m_use_RA4 && data[0]) {    // rising edge of clock signal
				sync_timer();
			}
		}
	}

	Timer0::Timer0(): Device("TMR0"),
		m_assigned_to_wdt(false), m_falling_edge(false), m_use_RA4(false),
		m_ra4_signal(false), m_wdt_en(false), m_prescale_rate(1), m_counter(0), m_timer(0), m_sync(false)
	{
		DeviceEvent<Register>::subscribe<Timer0>(this, &Timer0::register_changed);
		DeviceEvent<Clock>::subscribe<Timer0>(this, &Timer0::on_clock);
	}
	Timer0::~Timer0() {
		DeviceEvent<Register>::unsubscribe<Timer0>(this, &Timer0::register_changed);
		DeviceEvent<Clock>::unsubscribe<Timer0>(this, &Timer0::on_clock);
	}
	void Timer0::clock_source_select(bool a_use_RA4){
		m_use_RA4 = a_use_RA4;
	};
	void Timer0::clock_transition(bool a_falling_edge){
		m_falling_edge = a_falling_edge;
	};
	void Timer0::assign_prescaler(bool a_assigned_to_wdt){
		m_assigned_to_wdt = a_assigned_to_wdt;
	};
	void Timer0::prescaler_rate_select(BYTE a_prescale_rate){
		// bits   000   001   010   011   100    101    110     111
		// TMR0   1:2   1:4   1:8   1:16  1:32   1:64   1:128   1:256
		// WDT    1:1   1:2   1:4	1:8   1:16   1:32   1:64    1:128
		assert((a_prescale_rate >= 0) && (a_prescale_rate < 8));
		m_prescale_rate = a_prescale_rate;
		m_prescale_rate = 0;
	};


	//_______________________________________________________________________________________________
	// Timer1
	void Timer1::register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		if (name == "PORTB") {
			m_rb6.set_value((r->get_value() & 0b0100000)?Vdd:Vss, false);
			m_rb7.set_value((r->get_value() & 0b1000000)?Vdd:Vss, false);
		} else if (name == "T1CON") {
			BYTE d = r->get_value();
			m_t1oscen.set_value((d & Flags::T1CON::T1OSCEN)?Vdd:Vss, false);
			m_tmr1cs.set_value((d & Flags::T1CON::TMR1CS)?Vdd:Vss, false);
			m_t1sync.set_value((d & Flags::T1CON::T1SYNC)?Vdd:Vss, false);
			m_tmr1on.set_value((d & Flags::T1CON::TMR1ON)?Vdd:Vss, false);
			m_t1ckps0.set_value((d & Flags::T1CON::T1CKPS0)?Vdd:Vss, false);
			m_t1ckps1.set_value((d & Flags::T1CON::T1CKPS1)?Vdd:Vss, false);
		} else if (name == "TMR1L") {
			m_prescaler.set_value(0);
			m_tmr1.set_value((m_tmr1.get() & ~0xff) | data[0]);
		} else if (name == "TMR1H") {
			m_prescaler.set_value(0);
			m_tmr1.set_value((m_tmr1.get() & ~0xff00) | ((int)data[0] << 8));
		}
	}

	void Timer1::on_clock(Clock *c, const std::string &name, const std::vector<BYTE> &data) {
		if (name == "CLKOUT") {
			m_fosc.set_value(data[0] * Vdd, false);
		}
	}

	void Timer1::on_tmr1(Connection *c, const std::string &name, const std::vector<BYTE> &data) {
		if (m_tmr1.overflow().signal()) {  // check overflow
			eq.queue_event(new DeviceEvent<Timer1>(*this, "Overflow", {}));
		} else if (m_tmr1on.signal()) {
			unsigned long val = m_tmr1.get();
			BYTE LO = (BYTE)(val & 0xff);
			BYTE HI = (BYTE)((val >> 8) & 0xff);
			eq.queue_event(new DeviceEvent<Timer1>(*this, "Value", {LO, HI}));
		}
	}

	Timer1::Timer1(): Device(),
		m_t1osc(m_rb7, m_t1oscen, false, true, "T1OSC"),
		m_osc_wire(m_rb7, m_t1osc.rd()),
		m_trigger(m_rb6, false, false),
		m_t1csmux({&m_fosc, &m_trigger.rd()}, {&m_tmr1cs}, "T1CS"),
		m_prescaler(m_t1csmux.rd(), false, 4),
		m_scale(m_prescaler.databits(), {&m_t1ckps0, &m_t1ckps1}, "Scale"),
		m_synch(m_scale.rd(), true, 1, 0, &m_fosc),
		m_syn_asyn({&m_synch.bit(0), &m_scale.rd()}, {&m_t1sync}, "T1Sync"),
		m_signal({&m_syn_asyn.rd(), &m_tmr1on}, false, "Timer ON"),
		m_tmr1(m_signal.rd(), false, 16)
	{
		DeviceEvent<Register>::subscribe<Timer1>(this, &Timer1::register_changed);
		DeviceEvent<Clock>::subscribe<Timer1>(this, &Timer1::on_clock);
		DeviceEvent<Connection>::subscribe<Timer1>(this, &Timer1::on_tmr1, &m_tmr1.bit(0));
		m_rb6.name("RB6");
		m_rb7.name("RB7");
		m_fosc.name("Fosc/4");
		m_scale.rd().name("Scale");
		m_synch.bit(0).name("Sync");

		m_t1oscen.set_value(Vss, false);
		m_tmr1cs.set_value(Vss, false);
		m_t1sync.set_value(Vss, false);
		m_tmr1on.set_value(Vss, false);
		m_t1ckps0.set_value(Vss, false);
		m_t1ckps1.set_value(Vss, false);

	}

	Timer1::~Timer1() {
		DeviceEvent<Register>::unsubscribe<Timer1>(this, &Timer1::register_changed);
		DeviceEvent<Clock>::unsubscribe<Timer1>(this, &Timer1::on_clock);
		DeviceEvent<Connection>::unsubscribe<Timer1>(this, &Timer1::on_tmr1, &m_tmr1.bit(0));
	}

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
	Simulation::clock().set_value(high*Vdd, false);

	Q1 = phase == 1; if (Q1 && high) eq.queue_event(new DeviceEvent<Clock>(*this, "Q1"));
	Q2 = phase == 2; if (Q2 && high) eq.queue_event(new DeviceEvent<Clock>(*this, "Q2"));
	Q3 = phase == 3; if (Q3 && high) eq.queue_event(new DeviceEvent<Clock>(*this, "Q3"));
	Q4 = phase == 4; if (Q4 && high) eq.queue_event(new DeviceEvent<Clock>(*this, "Q4"));

	if (phase % 2)
		eq.queue_event(new DeviceEvent<Clock>(*this, "CLKOUT", {(BYTE)(phase/2?0:1)}));

	if (high && Q1) {
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


