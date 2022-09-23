#pragma once
#include "device_base.h"
#include "register.h"
#include "clock.h"

//___________________________________________________________________________________
class Timer0: public Device {
	bool m_assigned_to_wdt;
	bool m_falling_edge;
	bool m_use_RA4;
	bool m_ra4_signal;
	bool m_wdt_en;
	BYTE m_prescale_rate;
	WORD m_counter;
	BYTE m_timer;
	bool m_sync;
	DeviceEventQueue eq;


	void sync_timer();
	void register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data);
	void on_clock(Clock *c, const std::string &name, const std::vector<BYTE> &data);

  public:
	Timer0();
	~Timer0();

	void clock_source_select(bool a_use_RA4);
	void clock_transition(bool a_falling_edge);
	void assign_prescaler(bool a_assigned_to_wdt);
	void prescaler_rate_select(BYTE a_prescale_rate);
	bool assigned_to_wdt() const { return m_assigned_to_wdt; }
	bool falling_edge() const { return m_falling_edge; }
	bool use_RA4() const { return m_use_RA4; }
	bool WDT_en() const { return m_wdt_en; }
	BYTE ra4_signal() const {return m_ra4_signal; }
	BYTE prescale_rate() const { return m_prescale_rate; }
	WORD prescaler() const { return m_counter; };
	BYTE timer() const {return m_timer; }
};

//___________________________________________________________________________________
class Timer1: public Device {
	DeviceEventQueue eq;

	Connection m_rb6;
	Connection m_rb7;
	Connection m_fosc;
	Connection m_t1oscen;
	Tristate   m_t1osc;
	Wire       m_osc_wire;
	Schmitt    m_trigger;
	Connection m_tmr1cs;
	Mux        m_t1csmux;
	Counter    m_prescaler;
	Connection m_t1ckps0;
	Connection m_t1ckps1;
	Mux        m_scale;
	Counter    m_synch;
	Connection m_t1sync;
	Mux        m_syn_asyn;
	Connection m_tmr1on;
	AndGate    m_signal;
	Counter    m_tmr1;

	void register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data);
	void on_clock(Clock *c, const std::string &name, const std::vector<BYTE> &data);
	void on_tmr1(Connection *c, const std::string &name, const std::vector<BYTE> &data);

  public:
	Timer1();
	~Timer1();

	Connection &rb6() { return m_rb6; }
	Connection &rb7() { return m_rb7; }
	Connection &fosc() { return m_fosc; }
	Connection &t1oscen() { return m_t1oscen; }
	Tristate   &t1osc() { return m_t1osc; }
	Wire       &osc_wire() { return m_osc_wire; }
	Schmitt    &trigger() { return m_trigger; }
	Connection &tmr1cs() { return m_tmr1cs; }
	Mux        &t1csmux() { return m_t1csmux; }
	Counter    &prescaler() { return m_prescaler; }
	Connection &t1ckps0() { return m_t1ckps0; }
	Connection &t1ckps1() { return m_t1ckps1; }
	Mux        &pscale() { return m_scale; }
	Counter    &synch() { return m_synch; }
	Connection &t1sync() { return m_t1sync; }
	Mux        &syn_asyn() { return m_syn_asyn; }
	Connection &tmr1on() { return m_tmr1on; }
	AndGate    &signal() { return m_signal; }
	Counter    &tmr1() { return m_tmr1; }
};

//___________________________________________________________________________________
class Timer2: public Device {

};

