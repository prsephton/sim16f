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

class Timer1: public Device {

};

class Timer2: public Device {

};

