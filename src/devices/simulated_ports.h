#include "device_base.h"
#include "register.h"
#include "sram.h"
#include "flags.h"
#include "../utils/utility.h"


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
	bool debug = false;

	void process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		auto c = components();  // simulates a data bus write operation to either port or tris
		if ((data[0] & port_mask) == port_mask) {  // this port or tris is changing
			if ((porta_select && name == "PORTA") || (!porta_select && name == "PORTB")) {
				bool input = ((data[2] & port_mask) == port_mask);
				if (debug) std::cout << this->name() << "; " << name << " = " << input << std::endl;
				Data.set_value(input * Vdd, true);    // set the input value on the bus.
				eq.process_events();            // basically propagate all events, and their events etc.
				Port.set_value(Vdd, true);
				eq.process_events();
				Port.set_value(Vss, true);      // latch the value from the bus into Q
				eq.process_events();
			} else if ((porta_select && name == "TRISA") || (!porta_select && name == "TRISB")) {
				bool input = ((data[2] & port_mask) == port_mask);
				if (debug) std::cout << this->name() << "; " << name << " = " << input << std::endl;
				Data.set_value(input * Vdd, true);
				eq.process_events();
				Tris.set_value(Vdd, true);
				eq.process_events();
				Tris.set_value(Vss, true); // clock the signal
				eq.process_events();
				Data.set_value(Data.rd(), true);    // impeded.
			}
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

		Latch *DataLatch = new Latch(Data, Port, false, true); DataLatch->set_name(a_name+"::DataLatch");
		Latch *TrisLatch = new Latch(Data, Tris, false, true); TrisLatch->set_name(a_name+"::TrisLatch");

		PinWire->connect(Pin);
		Schmitt *trigger = new Schmitt(S1, S1_en);

		Inverter *NotPort = new Inverter(Port);
		Latch *SR1 = new Latch(trigger->rd(), NotPort->rd(), true, false); SR1->set_name(a_name+"::InLatch");
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
	Connection &data() { return Data; }
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
		if  (name == "CMCON") {
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
		set_comparators_for_an0_and_an1(0b111);
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
		auto &c = components();
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
//  A model for the pin RA4/AN4.
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

		auto &c = components();
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


