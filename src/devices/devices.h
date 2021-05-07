#pragma once

#include "device_base.h"
#include "sram.h"
#include "flags.h"
#include "../utils/utility.h"

//___________________________________________________________________________________
// A file register is a memory location having special significance.
class Register : public Device {
	WORD m_idx;
	std::string m_name;
	std::string m_doc;

  public:
	DeviceEventQueue eq;

	Register(const WORD a_idx, const std::string &a_name, const std::string &a_doc = "")
  	  : m_idx(a_idx), m_name(a_name), m_doc(a_doc) {
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
		if (changed) {
			eq.queue_event(new DeviceEvent<Register>(*this, m_name, {old, changed, value}));
		}
		a_sram.write(m_idx, value);
	}
};


//___________________________________________________________________________________
//   A basic port implements a Port latch and Tris latch having high impedence inputs
// directly from the data bus.
//   To set these latches, a write signal is pulsed for either Port or Tris latches,
// and the data value gets recorded on the clock signals falling edge.
//   The wiring for various ports differ between the latches and the actual port, but
// commonly we see a Tristate buffer being fed from the Q output of the Port latch, and
// controlled by an inverted Q signal from the Tris latch.  This means that the pin
// signal is equal to the PortLatch.Q if the TrisLatch.Q is low, but the Tristate outpout
// is set to a high impedence when TrisLatch.Q is high.
//   The voltage on the pin, whether as a result of a signal from PortLatch.Q or an
// external signal, is fed into a Schmitt trigger, and from there into an input latch.
//   To read the TrisLatch value, we can raise a control signal on an inverted Tristate
// buffer which is connected to TrisLatch.Qc.  The signal is then output to the data bus.
// A similar strategy is employed for reading data from the InputLatch.Q.

class BasicPort: public Device {
	std::map<std::string, SmartPtr<Device> > m_components;
protected:
	Connection &Pin;
	Connection Data;    // This is the data bus value
	Connection Port;    // write port
	Connection Tris;    // write tris
	Connection rdPort;  // read port
	Connection rdTris;  // read tris
	Connection S1;
	Connection S1_en;
	bool       porta_select;  // false is portb
	BYTE 	   port_mask;
	DeviceEventQueue eq;

	void process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		auto c = components();  // simulates a data bus write operation to either port or tris
		if ((porta_select && name == "PORTA") || (!porta_select && name == "PORTB")) {
			bool input = ((data[2] & port_mask) == port_mask);
			Data.set_value(input * Vdd, true);    // set the input value on the bus.
			eq.process_events();            // basically propagate all events, and their events etc.
			Port.set_value(Vdd, true);
			eq.process_events();
			Port.set_value(Vss, true);      // latch the value from the bus into Q
			eq.process_events();
		} else if ((porta_select && name == "TRISA") || (!porta_select && name == "TRISB")) {
			bool input = ((data[2] & port_mask) == port_mask);
			Data.set_value(input * Vdd, true);
			eq.process_events();
			Tris.set_value(Vdd, true);
			eq.process_events();
			Tris.set_value(Vss, true); // clock the signal
			eq.process_events();
			Data.set_value(Data.rd(), true);    // impeded.
		}
	}

	virtual void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		process_register_change(r, name, data);
	}



public:
	BasicPort(Connection &a_Pin, const std::string &a_name, int port_no, int port_bit_ofs):
		Device(a_name), Pin(a_Pin), porta_select(port_no==0), port_mask(1 << port_bit_ofs)
	{
		Wire *DataBus = new Wire(a_name+"::data");
		Wire *PinWire = new Wire(a_name+"::pin");

		Latch *DataLatch = new Latch(Data, Port); DataLatch->set_name(a_name+"::DataLatch");
		Latch *TrisLatch = new Latch(Data, Tris); TrisLatch->set_name(a_name+"::TrisLatch");

		PinWire->connect(Pin);
		Schmitt *trigger = new Schmitt(S1, S1_en);

		Inverter *NotPort = new Inverter(Port);
		Latch *SR1 = new Latch(trigger->rd(), NotPort->rd(), true, false); SR1->set_name(a_name+"::SR1");
		Tristate *Tristate2 = new Tristate(SR1->Q(), rdPort);  Tristate2->name(a_name+"::TS2");
		DataBus->connect(Tristate2->rd());

		Tristate *Tristate3 = new Tristate(TrisLatch->Qc(), rdTris, false, true);  Tristate3->name(a_name+"::TS3");
		DataBus->connect(Tristate3->rd());

		m_components["Data Bus"] = DataBus;
		m_components["Pin Wire"] = PinWire;
		m_components["Data Latch"] = DataLatch;
		m_components["Tris Latch"] = TrisLatch;

		m_components["Inverter1"] = NotPort;
		m_components["Tristate2"] = Tristate2;
		m_components["Tristate3"] = Tristate3;
		m_components["Schmitt Trigger"] = trigger;
		m_components["SR1"] = SR1;

		DeviceEvent<Register>::subscribe<BasicPort>(this, &BasicPort::on_register_change);
	}
	Wire &bus_line() { return dynamic_cast<Wire &>(*m_components["Data Bus"]); }
	Connection &pin() { return Pin; }
	std::map<std::string, SmartPtr<Device> > &components() { return m_components; }
	Connection &read_port() {           // read the port
		auto c = components();
		rdPort.set_value(Vdd, true);    // raise a read signal on the port read control line
		eq.process_events();            // basically propagate all events, and their events etc.
		rdPort.set_value(Vss, true);    // lower the read singal.  The result is on the bus.
		return Data;
	}
	Connection &read_tris() {           // read the tris latch value
		auto c = components();
		rdTris.set_value(Vdd, true);    // raise a read signal on the port read control line
		eq.process_events();            // basically propagate all events, and their events etc.
		rdTris.set_value(Vss, true);    // lower the read singal.  The result is on the bus.
		return Data;
	}
};


//___________________________________________________________________________________
//  A model for a most ports, which have a Tristate connected to the DataLatch
// and TrisLatch, and clamps the port range.
class SinglePort: public BasicPort {

  public:
	SinglePort(Connection &a_Pin, const std::string &a_name, int port_no, int port_bit_ofs):
		BasicPort(a_Pin, a_name, port_no, port_bit_ofs)
	{
		auto &c = components();
		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);

		Tristate *Tristate1 = new Tristate(DataLatch.Q(), TrisLatch.Q(), true); Tristate1->name(a_name+"::TS1");
		Clamp * PinClamp = new Clamp(Pin);
		c["Tristate1"] = Tristate1;
		c["PinClamp"] = PinClamp;

		PinWire.connect(Tristate1->rd());
	}
};

//___________________________________________________________________________________
//  A model for a single port for pins RA0/AN0, RA1/AN1
//  These are standard ports, but with a comparator output
class SinglePortA_Analog: public SinglePort {

  protected:
	Connection Comparator;

	void set_comparator(bool on) {
		if (on) {
			S1_en.set_value(Vss, true);   // schmitt trigger enable active low
			Comparator.set_value(Comparator.rd(), false);    // low impedence
		} else {
			S1_en.set_value(Vdd, true);
			Comparator.set_value(Comparator.rd(), true);
		}
	}

	void set_comparators_for_an0_and_an1(BYTE cmcon) {
		switch (cmcon & 0b111) {
		case 0b000 :    // Comparators reset
			S1_en.set_value(Vss, true);
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

	virtual void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		if        (name == "CMCON") {
			BYTE cmcon = data[2];
			set_comparators_for_an0_and_an1(cmcon);
		} else {
			process_register_change(r, name, data);  // handles TRIS/PORT changes
		}
	}

  public:
	SinglePortA_Analog(Connection &a_Pin, const std::string &a_name):
		SinglePort(a_Pin, a_name, 0, a_name=="RA0"?0:a_name=="RA1"?1:a_name=="RA3"?2:a_name=="RA4"?3:0),
		Comparator(Vss, true, a_name+"::Comparator")
	{
		DeviceEvent<Register>::subscribe<SinglePortA_Analog>(this, &SinglePortA_Analog::on_register_change);
	}
	Connection &comparator() { return Comparator; }
};

//___________________________________________________________________________________
//  A model for a single port for pin AN2.  This looks like AN0/AN1 except that
//  it also has a voltage reference.
class SinglePortA_Analog_RA2: public  SinglePortA_Analog {
	Connection VRef;

	virtual void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		if        (name == "CMCON") {
			BYTE cmcon = data[2];

			switch (cmcon & 0b111) {
			case 0b000 :    // Comparators reset
				S1_en.set_value(Vss, true);
				Comparator.set_value(Vss, false);
				break;
			case 0b010 :    // 4 inputs Multiplexed 2 Comparators
				set_comparator((cmcon & Flags::CMCON::CIS) != 0);
				break;
			case 0b101 :    // One independent comparator
			case 0b011 :    // 2 common reference comparators
			case 0b100 :    // Two independent comparators
			case 0b001 :    // 3 inputs Multiplexed 2 Comparators
			case 0b110 :    // Two common reference comparators with outputs
				set_comparator(true);
				break;
			case 0b111 :    // Comparators off
				set_comparator(false);
				break;
			}
		} else if (name == "VRCON") {
			BYTE vrcon = data[2];
			bool vren = (vrcon & Flags::VRCON::VREN) == Flags::VRCON::VREN;  // VRef enable
			bool vroe = (vrcon & Flags::VRCON::VROE) == Flags::VRCON::VROE;  // VRef output enable
			bool vrr  = (vrcon & Flags::VRCON::VRR)  == Flags::VRCON::VRR;   // VRef Range
			float vref =  0;
			if (vren) {
				if (vrr) {
					vref = ((vrcon & 0b111) / 24.0) * Vdd;
				} else {
					vref = ((vrcon & 0b111) / 32.0) * Vdd + Vdd/4;
				}
			}
			VRef.set_value(vref, !vroe);
		} else {
			process_register_change(r, name, data);  // handles TRIS/PORT changes
		}
	}

  public:
	SinglePortA_Analog_RA2(Connection &a_Pin, const std::string &a_name) :
		SinglePortA_Analog(a_Pin, a_name), VRef((float)0.35, true)
	{
		auto c = components();
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
		PinWire.connect(VRef);
	}
};

//___________________________________________________________________________________
//  A model for a single port for pin AN3.
//
// It differs from a standard port by the introduction of a Mux between the data latch
// and the tristate ouput.
//
// The mux selects a comparator output if the CMCON register has a comparator
// mode of 0b110, otherwise the Q output of the data latch is selected

class SinglePortA_Analog_RA3: public  BasicPort {
	Connection &m_comparator_out;
	Connection m_cmp_mode_sw;
	Connection Comparator;

	void set_comparator(bool on) {
		if (on) {
			S1_en.set_value(Vss, true);   // schmitt trigger enable active low
			Comparator.set_value(Comparator.rd(), false);    // low impedence
		} else {
			S1_en.set_value(Vdd, true);
			Comparator.set_value(Comparator.rd(), true);
		}
	}

	virtual void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		if        (name == "CMCON") {
			BYTE cmcon = data[2];
			bool comparator_mode_switch = false;

			switch (cmcon & 0b111) {
			case 0b000 :    // Comparators reset
				S1_en.set_value(Vss, true);
				Comparator.set_value(Vss, false);
				break;
			case 0b010 :    // 4 inputs Multiplexed 2 Comparators
				set_comparator((cmcon & Flags::CMCON::CIS) != 0);
				break;
			case 0b110 :    // Two common reference comparators with outputs
				comparator_mode_switch = true;
				/* no break */
			case 0b100 :    // Two independent comparators
				set_comparator(true);
				break;
			case 0b101 :    // One independent comparator
			case 0b011 :    // 2 common reference comparators
			case 0b001 :    // 3 inputs Multiplexed 2 Comparators
			case 0b111 :    // Comparators off
				set_comparator(false);
				break;
			}
			m_cmp_mode_sw.set_value(comparator_mode_switch * Vdd, true);
		} else {
			process_register_change(r, name, data);  // handles TRIS/PORT changes
		}
	}

public:
	SinglePortA_Analog_RA3(Connection &a_Pin, Connection &comparator_out, const std::string &a_name) :
		BasicPort(a_Pin, a_name, 0, 3), m_comparator_out(comparator_out)
	{
		auto c = components();
		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Mux *mux = new Mux({&DataLatch.Q(), &m_comparator_out}, {&m_cmp_mode_sw});
		c["Mux"] = mux;  // remember our mux component
		Tristate *Tristate1 = new Tristate(mux->rd(), TrisLatch.Q(), true);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
		PinWire.connect(Tristate1->rd());
		c["Tristate1"] = Tristate1;    // smart pointer should discard old Tristate1
		PinWire.connect(Comparator);
	}
	Connection &comparator() { return Comparator; }
};

//___________________________________________________________________________________
//  A model for a single port for pin RA4/AN4.
//
// This is a weird port.  It's a bit like RA3 in that it has a mux that decides
// whether to use DataLatch.Q or the output of a comparator, but instead of
// feeding into a tristate, the mux feeds into a NOR gate, which in turn feeds
// into the gate of an n-FET. The source for the n-FET is connected to Vss,
// and the drain directly to the pin, which is protected against negative voltage
// by a diode.
// Unlike other ports, the Schmitt trigger is always connected, and its output
// serves the TMR0 clock input.
//
// We can derive from BasicPort, since that has no Tristate connected to DataLatch,
// and no pin clamp.

class SinglePortA_Analog_RA4: public BasicPort {
	Connection &m_comparator_out;
	Connection m_cmp_mode_sw;

	void process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		if (name == "PORTA" || name == "PORTB") {
			bool input = ((data[2] & port_mask) == port_mask);
			Port.set_value(input * Vdd, true);
		} else if (name == "TRISA" || name == "TRISB") {
			bool input = ((data[2] & port_mask) == port_mask);
			Tris.set_value(input * Vdd, true);
		}
	}

	void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		process_register_change(r, name, data);  // handles TRIS/PORT changes
	}


  public:
	SinglePortA_Analog_RA4(Connection &a_Pin, Connection &comparator_out, const std::string &a_name) :
		BasicPort(a_Pin, a_name, 0, 4), m_comparator_out(comparator_out)
	{

		auto c = components();
		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Mux *mux = new Mux({&DataLatch.Q(), &m_comparator_out}, {&m_cmp_mode_sw});

		OrGate *Nor1 = new OrGate({&mux->rd(), &TrisLatch.Q()}, true);

		FET *nFET1 = new FET(Pin, Nor1->rd());
		nFET1->rd().set_value(Vss, false);

		c["Mux"] = mux;  // remember our mux component
		c["NOR Gate"] = Nor1;
		c["FET1"] = nFET1;

	}
	Connection &TMR0() {
		auto c = components();
		Schmitt &trigger = dynamic_cast<Schmitt &>(*c["Schmitt Trigger"]);
		return trigger.rd();
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

	void register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		if (name=="OPTION"){
			BYTE changed = data[0];
			BYTE new_value = data[2];

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

  public:
	Timer0(): assigned_to_wdt(false), falling_edge(false), use_RA4(false), prescale_rate(0) {
		DeviceEvent<Register>::subscribe<Timer0>(this, &Timer0::register_changed);
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


	Connection &operator[](BYTE idx) {
		if (idx==0 or idx > 18)
			throw(std::string("PIN Index is out of range: ") + int_to_string(idx));
		return pins[idx-1];
	}

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
//		std::cout << "Register changed " << name << std::endl;
	}

	PINS() {
		for (int n = 0; n < PIN_COUNT; ++n) {
			std::string name = "P"+int_to_string(n+1);
			pins[n].name(name);
		}
		DeviceEvent<Clock>::subscribe<PINS>(this, &PINS::clock_event);
		DeviceEvent<Register>::subscribe<PINS>(this, &PINS::register_changed);
	}

};


class PORTA: public Device {
	PINS &pins;

	void register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	}

  public:
	std::vector< SmartPtr<Device> > RA;
	PORTA(PINS &a_pins): pins(a_pins) {
		RA.resize(8);
		Connection Comp;
		DeviceEvent<Register>::subscribe<PORTA>(this, &PORTA::register_changed);
		RA[0] = new SinglePortA_Analog(pins[PINS::pin_RA0], "RA0");
		RA[1] = new SinglePortA_Analog(pins[PINS::pin_RA1], "RA1");
		RA[2] = new SinglePortA_Analog_RA2(pins[PINS::pin_RA2], "RA2");
//		RA[3] = new SinglePortA_Analog_RA3(pins[PINS::pin_RA3], Comp, "RA3");
	}
};


class PORTB: public Device {
	PINS &pins;

	void register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		if (name=="OPTION"){
			BYTE changed = data[0];
			BYTE new_value = data[2];

			if (changed & Flags::OPTION::RBPU) {
				recalc_pullups(pins, new_value & Flags::OPTION::RBPU);
			}
			if (changed & Flags::OPTION::INTEDG) {
				rising_rb0_interrupt(pins, new_value & Flags::OPTION::INTEDG);
			}
		}
	}

  public:
	PORTB(PINS &a_pins): pins(a_pins) {
		DeviceEvent<Register>::subscribe<PORTB>(this, &PORTB::register_changed);

	}

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

