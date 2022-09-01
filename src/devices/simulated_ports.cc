#include "simulated_ports.h"
#include <bitset>

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


template<class T> class DeviceEvent<T>::registry DeviceEvent<T>::subscribers;           // define static member variable

void BasicPort::queue_change(){  // Add a voltage change event to the queue
	eq.queue_event(new DeviceEvent<BasicPort>(*this, "Port Changed"));
	eq.process_events();
}

void BasicPort::complete_read() {
	if (pending.size()) {
		bool getval = false;
		Register *r =  pending.front();
		bool signal = Data.signal();
		if ((r->name() == "PORTA" || r->name() == "PORTB") and rdPort.signal()) {
			rdPort.set_value(Vss, true);   // Tristate 2 low
			getval = true;
		} else if ((r->name() == "TRISA" || r->name() == "TRISB") and rdTris.signal()) {
			rdTris.set_value(Vss, true);   //  Tristate 3 low
			getval = true;
		} else {
			std::cout << "Unexpected state [" << r->name() << "] whilst completing port read operation" << std::endl;
		}
		if (getval) {
			BYTE d = r->get_value();
			BYTE o = d;
			if (signal)
				d = d | port_mask;
			else
				d = d & (~port_mask);

			r->debug(debug());
			r->set_value(d, o);
			if (debug()) std::cout << "<------ " << Pin.name() << ": " << r->name() << " complete: signal = " << (signal?"high":"low") << " [" << std::bitset<8>( (int)d) << "]" << std::endl;
			if (rdPort.signal()) rdPort.set_value(Vss, true);
			if (rdTris.signal()) rdTris.set_value(Vss, true);
			if (debug()) {
				std::cout << "======================================================";
				std::cout << "  Read End " << this->name()<< ":" << r->name() << " ";
				std::cout << "======================================================";
				std::cout << std::endl;
			}
			queue_change();
		}
	}
}

void BasicPort::process_clock_change(Clock *c, const std::string &name, const std::vector<BYTE> &data)  {}
void BasicPort::on_clock_change(Clock *c, const std::string &name, const std::vector<BYTE> &data) {
	if (debug() && name[0]=='Q') std::cout << this->name() << ": Clock signal: [" << name << "]" << std::endl;
	if      (name == "Q2") {     // read happens at the start of an instruction cycle
		complete_read();
	} else if (name == "Q3") {         // read happens at the start of an instruction cycle
		if (pending.size()) {
			Register *r =  pending.front(); pending.pop();
			r->busy(false);
		}
	} else if (name == "Q4") {
		if (Port.signal()) {               // write only happens at the end of the clock cycle
			Port.set_value(Vss, true);     // The value on PortA/B changes to what is on the bus as clock goes low
			queue_change();
			if (debug()) {
				std::cout << "======================================================";
				std::cout << "  Write End " << this->name() << " Datalatch Port ";
				std::cout << "======================================================";
				std::cout << std::endl;
			}
		}
		if (Tris.signal()) {
			Tris.set_value(Vss, true);     // The value on TrisA/B changes to what is on the bus as clock goes low
			queue_change();
			if (debug()) {
				std::cout << "======================================================";
				std::cout << "  Write End " << this->name() << " Trislatch Port ";
				std::cout << "======================================================";
				std::cout << std::endl;
			}
		}
	}
	process_clock_change(c, name, data);  // call virtual function
}


//  data[0] == old value   data[1] == changed bits  data[2] == new value
void BasicPort::process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {}
void BasicPort::on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	const std::set<std::string>  output_registers({"TRISA", "TRISB", "PORTA", "PORTB"});
	const std::set<std::string>  input_registers({"TRISA.read", "TRISB.read", "PORTA.read", "PORTB.read"});
	if (output_registers.find(name) != output_registers.end()) {
		auto c = components();  // simulates a data bus write operation to either port or tris
		if ((data[Register::DVALUE::CHANGED] & port_mask) == port_mask) {  // this port or tris is changing
			if ((porta_select && name == "PORTA") || (!porta_select && name == "PORTB")) {
				if (debug()) {
					std::cout << "======================================================";
					std::cout << "  Write Start " << this->name()<< ":" << name << " ";
					std::cout << "======================================================";
					std::cout << std::endl;
				}
				bool input = ((data[Register::DVALUE::NEW] & port_mask) == port_mask);
				if (debug()) std::cout << this->name() << "; " << name << " = " << input << std::endl;

				Data.set_value(input * Vdd, false);    // set the input value on the bus.
				Port.set_value(Vdd, true);             // clock data into latch

				queue_change();
			} else if ((porta_select && name == "TRISA") || (!porta_select && name == "TRISB")) {
				if (debug()) {
					std::cout << "======================================================";
					std::cout << "  Write Start " << this->name()<< ":" << name << " ";
					std::cout << "======================================================";
					std::cout << std::endl;
				}
				bool input = ((data[Register::DVALUE::NEW] & port_mask) == port_mask);
				if (debug()) std::cout << this->name() << ":" << name << ": input=" << (input?"true":"false") << std::endl;

				Pin.impeded(!input);

				Data.set_value(input * Vdd, false);
				Tris.set_value(Vdd, true);
				queue_change();
			}
//			if (debug())
//				std::cout << this->name() << ":" << Pin.name() << " is " << (Pin.impeded()?"":" not ") << "impeded" << std::endl;
		}
	} else if (input_registers.find(name) != input_registers.end()) {
		if ((porta_select && name == "PORTA.read") || (!porta_select && name == "PORTB.read")) {
			if (debug()) {
				std::cout << "======================================================";
				std::cout << "  Read Start " << this->name()<< ":" << name << " ";
				std::cout << "======================================================";
				std::cout << std::endl;
			}
			Data.set_value(Vss, true);      // data is an input
			rdPort.set_value(Vdd, true);    // set the value of tristate 2 high.
			pending.push(r);
		} else if ((porta_select && name == "TRISA.read") || (!porta_select && name == "TRISB.read")) {
			if (debug()) {
				std::cout << "======================================================";
				std::cout << "  Read Start " << this->name()<< ":" << name << " ";
				std::cout << "======================================================";
				std::cout << std::endl;
			}
			Data.set_value(Vss, true);  // data is an input
			rdTris.set_value(Vdd, true);   // set the value of tristate 3 high.
			pending.push(r);
		}
	}
	process_register_change(r, name, data);   // call the port/pin-specific virtual override if defined
}

BasicPort::BasicPort(Terminal &a_Pin, const std::string &a_name, int port_no, int port_bit_ofs):
	Device(a_name), Pin(a_Pin),
	Data("Data.io"), Port("Port.ck"), Tris("Tris.ck"), rdPort("rdPort"), rdTris("rdTris"),
	porta_select(port_no==0), port_mask(1 << port_bit_ofs)
{
	Wire *DataBus = new Wire(a_name+"::databus");
	Wire *PinWire = new Wire(a_name+"::pinwire");

	Latch *DataLatch = new Latch(Data, Port, false, true); DataLatch->set_name(a_name+"::DataLatch");
	Latch *TrisLatch = new Latch(Data, Tris, false, true); TrisLatch->set_name(a_name+"::TrisLatch");

	DataBus->connect(Data);

	Pin.impeded(true);
	PinWire->connect(Pin);

	Inverter *NotPort = new Inverter(rdPort, "not(rdData)");

	m_components["Data Bus"] = DataBus;
	m_components["Pin Wire"] = PinWire;
	m_components["Data Latch"] = DataLatch;
	m_components["Tris Latch"] = TrisLatch;

	m_components["Inverter1"] = NotPort;

	DeviceEvent<Register>::subscribe<BasicPort>(this, &BasicPort::on_register_change);
	DeviceEvent<Clock>::subscribe<BasicPort>(this, &BasicPort::on_clock_change);
}
BasicPort::~BasicPort() {
	DeviceEvent<Register>::unsubscribe<BasicPort>(this, &BasicPort::on_register_change);
	DeviceEvent<Clock>::unsubscribe<BasicPort>(this, &BasicPort::on_clock_change);
}
Wire &BasicPort::bus_line() { return dynamic_cast<Wire &>(*m_components["Data Bus"]); }
Connection &BasicPort::data() { return Data; }
Connection &BasicPort::pin() { return Pin; }
std::map<std::string, SmartPtr<Device> > &BasicPort::components() { return m_components; }

//Connection &BasicPort::read_port() {           // read the port
//	auto c = components();
//	rdPort.set_value(Vdd, true);    // raise a read signal on the port read control line
//	eq.process_events();            // basically propagate all events, and their events etc.
//	rdPort.set_value(Vss, true);    // lower the read singal.  The result is on the bus.
//	eq.process_events();            // basically propagate all events, and their events etc.
//	return Data;
//}
//Connection &BasicPort::read_tris() {           // read the tris latch value
//	auto c = components();
//	rdTris.set_value(Vdd, true);    // raise a read signal on the port read control line
//	eq.process_events();            // basically propagate all events, and their events etc.
//	rdTris.set_value(Vss, true);    // lower the read singal.  The result is on the bus.
//	eq.process_events();            // basically propagate all events, and their events etc.
//	return Data;
//}
//

BasicPortA::BasicPortA(Terminal &a_Pin, const std::string &a_name, int port_bit_ofs):
	BasicPort(a_Pin, a_name, 0, port_bit_ofs), S1("Scmitt1.in"), S1_en("Schmitt1.en")
{
	auto &c = components();
	Inverter &NotPort = dynamic_cast<Inverter &>(*c["Inverter1"]);
	Wire &DataBus = dynamic_cast<Wire &>(*c["Data Bus"]);
	Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
	Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);

	Schmitt *trigger = new Schmitt(S1, S1_en, false, true, false);
	Latch *SR1 = new Latch(trigger->rd(), NotPort.rd(), true, false); SR1->set_name(a_name+"::InLatch");
	PinWire.connect(S1);

//	S1.name("S1"); S1.debug(true);
//	S1_en.name("S1_en"); S1_en.debug(true);

	Tristate *Tristate2 = new Tristate(SR1->Q(), rdPort);  Tristate2->name(a_name+"::TS(rdData)");
	DataBus.connect(Tristate2->rd());

	Tristate *Tristate3 = new Tristate(TrisLatch.Qc(), rdTris, false, true);  Tristate3->name(a_name+"::TS(rdTris)");
	DataBus.connect(Tristate3->rd());

	c["Tristate2"] = Tristate2;
	c["Schmitt Trigger"] = trigger;
	c["SR1"] = SR1;
	c["Tristate3"] = Tristate3;

}

//___________________________________________________________________________________
//  A model for a most ports, which have a Tristate connected to the DataLatch
// and TrisLatch, and clamps the port range.
SinglePortA::SinglePortA(Terminal &a_Pin, const std::string &a_name, int port_bit_ofs):
	BasicPortA(a_Pin, a_name, port_bit_ofs)
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

//___________________________________________________________________8________________
//  A model for a single port for pins RA0/AN0, RA1/AN1
//  These are standard ports, but with a comparator output
void SinglePortA_Analog::set_comparator(bool on) {
	if (on) {
		S1_en.set_value(Vdd, true);   // schmitt trigger enable active low
		m_comparator.set_value(m_comparator.rd(), true);    // low impedence
	} else {
		S1_en.set_value(Vss, true);
		m_comparator.set_value(m_comparator.rd(), true);
	}
}

void SinglePortA_Analog::set_comparators_for_an0_and_an1(BYTE cmcon) {
	if (debug()) std::cout << name() << ": Comparator mode = " << (int)(cmcon &0b111) << std::endl;
	switch (cmcon & 0b111) {
	case 0b001 :    // 3 inputs Multiplexed 2 Comparators
		if (name() == "RA0")
			set_comparator((cmcon & Flags::CMCON::CIS) == 0);
		else
			set_comparator(true);
		break;
	case 0b010 :    // 4 inputs Multiplexed 2 Comparators
	case 0b011 :    // 2 common reference comparators
	case 0b110 :    // Two common reference comparators with outputs
	case 0b100 :    // Two independent comparators
		set_comparator(true);
		break;
	case 0b101 :    // One independent comparator
		if (this->name() == "AN0" || this->name() == "RA0")
			set_comparator(false);
		else
			set_comparator(true);
		break;
	case 0b000 :    // Comparators reset
	case 0b111 :    // Comparators off
		set_comparator(false);
		break;
	}
}

void SinglePortA_Analog::comparator_changed(Comparator *c, const std::string &name, const std::vector<BYTE> &data) {
	BYTE cmcon = data[Comparator::DVALUE::NEW];
	set_comparators_for_an0_and_an1(cmcon);
}

SinglePortA_Analog::SinglePortA_Analog(Terminal &a_Pin, const std::string &a_name):
			SinglePortA(a_Pin, a_name, a_name=="RA0"?0:a_name=="RA1"?1:a_name=="RA2"?2:a_name=="RA3"?3:0),
			m_comparator(Vss, true, a_name+"::Comparator") {
	auto &c = components();
	set_comparators_for_an0_and_an1(0b111);
	Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
	PinWire.connect(comparator());

	DeviceEvent<Comparator>::subscribe<SinglePortA_Analog>(this, &SinglePortA_Analog::comparator_changed);
}
SinglePortA_Analog::~SinglePortA_Analog(){
	DeviceEvent<Comparator>::unsubscribe<SinglePortA_Analog>(this, &SinglePortA_Analog::comparator_changed);
}

Connection &SinglePortA_Analog::comparator() { return m_comparator; }

//___________________________________________________________________________________
//  A model for a single port for pin AN2.  This looks like AN0/AN1 except that
//  it also has a voltage reference.

void SinglePortA_Analog_RA2::process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	if        (name == "CMCON") {
		BYTE cmcon = data[Register::DVALUE::NEW];

		if (debug()) std::cout << "cmcon mode is " << (int)(cmcon & 7) << std::endl;
		switch (cmcon & 0b111) {
		case 0b000 :    // Comparators reset
			S1_en.set_value(Vss, true);
			m_comparator.set_value(Vss, false);
			break;
		case 0b101 :    // One independent comparator
		case 0b011 :    // 2 common reference comparators
		case 0b100 :    // Two independent comparators
		case 0b001 :    // 3 inputs Multiplexed 2 Comparators
		case 0b010 :    // 4 inputs Multiplexed 2 Comparators
		case 0b110 :    // Two common reference comparators with outputs
			set_comparator(true);
			break;
		case 0b111 :    // Comparators off
			set_comparator(false);
			break;
		}
	} else if (name == "VRCON") {
		BYTE vrcon = data[Register::DVALUE::NEW];
		bool vroe = (vrcon & Flags::VRCON::VROE) == Flags::VRCON::VROE;  // VRef output enable
		bool vren = (vrcon & Flags::VRCON::VREN) == Flags::VRCON::VREN;  // VRef enable
		bool vrr  = (vrcon & Flags::VRCON::VRR)  == Flags::VRCON::VRR;   // VRef Range
		float vref =  0;
		Relay &relay = VRef();
		relay.sw().set_value(vroe*Vdd, true);
		if (vren) {
			if (vrr) {
				vref = ((vrcon & 0b1111) / 24.0) * Vdd;
			} else {
				vref = ((vrcon & 0b1111) / 32.0) * Vdd + Vdd/4;
			}
		};
		relay.in().set_value(vref, true);
	}
}

SinglePortA_Analog_RA2::SinglePortA_Analog_RA2(Terminal &a_Pin, const std::string &a_name) :
		SinglePortA_Analog(a_Pin, a_name) {

	auto &c = components();
	Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
	m_vref_in.name("VREF");                   // a change to this will be detected by the Comparator module.
	Relay *VRef = new Relay(m_vref_in, m_vref_sw, "VRef");
	c["VRef"] = VRef;
	PinWire.connect(VRef->rd());
	VRef->rd().name(a_name+"::Comparator");   // This connection doubles for the output to the comparator
}

Relay &SinglePortA_Analog_RA2::VRef() {
	auto &c = components();
	Relay &vref = dynamic_cast<Relay &>(*c["VRef"]);
	return vref;
}

//___________________________________________________________________________________
//  A model for a single port for pin AN3.
//
// It differs from a standard port by the introduction of a Mux between the data latch
// and the tristate ouput.
//
// The mux selects a comparator output if the CMCON register has a comparator
// mode of 0b110, otherwise the Q output of the data latch is selected

void SinglePortA_Analog_RA3::set_comparator(bool on) {
	if (debug()) std::cout << "s1_en is " << (on?"true":"false") << std::endl;
	if (on) {
		S1_en.set_value(Vdd, true);      // schmitt trigger enable active low
		m_comparator.set_value(m_comparator.rd(), true);    // low impedence
	} else {
		S1_en.set_value(Vss, true);
		m_comparator.set_value(m_comparator.rd(), true);
	}
}

void SinglePortA_Analog_RA3::comparator_changed(Comparator *c, const std::string &name, const std::vector<BYTE> &data) {
	m_comparator_out.set_value((bool)(data[0] & Flags::CMCON::C1OUT) * Vdd, true);
	BYTE cmcon = data[Comparator::DVALUE::NEW];

	if (debug())
		std::cout << this->name() << ": Comparator mode is now set to: " << (int)(cmcon & 7) << std::endl;

	bool comparator_mode_switch = false;
	switch (cmcon & 0b111) {
	case 0b110 :    // Two common reference comparators with outputs
		comparator_mode_switch = true;
		/* no break */
	case 0b100 :    // Two independent comparators
	case 0b010 :    // 4 inputs Multiplexed 2 Comparators
	case 0b101 :    // One independent comparator - thats us
	case 0b011 :    // 2 common reference comparators
	case 0b001 :    // 3 inputs Multiplexed 2 Comparators
		set_comparator(true);
		break;
	case 0b000 :    // Comparators reset
	case 0b111 :    // Comparators off
		set_comparator(false);
	}
	m_cmp_mode_sw.set_value(comparator_mode_switch * Vdd, true);
}

SinglePortA_Analog_RA3::SinglePortA_Analog_RA3(Terminal &a_Pin, const std::string &a_name) :
	BasicPortA(a_Pin, a_name, 3), m_comparator(Vss, true, a_name+"::Comparator")
{
	auto &c = components();
	Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
	Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
	Mux *mux = new Mux({&DataLatch.Q(), &m_comparator_out}, {&m_cmp_mode_sw});

	c["Mux"] = mux;  // remember our mux component
	Tristate *Tristate1 = new Tristate(mux->rd(), TrisLatch.Q(), true);
	c["Tristate1"] = Tristate1;    // smart pointer should discard old Tristate1

	Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
	PinWire.connect(Tristate1->rd());
	PinWire.connect(m_comparator);

	Clamp * PinClamp = new Clamp(Pin);
	c["PinClamp"] = PinClamp;
	m_comparator_out.set_value(0, true);   // This must be tied to comparator output C1
	DeviceEvent<Comparator>::subscribe<SinglePortA_Analog_RA3>(this, &SinglePortA_Analog_RA3::comparator_changed);
}

SinglePortA_Analog_RA3::~SinglePortA_Analog_RA3() {
	DeviceEvent<Comparator>::unsubscribe<SinglePortA_Analog_RA3>(this, &SinglePortA_Analog_RA3::comparator_changed);
}

Connection &SinglePortA_Analog_RA3::comparator() { return m_comparator; }

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


// TODO:  the following does not properly clock data- ref: BasicPort
//void SinglePortA_Analog_RA4::process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
//	if (name == "PORTA" || name == "PORTB") {
//		bool input = ((data[2] & port_mask) == port_mask);
//		Port.set_value(input * Vdd, true);
//	} else if (name == "TRISA" || name == "TRISB") {
//		bool input = ((data[2] & port_mask) == port_mask);
//		Tris.set_value(input * Vdd, true);
//	}
//}

void SinglePortA_Analog_RA4::comparator_changed(Comparator *c, const std::string &name, const std::vector<BYTE> &data) {
	m_comparator_out.set_value(((data[Comparator::DVALUE::NEW] & Flags::CMCON::C2OUT) == Flags::CMCON::C2OUT) * Vdd, true);
	m_cmp_mode_sw.set_value(((data[Comparator::DVALUE::NEW] & 7) == 0b110) * Vdd, true);
}


SinglePortA_Analog_RA4::SinglePortA_Analog_RA4(Terminal &a_Pin, const std::string &a_name) :
	BasicPortA(a_Pin, a_name, 4)
{
	auto &c = components();
	Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
	Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
	Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
	Mux *mux = new Mux({&DataLatch.Q(), &m_comparator_out}, {&m_cmp_mode_sw}, "MUX1");

	OrGate *Nor1 = new OrGate({&mux->rd(), &TrisLatch.Q()}, true, "NOR1");

	FET *nFET1 = new FET(m_fet_drain, Nor1->rd());
	nFET1->rd().set_value(Vss, false);
	PinWire.connect(m_fet_drain);
	m_fet_drain.name("RA4::FET::drain");

	c["Mux"] = mux;  // remember our mux component
	c["NOR Gate"] = Nor1;
	c["FET1"] = nFET1;

	S1_en.set_value(Vss, true);            // always enabled

	m_cmp_mode_sw.set_value(0, true);      // Mux selects DataLatch.Q on startup
	m_comparator_out.set_value(0, true);   // This must be tied to comparator output C2
	DeviceEvent<Comparator>::subscribe<SinglePortA_Analog_RA4>(this, &SinglePortA_Analog_RA4::comparator_changed);
}

SinglePortA_Analog_RA4::~SinglePortA_Analog_RA4(){
	DeviceEvent<Comparator>::unsubscribe<SinglePortA_Analog_RA4>(this, &SinglePortA_Analog_RA4::comparator_changed);
}

Connection &SinglePortA_Analog_RA4::TMR0() {
	auto c = components();
	Schmitt &trigger = dynamic_cast<Schmitt &>(*c["Schmitt Trigger"]);
	return trigger.rd();
}


//___________________________________________________________________________________
//  A model for the pin MCLR/RA5/Vpp.
//
//  This port has no tris or data latch, but may serve as an input for data. An attempt
//  to read the non-existent tris latch state always returns 0.
//
//  A real chip would detect a high voltage on this pin to initiate in-circuit programming.
//
//  A low signal on this pin will reset the chip, if the MCLRE configuration bit is set,
//  otherwise the port may be used as an input.
//
//  This port, and the next, RA6, just look like nothing else.  So we will model it from
//  ground up.

	void SinglePortA_MCLR_RA5::on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
		if (name=="CONFIG1") {
			bool flag = data[Register::DVALUE::NEW] & Flags::CONFIG::MCLRE;
			MCLRE.set_value(Vdd*flag, false);
		} else {
			const std::set<std::string>  input_registers({"TRISA.read", "PORTA.read"});
			if (input_registers.find(name) != input_registers.end()) {
				bool getval = false;

				if (name == "PORTA.read") {
					Data.set_value(Vss, true);  // data is an input
					rdPort.set_value(Vdd, true);    // set the value of tristate 2 high.
					getval = true;
				} else if (name == "TRISA.read") {
					Data.set_value(Vss, true);  // data is an input
					rdTris.set_value(Vdd, true);   // set the value of tristate 3 high.
					getval = true;
				}
				if (getval) {
					eq.queue_event(new DeviceEvent<SinglePortA_MCLR_RA5>(*this, "Port Changed"));
					eq.process_events();
					bool signal = Data.signal();
					BYTE d = r->get_value();
					if (signal)
						d = d | 0b00100000;
					else
						d = d & 0b11011111;
					r->set_value(d, d);
					if (rdPort.signal()) rdPort.set_value(Vss, true);
					if (rdTris.signal()) rdTris.set_value(Vss, true);
				}
			}
		}
	}

	void SinglePortA_MCLR_RA5::HV_Detect(Connection *c, const std::string &name, const std::vector<BYTE> &data) {
		if (c == &Pin) {
			if (Pin.rd() > Pin.Vdd * 1.2)
				PGM.set_value(PGM.Vdd, true);
			else
				PGM.set_value(PGM.Vss, true);
		}
	}

	SinglePortA_MCLR_RA5::SinglePortA_MCLR_RA5(Terminal &a_Pin, const std::string &a_name) :
		Device(), Pin(a_Pin), MCLRE((float)0.0, false, "MCLRE")
	{
		Wire *DataBus = new Wire(a_name+"::data");
		Wire *PinWire = new Wire(a_name+"::pin");
		Wire *MCLREWire = new Wire(a_name+"::mclre");

		DataBus->connect(Data);

		Pin.set_value(Vdd, false);

		Schmitt *St1 = new Schmitt(S1, false, true);
		Schmitt *St2 = new Schmitt(S2, S2_en, true, true);

		AndGate *g1 = new AndGate({&MCLRE, &St1->rd()}, true, "And1");

		Inverter *NotPort = new Inverter(rdPort);
		Latch *SR1 = new Latch(St2->rd(), NotPort->rd(), true, false); SR1->set_name(a_name+"::InLatch");
		Tristate *Tristate2 = new Tristate(SR1->Q(), rdPort);  Tristate2->name(a_name+"::TS2");
		DataBus->connect(Tristate2->rd());

		Tristate *Tristate3 = new Tristate(cVss, rdTris, false, true);  Tristate3->name(a_name+"::TS3");
		DataBus->connect(Tristate3->rd());

		PinWire->connect(Pin);
		PinWire->connect(S1);
		PinWire->connect(S2);

		MCLREWire->connect(MCLRE);
		MCLREWire->connect(S2_en);

		m_components["Data Bus"] = DataBus;
		m_components["Pin Wire"] = PinWire;
		m_components["MCLRE Wire"] = MCLREWire;
		m_components["Schmitt1"] = St1;
		m_components["Schmitt2"] = St2;
		m_components["And1"] = g1;
		m_components["Inverter1"] = NotPort;
		m_components["SR1"] = SR1;
		m_components["Tristate2"] = Tristate2;
		m_components["Tristate3"] = Tristate3;

		DeviceEvent<Register>::subscribe<SinglePortA_MCLR_RA5>(this, &SinglePortA_MCLR_RA5::on_register_change);
		DeviceEvent<Connection>::subscribe<SinglePortA_MCLR_RA5>(this, &SinglePortA_MCLR_RA5::HV_Detect, &Pin);
	}
	SinglePortA_MCLR_RA5::~SinglePortA_MCLR_RA5(){
		DeviceEvent<Register>::unsubscribe<SinglePortA_MCLR_RA5>(this, &SinglePortA_MCLR_RA5::on_register_change);
		DeviceEvent<Connection>::unsubscribe<SinglePortA_MCLR_RA5>(this, &SinglePortA_MCLR_RA5::HV_Detect, &Pin);
	}
	Wire &SinglePortA_MCLR_RA5::bus_line() { return dynamic_cast<Wire &>(*m_components["Data Bus"]); }
	Connection &SinglePortA_MCLR_RA5::data() { return Data; }
	Terminal &SinglePortA_MCLR_RA5::pin() { return Pin; }
	Connection &SinglePortA_MCLR_RA5::mclr() {
		AndGate &nand1 = dynamic_cast<AndGate &>(*m_components["And1"]);
		return nand1.rd();
	}
	Connection &SinglePortA_MCLR_RA5::pgm() { return PGM; }
	std::map<std::string, SmartPtr<Device> > &SinglePortA_MCLR_RA5::components() { return m_components; }

//___________________________________________________________________________________
//  A model for the pin RA6/OSC2/CLKOUT
//  This time, there are the expected data and tris latches, and the read mechanism
// is comfortably familiar.
//  However, the internal oscillator circuit does potentially output the clock
// signal on this pin (FOSC = 011, 100, 110 with RA6=CLKOUT).   If RA6 is
// configured for I/O, CLKOUT can also be output with FOSC=101,111.
//  We should be able to derive this port from BasicPort.

void SinglePortA_RA6_CLKOUT::process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	if (name == "CONFIG1") {
		m_fosc = (data[Register::DVALUE::NEW] & 0b11) | ((data[Register::DVALUE::NEW] >> 2) & 0b100);
		m_OSC.set_value(Vss, true);
		bool clkout = false;
		bool ra6_is_io = false;
		switch (m_fosc) {
		case 0b111:         // RC osc + CLKOUT
		case 0b101:         // INTOSC + CLKOUT
			clkout = true;
			break;
		case 0b110:         // RC, RA6 is I/O
		case 0b100:         // INTOSC, RA6 is I/O
		case 0b011:         // EC, RA6 is I/O
			ra6_is_io = true;
			break;
		case 0b010:         // HS xtal/resonator on RA6 & RA7
		case 0b001:         // XT osc on RA6 & RA7
		case 0b000:         // LP osc on RA6 & RA7
			break;
		}
		m_Fosc1.set_value(Vdd*clkout, false);       // If High, select CLKOUT signal else port latch
		m_Fosc2.set_value(Vdd*ra6_is_io, false);    // If High, use pin for I/O
		S1_en.set_value(Vdd*(ra6_is_io), false);
	}
}

void SinglePortA_RA6_CLKOUT::process_clock_change(Clock *c, const std::string &name, const std::vector<BYTE> &data) {
	if (name == "oscillator") {
		if (m_fosc==0b010 || m_fosc==0b001 || m_fosc==0b000) {
		}
	} else if (name == "CLKOUT") {
		m_CLKOUT.set_value(((bool)data[0]) * Vdd, false);
	}
}

SinglePortA_RA6_CLKOUT::SinglePortA_RA6_CLKOUT(Terminal &a_Pin, const std::string &a_name) :
	BasicPortA(a_Pin, a_name, 6), m_fosc(0)
{
	auto &c = components();
	Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
	Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
	Schmitt &trigger = dynamic_cast<Schmitt &>(*c["Schmitt Trigger"]);
	Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);

	trigger.gate_invert(false);
	Clamp * PinClamp = new Clamp(Pin);
//	PinWire.debug(true);

	Mux *mux = new Mux({&DataLatch.Q(), &m_CLKOUT}, {&m_Fosc1});

	auto And1 = new AndGate({&TrisLatch.Qc(), &m_Fosc2}, false, "And1");
	auto Nor1 = new OrGate({&And1->rd(), &m_Fosc1}, true, "Nor1");
	Tristate *Tristate1 = new Tristate(mux->rd(), Nor1->rd(), true);
	PinWire.connect(Tristate1->rd());

	c["Tristate1"] = Tristate1;    // smart pointer should discard old Tristate1
	c["Mux"] = mux;  // remember our mux component
	c["PinClamp"] = PinClamp;
	c["And1"] = And1;
	c["Nor1"] = Nor1;

}

Connection &SinglePortA_RA6_CLKOUT::fosc1() { return m_Fosc1; }
Connection &SinglePortA_RA6_CLKOUT::fosc2() { return m_Fosc2; }
Connection &SinglePortA_RA6_CLKOUT::osc()   { return m_OSC; }

//___________________________________________________________________8________________
// Port RA7/Osc1/CLKIN shares most features with a basic port.
// This means that it has the normal data and Tris latches and the output pin is
// clamped between Vss and Vdd.  It varies for Fosc modes 0b100 and 0b101, in that
// an internal oscillator determines whether RA7 can be used as an input/output pin.
// For anything other than these internal oscillator modes, the pin leads straight
// to the clock circuits.
void PortA_RA7::process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	if (name == "CONFIG1") {
		BYTE fosc = (data[Register::DVALUE::NEW] & 0b11) | ((data[Register::DVALUE::NEW] >> 2) & 0b100);
		bool pin_is_io = (fosc==0b100 || fosc==0b101);
		m_Fosc.set_value(Vdd * pin_is_io, false);
		S1_en.set_value(Vdd * pin_is_io, true);
	}
}

PortA_RA7::PortA_RA7(Terminal &a_Pin, const std::string &a_name):
	BasicPortA(a_Pin, a_name, 7), m_Fosc()
{
	auto &c = components();
	Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
	Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
	Schmitt &trigger = dynamic_cast<Schmitt &>(*c["Schmitt Trigger"]);
	trigger.gate_invert(false);
	AndGate *nand1 = new AndGate({&m_Fosc, &TrisLatch.Qc()}, true, "NAND1");
	c["NAND1"] = nand1;

	Tristate *Tristate1 = new Tristate(DataLatch.Q(), nand1->rd(), true);
	Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
	PinWire.connect(Tristate1->rd());
	c["Tristate1"] = Tristate1;    // smart pointer should discard old Tristate1
	Clamp * PinClamp = new Clamp(Pin);
	c["PinClamp"] = PinClamp;
}

Connection &PortA_RA7::Fosc() { return m_Fosc; }


//___________________________________________________________________________________
//___________________________________________________________________________________
// PortB implementations
//___________________________________________________________________________________
//___________________________________________________________________________________
//  A model for a most ports, which have a Tristate connected to the DataLatch
// and TrisLatch, and clamps the port range.

void BasicPortB::process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	if (name == "OPTION") {
//		bit 7 RBPU: PORTB Pull-up Enable bit
//			1 = PORTB pull-ups are disabled
//			0 = PORTB pull-ups are enabled by individual port latch values
//		bit 6 INTEDG: Interrupt Edge Select bit
//			1 = Interrupt on rising edge of RB0/INT pin
//			0 = Interrupt on falling edge of RB0/INT pin
//		bit 5 T0CS: TMR0 Clock Source Select bit
//			1 = Transition on RA4/T0CKI pin
//			0 = Internal instruction cycle clock (CLKOUT)
//		bit 4 T0SE: TMR0 Source Edge Select bit
//			1 = Increment on high-to-low transition on RA4/T0CKI pin
//			0 = Increment on low-to-high transition on RA4/T0CKI pin
//		bit 3 PSA: Prescaler Assignment bit
//			1 = Prescaler is assigned to the WDT
//			0 = Prescaler is assigned to the Timer0 module
//		bit 2-0 PS2:PS0: Prescaler Rate Select bits

		RBPU().set_value((data[Register::DVALUE::NEW] & Flags::OPTION::RBPU)?Vdd:Vss, false);
		queue_change();
	}
}


BasicPortB::BasicPortB(Terminal &a_Pin, const std::string &a_name, int port_bit_ofs):
	BasicPort(a_Pin, a_name, 1, port_bit_ofs), m_iRBPU(m_RBPU)
{
	auto &c = components();

	Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
	Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
	Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
	Inverter &NotPort = dynamic_cast<Inverter &>(*c["Inverter1"]);
	Wire &DataBus = dynamic_cast<Wire &>(*c["Data Bus"]);

	Tristate *Tristate1 = new Tristate(DataLatch.Q(), TrisLatch.Q(), true); Tristate1->set_name(a_name+"::TS(pinOut)");
	Clamp * PinClamp = new Clamp(Pin);

	PinWire.connect(m_PinOut);
	ABuffer *b = new ABuffer(m_PinOut);
	Latch *SR1 = new Latch(b->rd(), NotPort.rd(), true, false); SR1->set_name(a_name+"::InLatch");

	Tristate *Tristate2 = new Tristate(SR1->Q(), rdPort);  Tristate2->set_name(a_name+"::TS(rdData)");
	DataBus.connect(Tristate2->rd());

	Tristate *Tristate3 = new Tristate(TrisLatch.Q(), rdTris, false, false);  Tristate3->set_name(a_name+"::TS(rdTris)");
	DataBus.connect(Tristate3->rd());

	c["Tristate1"] = Tristate1;   // connected to pin wire
	c["Tristate2"] = Tristate2;   // connected to data bus and RD_PORTB
	c["Tristate3"] = Tristate3;   // connected to data bus and RD_TRISB
	c["PinClamp"] = PinClamp;
	c["Out Buffer"] = b;
	c["SR1"] = SR1;

	AndGate *RBPU_GATE = new AndGate({&m_iRBPU, &TrisLatch.Q()}, true, "RBPU NAND");

	PullUp *VDD = new PullUp(Vdd, "Vdd");
	c["VDD"] = VDD;

	Inverse *iFETGate = new Inverse(RBPU_GATE->rd());
	FET *pFET1 = new FET(*VDD, *iFETGate, false);

	pFET1->name(name()+".pFET1");
	pFET1->rd().name(name()+".pFET1.out");
	RBPU_GATE->rd().name(name()+".NAND1.out");

	PinWire.connect(pFET1->rd());

	c["iFETGate"] = iFETGate;
	c["RBPU_NAND"] = RBPU_GATE;
	c["RBPU_FET"] = pFET1;

	PinWire.connect(Tristate1->rd());
}

BasicPortB::~BasicPortB() {

}


//___________________________________________________________________________
//  RB0 adds a schmitt trigger connected to an external interrupt signal
PortB_RB0::PortB_RB0(Terminal &a_Pin, const std::string &a_name):
	BasicPortB(a_Pin, a_name, 0)
{
	auto &c = components();

	Schmitt *trigger = new Schmitt(PinOut(), true, false);
	Wire *IntWire = new Wire(trigger->rd(), m_INT, "INT");

	c["INT_TRIGGER"] = trigger;
	c["INT_WIRE"] = IntWire;

}

//___________________________________________________________________________
//  RB1 adds a schmitt trigger connected to the USART receive input.
void PortB_RB1::process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	if (name == "RCSTA") {
		SPEN().set_value((data[Register::DVALUE::NEW] & Flags::RCSTA::SPEN)?Vdd:Vss, false);
		Peripheral_OE().set_value((data[Register::DVALUE::NEW] & Flags::RCSTA::SREN)?Vdd:Vss, false);
	}
	BasicPortB::process_register_change(r, name, data);
}

PortB_RB1::PortB_RB1(Terminal &a_Pin, const std::string &a_name):
	BasicPortB(a_Pin, a_name, 1), m_Peripheral_OE(Vdd), m_iSPEN(m_SPEN)
{
	auto &c = components();
	Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
	Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
	Tristate &TS1 = dynamic_cast<Tristate &>(*c["Tristate1"]);
	AndGate &PU_en = dynamic_cast<AndGate &>(*c["RBPU_NAND"]);
	PU_en.inputs({&iRBPU(), &TrisLatch.Q(), &m_iSPEN});

	Schmitt *trigger = new Schmitt(PinOut(), true, false);
	Wire *USART_RECWire = new Wire(trigger->rd(), m_USART_Receive, "USART receive input");
	Mux *dmux = new Mux({&DataLatch.Q(), &m_USART_Data_Out}, {&m_SPEN}, "Data Mux");
	TS1.input(dmux->rd());

	AndGate *Out_en = new AndGate({&TrisLatch.Q(), &m_Peripheral_OE});
	TS1.gate(Out_en->rd());

	c["USART_TRIGGER"] = trigger;
	c["USART_REC_WIRE"] = USART_RECWire;
	c["Data MUX"] = dmux;
	c["Out Enable"] = Out_en;

	SPEN().set_value(Vss, false);
	Peripheral_OE().set_value(Vdd, false);
	USART_Data_Out().set_value(Vss, false);
}

//___________________________________________________________________________
//  RB2 looks functionally identical to RB1, but some inputs differ
void PortB_RB2::process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	if (name == "RCSTA") {
		SPEN().set_value((data[Register::DVALUE::NEW] & Flags::RCSTA::SPEN)?Vdd:Vss, false);
		Peripheral_OE().set_value((data[Register::DVALUE::NEW] & Flags::RCSTA::SREN)?Vdd:Vss, false);
	}
	BasicPortB::process_register_change(r, name, data);
}

PortB_RB2::PortB_RB2(Terminal &a_Pin, const std::string &a_name):
	BasicPortB(a_Pin, a_name, 2), m_iSPEN(m_SPEN)
{
	auto &c = components();
	Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
	Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
	Tristate &TS1 = dynamic_cast<Tristate &>(*c["Tristate1"]);
	AndGate &PU_en = dynamic_cast<AndGate &>(*c["RBPU_NAND"]);
	PU_en.inputs({&iRBPU(), &TrisLatch.Q(), &m_iSPEN});

	Schmitt *trigger = new Schmitt(PinOut(), true, false);
	Wire *USART_RECWire = new Wire(trigger->rd(), m_USART_TX_CK_Out, "USART_Slave_Clock_in");
	Mux *dmux = new Mux({&DataLatch.Q(), &m_USART_TX_CK_Out}, {&m_SPEN}, "Data Mux");
	TS1.input(dmux->rd());

	AndGate *Out_en = new AndGate({&TrisLatch.Q(), &m_Peripheral_OE});
	TS1.gate(Out_en->rd());

	c["USART_TRIGGER"] = trigger;
	c["USART_REC_WIRE"] = USART_RECWire;
	c["Data MUX"] = dmux;
	c["Out Enable"] = Out_en;

	SPEN().set_value(Vss, false);
	Peripheral_OE().set_value(Vdd, false);
	USART_Slave_Clock_in().set_value(Vss, false);
}


//___________________________________________________________________________
//  RB3 is the last of the familiar looking port functions
void PortB_RB3::process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	if (name == "CCP1CON") {
		// TODO: What gets set here depends on CCP1CON contents
		CCP_Out().set_value(Vdd, false);
	}

	if (name == "RCSTA") {
		Peripheral_OE().set_value((data[Register::DVALUE::NEW] & Flags::RCSTA::SREN)?Vdd:Vss, false);
	}
	BasicPortB::process_register_change(r, name, data);
}

PortB_RB3::PortB_RB3(Terminal &a_Pin, const std::string &a_name):
	BasicPortB(a_Pin, a_name, 3), m_Peripheral_OE(Vdd)
{
	auto &c = components();
	Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
	Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
	Tristate &TS1 = dynamic_cast<Tristate &>(*c["Tristate1"]);
	AndGate &PU_en = dynamic_cast<AndGate &>(*c["RBPU_NAND"]);
	PU_en.inputs({&iRBPU(), &TrisLatch.Q(), &m_CCP1CON});

	Schmitt *trigger = new Schmitt(PinOut(), false, false);
	Wire *CCP_RECWire = new Wire(trigger->rd(), m_CCP_Out, "CCP_in");
	Mux *dmux = new Mux({&DataLatch.Q(), &m_CCP_Out}, {&m_CCP1CON}, "Data Mux");
	TS1.input(dmux->rd());

	AndGate *Out_en = new AndGate({&TrisLatch.Q(), &m_Peripheral_OE});
	TS1.gate(Out_en->rd());

	c["TRIGGER"] = trigger;
	c["CCP_REC_WIRE"] = CCP_RECWire;
	c["Data MUX"] = dmux;
	c["Out Enable"] = Out_en;

	CCP1CON().set_value(Vss, false);
	Peripheral_OE().set_value(Vdd, false);
	CCP_in().set_value(Vss, false);
}


//___________________________________________________________________________
//  RB4 is very different from the RB[1..3] designs.  Instead of
// an AND gate before Tristate1, we have an OR gate combining LVP
// and TrisLatch.Q.
//  Where RB0..3 have a TTL buffer before an SR latch to read
// input from the pin, we see instead two SR latches, with enable
// connected to the Q1 and Q3 clock cycles. Q3 is dependent on a
// simultaneous RD_PORTB signal (an AND gate).
//  SR1.Q feeds Tristate2 which is connected to the data bus, while
// both SR1.Q and SR2.Q are connected to an XOR gate, which in turn
// leads to an AND gate, also having inputs iLVP and TrisLatch.Q.
// Thus AND(iLVP, Trislatch.Q, XOR(SR1.Q, SR2.Q)) will set RBIF, as
// will almost the same arrangement for the remaining pins RB[5,6,7]
//

void PortB_RB4::process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	if (name == "CONFIG") {
		LVP().set_value((data[Register::DVALUE::NEW] & Flags::CONFIG::LVP)?Vdd:Vss, false);
	}
	BasicPortB::process_register_change(r, name, data);
}

void PortB_RB4::on_iflag(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
	RBIF().set_value(D->rd(), false);
}

void PortB_RB4::process_clock_change(Clock *D, const std::string &name, const std::vector<BYTE> &data) {
	if        (name == "Q1") {
		Q1().set_value(Vdd, false);
	} else if (name == "Q2") {
		Q1().set_value(Vss, false);
	} else if (name == "Q3") {
		Q3().set_value(Vdd, false);
	} else if (name == "Q4") {
		Q3().set_value(Vss, false);
	}
	queue_change();
}

PortB_RB4::PortB_RB4(Terminal &a_Pin, const std::string &a_name):
	BasicPortB(a_Pin, a_name, 4), m_iLVP(LVP())
{
	auto &c = components();
	Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
	Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
	ABuffer &b = dynamic_cast<ABuffer &>(*c["Out Buffer"]);

	Tristate &TS1 = dynamic_cast<Tristate &>(*c["Tristate1"]);
	Tristate &TS2 = dynamic_cast<Tristate &>(*c["Tristate2"]);
	AndGate &PU_en = dynamic_cast<AndGate &>(*c["RBPU_NAND"]);
	PU_en.inputs({&iRBPU(), &TrisLatch.Q(), &m_iLVP});

	Schmitt *trigger = new Schmitt(PinOut(), true, false);
	c["TRIGGER"] = trigger;
	c["PGM_RECWire"] = new Wire(trigger->rd(), m_PGM, "PGM input");
	TS1.input(DataLatch.Q());

	OrGate *Out_en = new OrGate({&TrisLatch.Q(), &m_LVP});
	TS1.gate(Out_en->rd());

	c["OR(TrisLatch.Q, LVP)"] = Out_en;

	c.erase("Inverter1");  // smart pointer will clean up
	c["AND(Q3,rdPort)"] = new AndGate({&rdPort, &Q3()});
	AndGate &q3_and_rd = dynamic_cast<AndGate &>(*c["AND(Q3,rdPort)"]);

	c["SR1"] = new Latch(b.rd(), Q1(), true, false);
	c["SR2"] = new Latch(b.rd(), q3_and_rd.rd(), true, false);

	Latch &SR1 = dynamic_cast<Latch &>(*c["SR1"]);
	Latch &SR2 = dynamic_cast<Latch &>(*c["SR2"]);

	TS2.input(SR1.Q());

	SR1.set_name(a_name+"::Q1");
	SR2.set_name(a_name+"::Q3");

	c["XOR(SR1.Q, SR2.Q)"] = new XOrGate({&SR1.Q(), &SR2.Q()});
	XOrGate &XOr1 = dynamic_cast<XOrGate &>(*c["XOR(SR1.Q, SR2.Q)"]);

	c["AND(iLVP, TrisLatch.Q, XOr1)"] = new AndGate({&m_iLVP, &TrisLatch.Q(), &XOr1.rd()});
	AndGate &IFlag = dynamic_cast<AndGate &>(*c["AND(iLVP, TrisLatch.Q, XOr1)"]);

	DeviceEvent<Connection>::subscribe<PortB_RB4>(this, &PortB_RB4::on_iflag, &IFlag.rd());

	PGM().set_value(Vss, true);
	LVP().set_value(Vss, false);
}

PortB_RB4::~PortB_RB4() {
	auto &c = components();
	AndGate &IFlag = dynamic_cast<AndGate &>(*c["AND(iLVP, TrisLatch.Q, XOr1)"]);
	DeviceEvent<Connection>::unsubscribe<PortB_RB4>(this, &PortB_RB4::on_iflag, &IFlag.rd());
}



//___________________________________________________________________________
//  RB5 is a stripped down version of RB4  The RBIF logic is the same.
void PortB_RB5::process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	BasicPortB::process_register_change(r, name, data);
}

void PortB_RB5::on_iflag(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
	RBIF().set_value(D->rd(), false);
}

void PortB_RB5::process_clock_change(Clock *D, const std::string &name, const std::vector<BYTE> &data) {
	if        (name == "Q1") {
		Q1().set_value(Vdd, false);
	} else if (name == "Q2") {
		Q1().set_value(Vss, false);
	} else if (name == "Q3") {
		Q3().set_value(Vdd, false);
	} else if (name == "Q4") {
		Q3().set_value(Vss, false);
	}
	queue_change();
}

PortB_RB5::PortB_RB5(Terminal &a_Pin, const std::string &a_name):
	BasicPortB(a_Pin, a_name, 5)
{
	auto &c = components();
	Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
	Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
	ABuffer &b = dynamic_cast<ABuffer &>(*c["Out Buffer"]);

	Tristate &TS1 = dynamic_cast<Tristate &>(*c["Tristate1"]);
	Tristate &TS2 = dynamic_cast<Tristate &>(*c["Tristate2"]);
	AndGate &PU_en = dynamic_cast<AndGate &>(*c["RBPU_NAND"]);
	PU_en.inputs({&iRBPU(), &TrisLatch.Q()});

	TS1.input(DataLatch.Q());
	TS1.gate(TrisLatch.Q());

	c.erase("Inverter1");  // smart pointer will clean up
	c["AND(Q3,rdPort)"] = new AndGate({&rdPort, &Q3()});
	AndGate &q3_and_rd = dynamic_cast<AndGate &>(*c["AND(Q3,rdPort)"]);

	c["SR1"] = new Latch(b.rd(), Q1(), true, false);
	c["SR2"] = new Latch(b.rd(), q3_and_rd.rd(), true, false);

	Latch &SR1 = dynamic_cast<Latch &>(*c["SR1"]);
	Latch &SR2 = dynamic_cast<Latch &>(*c["SR2"]);

	TS2.input(SR1.Q()); // reallocates TS2.output as well

	SR1.set_name(a_name+"::Q1");
	SR2.set_name(a_name+"::Q3");

	c["XOR(SR1.Q, SR2.Q)"] = new XOrGate({&SR1.Q(), &SR2.Q()});
	XOrGate &XOr1 = dynamic_cast<XOrGate &>(*c["XOR(SR1.Q, SR2.Q)"]);

	c["AND(TrisLatch.Q, XOr1)"] = new AndGate({&TrisLatch.Q(), &XOr1.rd()});
	AndGate &IFlag = dynamic_cast<AndGate &>(*c["AND(TrisLatch.Q, XOr1)"]);

	DeviceEvent<Connection>::subscribe<PortB_RB5>(this, &PortB_RB5::on_iflag, &IFlag.rd());

}

PortB_RB5::~PortB_RB5() {
	auto &c = components();
	AndGate &IFlag = dynamic_cast<AndGate &>(*c["AND(TrisLatch.Q, XOr1)"]);
	DeviceEvent<Connection>::unsubscribe<PortB_RB5>(this, &PortB_RB5::on_iflag, &IFlag.rd());
}



//___________________________________________________________________________
//  RB6 is very similar to RB4, but with a few variations.  RB4::LVP is replaced by
// RB6::T1OSCEN,  which input now also feeds a tristate gate, controlling whether
// or not RB6::TMR1_Oscillator (from Port RB7) is raised on the pin wire. T1OSCEN is
// also inverted on an AND gate (which replaces RB4::TTL_Buffer) and switches off
// the pin signal to the SR latches which normally would drive RdPortB.
//  Other than that, the design is essentially that of RB4.
void PortB_RB6::process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	if (name == "T1CON") {
		T1OSCEN().set_value((data[Register::DVALUE::NEW] & Flags::T1CON::T1OSCEN)?Vdd:Vss, false);
	}
	BasicPortB::process_register_change(r, name, data);
}

void PortB_RB6::on_iflag(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
	RBIF().set_value(D->rd(), false);
}

void PortB_RB6::process_clock_change(Clock *D, const std::string &name, const std::vector<BYTE> &data) {
	if        (name == "Q1") {
		Q1().set_value(Vdd, false);
	} else if (name == "Q2") {
		Q1().set_value(Vss, false);
	} else if (name == "Q3") {
		Q3().set_value(Vdd, false);
	} else if (name == "Q4") {
		Q3().set_value(Vss, false);
	}
	queue_change();
}

PortB_RB6::PortB_RB6(Terminal &a_Pin, const std::string &a_name):
	BasicPortB(a_Pin, a_name, 6), m_iT1OSCEN(T1OSCEN())
{
	auto &c = components();
	Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
	Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);

	Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);

	c.erase("Out Buffer");

	c["Out Buffer"] = new AndGate({&m_iT1OSCEN, &PinOut()});
	AndGate &Out_Buffer = dynamic_cast<AndGate &>(*c["Out Buffer"]);

	c["TMR1 Osc"] = new Tristate(m_T1OSC, m_T1OSCEN, false, true);
	Tristate &TMR1Osc = dynamic_cast<Tristate &>(*c["TMR1 Osc"]);
	PinWire.connect(TMR1Osc.rd());

	Tristate &TS1 = dynamic_cast<Tristate &>(*c["Tristate1"]);
	Tristate &TS2 = dynamic_cast<Tristate &>(*c["Tristate2"]);
	AndGate &PU_en = dynamic_cast<AndGate &>(*c["RBPU_NAND"]);
	PU_en.inputs({&iRBPU(), &TrisLatch.Q(), &m_iT1OSCEN});

	Schmitt *trigger = new Schmitt(PinOut(), true, false);
	c["TRIGGER"] = trigger;
	c["TMR1_CkWire"] = new Wire(trigger->rd(), m_TMR1_Clock, "TMR1 Clock input");
	TS1.input(DataLatch.Q());

	OrGate *Out_en = new OrGate({&TrisLatch.Q(), &m_T1OSCEN});
	TS1.gate(Out_en->rd());

	c["OR(TrisLatch.Q, T1OSCEN)"] = Out_en;

	c.erase("Inverter1");  // smart pointer will clean up
	c["AND(Q3,rdPort)"] = new AndGate({&rdPort, &Q3()});
	AndGate &q3_and_rd = dynamic_cast<AndGate &>(*c["AND(Q3,rdPort)"]);

	c["SR1"] = new Latch(Out_Buffer.rd(), Q1(), true, false);
	c["SR2"] = new Latch(Out_Buffer.rd(), q3_and_rd.rd(), true, false);

	Latch &SR1 = dynamic_cast<Latch &>(*c["SR1"]);
	Latch &SR2 = dynamic_cast<Latch &>(*c["SR2"]);

	TS2.input(SR1.Q());

	SR1.set_name(a_name+"::Q1");
	SR2.set_name(a_name+"::Q3");

	c["XOR(SR1.Q, SR2.Q)"] = new XOrGate({&SR1.Q(), &SR2.Q()});
	XOrGate &XOr1 = dynamic_cast<XOrGate &>(*c["XOR(SR1.Q, SR2.Q)"]);

	c["AND(iT1OSCEN, TrisLatch.Q, XOr1)"] = new AndGate({&m_iT1OSCEN, &TrisLatch.Q(), &XOr1.rd()});
	AndGate &IFlag = dynamic_cast<AndGate &>(*c["AND(iT1OSCEN, TrisLatch.Q, XOr1)"]);

	DeviceEvent<Connection>::subscribe<PortB_RB6>(this, &PortB_RB6::on_iflag, &IFlag.rd());

	TMR1_Clock().set_value(Vss, true);
	T1OSCEN().set_value(Vss, false);
	T1OSC().set_value(Vss, true);
}

PortB_RB6::~PortB_RB6() {
	auto &c = components();
	AndGate &IFlag = dynamic_cast<AndGate &>(*c["AND(iT1OSCEN, TrisLatch.Q, XOr1)"]);
	DeviceEvent<Connection>::unsubscribe<PortB_RB6>(this, &PortB_RB6::on_iflag, &IFlag.rd());
}


//___________________________________________________________________________
//  RB7 is again similar in many respects to RB6.  The two block diagrams in
// fact share a Tristate component which connects the RB7 PIN to the RB6 pin
// if T1OSCEN is active.
//  Where RB6 has a serial programming clock as output, RB7 has a serial
// programming signal as output, again dependent on T1OSCEN, wich also disables
// normal pin read function as is the case with RB6.
void PortB_RB7::process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	if (name == "T1CON") {
		T1OSCEN().set_value((data[Register::DVALUE::NEW] & Flags::T1CON::T1OSCEN)?Vdd:Vss, false);
	}
	BasicPortB::process_register_change(r, name, data);
}

void PortB_RB7::on_iflag(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
	RBIF().set_value(D->rd(), false);
}

void PortB_RB7::process_clock_change(Clock *D, const std::string &name, const std::vector<BYTE> &data) {
	if        (name == "Q1") {
		Q1().set_value(Vdd, false);
	} else if (name == "Q2") {
		Q1().set_value(Vss, false);
	} else if (name == "Q3") {
		Q3().set_value(Vdd, false);
	} else if (name == "Q4") {
		Q3().set_value(Vss, false);
	}
	queue_change();
}

PortB_RB7::PortB_RB7(Terminal &a_Pin, const std::string &a_name):
	BasicPortB(a_Pin, a_name, 7), m_iT1OSCEN(T1OSCEN())
{
	auto &c = components();
	Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);

	c.erase("Out Buffer");
	c["Out Buffer"] = new AndGate({&m_iT1OSCEN, &PinOut()});
	AndGate &Out_Buffer = dynamic_cast<AndGate &>(*c["Out Buffer"]);
	ABuffer *osc_buffer = new ABuffer(Pin, "T1 Oscillator");
	c["T1 Oscillator"] = osc_buffer;
	Wire *rb6_out = new Wire(osc_buffer->rd(), T1OSC(), "RB6 Out");
	c["RB6 Out"] = rb6_out;

	Tristate &TS1 = dynamic_cast<Tristate &>(*c["Tristate1"]);
	Tristate &TS2 = dynamic_cast<Tristate &>(*c["Tristate2"]);
	AndGate &PU_en = dynamic_cast<AndGate &>(*c["RBPU_NAND"]);
	PU_en.inputs({&iRBPU(), &TrisLatch.Q(), &m_iT1OSCEN});

	Schmitt *trigger = new Schmitt(PinOut(), true, false);
	trigger->name("PGM trigger");
	c["TRIGGER"] = trigger;

	c["TMR1 Osc"] = new Tristate(PinOut(), m_T1OSCEN, false, true);
//	Tristate &TMR1Osc = dynamic_cast<Tristate &>(*c["TMR1 Osc"]);

	c["AND(iT1OSCEN, Trigger)"] = new AndGate({&m_iT1OSCEN, &trigger->rd()});
	AndGate &SPROG_en = dynamic_cast<AndGate &>(*c["AND(iT1OSCEN, Trigger)"]);


	c["SPROG"] = new Wire(SPROG_en.rd(), m_SPROG, "Serial Programming input");

	OrGate *Out_en = new OrGate({&TrisLatch.Q(), &m_T1OSCEN});
	c["OR(TrisLatch.Q, T1OSCEN)"] = Out_en;
	TS1.gate(Out_en->rd());

	c.erase("Inverter1");  // smart pointer will clean up
	c["AND(Q3,rdPort)"] = new AndGate({&rdPort, &Q3()});
	AndGate &q3_and_rd = dynamic_cast<AndGate &>(*c["AND(Q3,rdPort)"]);

	c["SR1"] = new Latch(Out_Buffer.rd(), Q1(), true, false);
	c["SR2"] = new Latch(Out_Buffer.rd(), q3_and_rd.rd(), true, false);

	Latch &SR1 = dynamic_cast<Latch &>(*c["SR1"]);
	Latch &SR2 = dynamic_cast<Latch &>(*c["SR2"]);

	TS2.input(SR1.Q());

	SR1.set_name(a_name+"::Q1");
	SR2.set_name(a_name+"::Q3");

	c["XOR(SR1.Q, SR2.Q)"] = new XOrGate({&SR1.Q(), &SR2.Q()});
	XOrGate &XOr1 = dynamic_cast<XOrGate &>(*c["XOR(SR1.Q, SR2.Q)"]);

	c["AND(iT1OSCEN, TrisLatch.Q, XOr1)"] = new AndGate({&m_iT1OSCEN, &TrisLatch.Q(), &XOr1.rd()});
	AndGate &IFlag = dynamic_cast<AndGate &>(*c["AND(iT1OSCEN, TrisLatch.Q, XOr1)"]);

	DeviceEvent<Connection>::subscribe<PortB_RB7>(this, &PortB_RB7::on_iflag, &IFlag.rd());

	SPROG().set_value(Vss, false);
	T1OSCEN().set_value(Vss, false);
	T1OSC().set_value(Vss, true);
}

PortB_RB7::~PortB_RB7() {
	auto &c = components();
	AndGate &IFlag = dynamic_cast<AndGate &>(*c["AND(iT1OSCEN, TrisLatch.Q, XOr1)"]);
	DeviceEvent<Connection>::unsubscribe<PortB_RB7>(this, &PortB_RB7::on_iflag, &IFlag.rd());
}

