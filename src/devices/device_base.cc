#include "device_base.h"
#include "../utils/utility.h"


template <class Wire> class
	DeviceEvent<Wire>::registry  DeviceEvent<Wire>::subscribers;

//___________________________________________________________________________________
// The hardware architecture uses several sequential logic components.  It would be
// really nice to be able to emulate the way the hardware works by stringing together
// elements and reacting to events that cause status to change.
//
// Here's a basic connection.  Changes to connection values always spawn and
// process device events.
//
// Note:  Store conductance (1/R) instead of R, because we use it way more often.
//
	void Connection::queue_change(){  // Add a voltage change event to the queue
		eq.queue_event(new DeviceEvent<Connection>(*this, "Voltage Change"));
		if (debug())
			std::cout << name() << ": processing events" << std::endl;
		eq.process_events();
		if (debug())
			std::cout << name() << ": done processing events" << std::endl;
	}

	float Connection::calc_voltage_drop(float out_voltage) {
		float V = rd(false);
		if (debug()) {
			std::cout << name() << ": Calculating vDrop = out_voltage["<<out_voltage << "] - V[" ;
			std::cout << V << "] = " << out_voltage - V << std::endl;
		}
		return out_voltage - V;
	}

	bool Connection::impeded_suppress_change(bool a_impeded) {
		if (m_impeded != a_impeded) {
			if (debug())
				std::cout << "Connection " << name() << ": impeded " << m_impeded << " -> " << a_impeded << std::endl;
			m_impeded = a_impeded;
			if (m_impeded) m_vDrop = 0;
			return true;
		}
		return false;
	}


	Connection::Connection(const std::string &a_name):
		Device(a_name), m_V(Vss), m_conductance(1.0e+4), m_impeded(true), m_determinate(false), m_vDrop(0) {
	};

	Connection::Connection(float V, bool impeded, const std::string &a_name):
		Device(a_name), m_V(V), m_conductance(1.0e+4), m_impeded(impeded), m_determinate(true), m_vDrop(0) {
	};

	Connection::~Connection() { eq.remove_events_for(this); }

	float Connection::rd(bool include_vdrop) const {
		float val = m_V + include_vdrop * vDrop();
		if (debug()) {
			if (include_vdrop)
				std::cout << "Connection " << name() << ": read value ["<< m_V << " + " << vDrop() << "] = " << val << std::endl;
			else
				std::cout << "Connection " << name() << ": read value = " << val << std::endl;
		}
		return val;
	}

	float Connection::vDrop() const { return m_vDrop; }

	bool Connection::signal() const { return rd(true) > Vdd/2.0; }
	bool Connection::impeded() const { return m_impeded; }
	bool Connection::determinate() const { return m_determinate || !impeded(); }
	float Connection::set_vdrop(float out_voltage) {
		m_vDrop = calc_voltage_drop(out_voltage);
		if (debug()) std::cout << "vDrop: " << name() << "= " << m_vDrop << std::endl;
		return m_vDrop;
	}
//	virtual bool determinate() const { return m_determinate; }

	void Connection::impeded(bool a_impeded) {
		if (impeded_suppress_change(a_impeded)) queue_change();
	}

	void Connection::determinate(bool on) {
		if (debug())
			std::cout << name() << ": determinate " << m_determinate << " -> " << on << std::endl;
		m_determinate = on;
	}

	void Connection::conductance(float iR) {  // set internal resistanve
		if (iR >= min_R && iR <= max_R) {
			m_conductance = iR;
		} else if (iR < min_R) {
			impeded(true);
		}
	}

	void Connection::R(float a_R) {  // set internal resistanve
		conductance(1/a_R);
	}

	float Connection::conductance() const {  // internal resistanve
		return m_conductance;
	}

	float Connection::R() const {  // internal resistanve
		return 1/m_conductance;
	}

	float Connection::I() const {  // maximum current given V & R
		return m_vDrop * m_conductance;
	}

	void Connection::set_value(float V, bool a_impeded) {
		if (m_V != V || m_impeded != a_impeded || !determinate()) {
			determinate(true);     // the moment we change the value of a connection the value is determined
			impeded_suppress_change(a_impeded);
			m_V = V;
			m_vDrop = 0;
			if (debug())
				std::cout << "Connection " << name() << ": set value V = " << V << std::endl;
			queue_change();
		}
	}


//___________________________________________________________________________________
//   A terminal for connections.  It is impeded by default, but any
// inputs connected to the pin will allow the pin itself to become an input.
// This makes a pin function a little like a "wire", but at the same time
// it is also itself a "connection".
//   A terminal without connections is always impeded (treated as an output).

	void Terminal::recalc() {
		if (m_connects.size()) {   // don't change value unless at least one connection
			m_terminal_impeded = true;
			float sum_conductance = 0;
			float sum_v_over_R = 0;
			for (auto &c : m_connects ) {
				if (c->impeded()) {
				} else {
					m_terminal_impeded = false;
					float iR = c->conductance();
					float Vc = c->rd(false);
					sum_conductance += iR;
					sum_v_over_R += Vc * iR;
				}
			}
			if (not m_terminal_impeded) {
				conductance(sum_conductance);  // treat that external R as an internal R
				float V = sum_v_over_R/sum_conductance;
				Connection::set_value(V, Connection::impeded());       // the calculated voltage becomes our voltage
			}
		}
	}

	void Terminal::on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc();
		for (auto &c : m_connects ) {
			if (c->impeded()) {           // set voltage on outputs
				c->set_value(rd(false), true);
			}
		}
	}

	float Terminal::calc_voltage_drop(float out_voltage) {
		float total_vDrop = Connection::calc_voltage_drop(out_voltage); // vDrop = out_voltage - V
		if ( not impeded() && m_connects.size()) {    // if we have inputs, update them
			for (auto &c : m_connects ) {
				if (not c->impeded()) {
					c->set_vdrop(out_voltage);
				}
			}
		}
		return total_vDrop;
	}

	Terminal::Terminal(const std::string name): Connection(name), m_connects(), m_terminal_impeded(true) {}
	Terminal::Terminal(float V, const std::string name): Connection(V, true, name), m_connects(), m_terminal_impeded(true) {}
	Terminal::~Terminal() {
		for (auto &c : m_connects ) {
			DeviceEvent<Connection>::unsubscribe<Terminal>(this, &Terminal::on_change, c);
		}
	}
	void Terminal::connect(Connection &c) {
		if (m_connects.find(&c) == m_connects.end()) {
			m_connects.insert(&c);
			recalc();
			DeviceEvent<Connection>::subscribe<Terminal>(this, &Terminal::on_change, &c);
		}
	}
	void Terminal::disconnect(Connection &c) {
		if (m_connects.find(&c) != m_connects.end()) {
			DeviceEvent<Connection>::unsubscribe<Terminal>(this, &Terminal::on_change, &c);
			m_connects.erase(&c);
			recalc();
		}
	}

	void Terminal::set_value(float V, bool a_impeded) {
		// NOTE: When our actual internal voltage is being determined by a pool of connections with at
		//       least one input, we cannot (should not) override the internal voltage by using
		//       set_value().   We can achieve the same effect as set_value() by setting default
		//       direction, and calling set_vdrop() instead.
		if (m_connects.size() && !m_terminal_impeded) {
			Connection::impeded_suppress_change(a_impeded);
			float drop = vDrop();
			set_vdrop(V);
			if (drop != vDrop()) queue_change();
		} else {
			Connection::set_value(V, a_impeded);
		}
	}

	void Terminal::impeded(bool a_impeded) { Connection::impeded(a_impeded); }
	bool Terminal::impeded() const { if (m_connects.size()) return m_terminal_impeded; return Connection::impeded(); }
	float Terminal::rd(bool include_vdrop) const {
		return Connection::rd(include_vdrop);
	}


//___________________________________________________________________________________
// Voltage source.   Always constant, no matter what.  Overrides connection V.
	Voltage::Voltage(float V, const std::string name): Terminal(V, name), m_voltage(V) {}

	bool Voltage::impeded() const { return false; }
	float Voltage::rd(bool include_vdrop) const { return m_voltage; }


//___________________________________________________________________________________
// A Weak Voltage source.  If there are any unimpeded connections, we calculate the
// lowest unimpeded, otherwise the highest impeded connection as the voltage
	PullUp::PullUp(float V, const std::string name): Connection(V, false, name) {
		R(1.0e+6);
	}

	bool PullUp::impeded() const { return false; }


//___________________________________________________________________________________
// Ground.  Always zero.
	Ground::Ground(): Voltage(0, "GND") {}

	bool Ground::impeded() const { return false; }
	float Ground::rd(bool include_vdrop) const { return Vss; }


//___________________________________________________________________________________
// An inverted connection.  If the connection was high, we return low, and vica versa.
	void Inverse::on_connection_change(Connection *conn, const std::string &name, const std::vector<BYTE> &data)  {
		Connection::set_value((not conn->signal())*Vdd, conn->impeded());
	}

	Inverse::Inverse(Connection &a_c): Connection(), c(a_c) {
		Connection::set_value((not c.signal())*Vdd, c.impeded());
		DeviceEvent<Connection>::subscribe<Inverse>(this, &Inverse::on_connection_change, &c);
	}
	Inverse::~Inverse() {
		DeviceEvent<Connection>::unsubscribe<Inverse>(this, &Inverse::on_connection_change, &c);
	}

	void Inverse::set_value(float V, bool a_impeded) { c.set_value(V, a_impeded); }
	void Inverse::impeded(bool a_impeded) { Connection::impeded(a_impeded); c.impeded(a_impeded); }
	void Inverse::determinate(bool on) { Connection::determinate(on); c.determinate(on); }

//___________________________________________________________________________________
// Allows us to treat a connection as an output by setting impedence low.  An impeded
// connection has an arbitrary high resistance, while an unimpeded connection has
// an arbitrarily low resistance (a current source).
// In this digital model, we don't particularly care what the resistance is, or how
// much current flows.

	void Output::on_connection_change(Connection *c, const std::string &name, const std::vector<BYTE> &data)  {
		queue_change();
	}

	Output::Output(): Connection(), c(*this), wrapper(false) {}
	Output::Output(Connection &a_c): Connection(), c(a_c), wrapper(true) {
		DeviceEvent<Connection>::subscribe<Output>(this, &Output::on_connection_change, &c);
	}
	Output::Output(float V, const std::string &a_name): Connection(V, false, a_name), c(*this), wrapper(false) {};
	Output::~Output() {
		if (wrapper)
			DeviceEvent<Connection>::unsubscribe<Output>(this, &Output::on_connection_change, &c);
	}

	bool Output::signal() const { if (wrapper) return c.signal(); return Connection::signal(); }
	float Output::rd(bool include_vdrop) const { if (wrapper) return c.rd(); return Connection::rd(); }
	bool Output::impeded() const { return false; }
	bool Output::determinate() const { return true; }

	void Output::set_value(float V, bool a_impeded) { if (wrapper) c.set_value(V, false); else Connection::set_value(V, false); }
	void Output::impeded(bool a_impeded) { if (wrapper) c.impeded(false); else Connection::impeded(false); }
	void Output::determinate(bool on) { if (wrapper) c.determinate(true); else Connection::determinate(true); }


//___________________________________________________________________________________
// An output connection is unimpeded- basically, this means that multiple inputs
// will allow current to flow between them.  Using Output() and Input() for the
// same Connection(), we can control exactly how we want to define components.  For
// example, an AND gate will have one or more Inputs with an unimpeded Output.

	void Input::on_connection_change(Connection *c, const std::string &name, const std::vector<BYTE> &data)  {
		queue_change();
	}

	Input::Input(): Connection(), c(*this), wrapper(false) {}
	Input::Input(Connection &a_c): Connection(), c(a_c), wrapper(true) {
		DeviceEvent<Connection>::subscribe<Input>(this, &Input::on_connection_change, &c);
	}
	Input::Input(float V, const std::string &a_name): Connection(V, false, a_name), c(*this), wrapper(false) {};
	Input::~Input() {
		if (wrapper)
			DeviceEvent<Connection>::unsubscribe<Input>(this, &Input::on_connection_change, &c);
	}

	bool Input::signal() const { if (wrapper) return c.signal(); return Connection::signal(); }
	float Input::rd(bool include_vdrop) const { if (wrapper) return c.rd(); return Connection::rd(); }
	bool Input::impeded() const { return true; }
	bool Input::determinate() const { if (wrapper) return c.determinate(); return Connection::determinate(); }

	void Input::set_value(float V, bool a_impeded) { if (wrapper) c.set_value(V, true); else Connection::set_value(V, true); }
	void Input::impeded(bool a_impeded) { if (wrapper) c.impeded(true); else Connection::impeded(true); }
	void Input::determinate(bool on) { if (wrapper) c.determinate(on); else Connection::determinate(on); }

//___________________________________________________________________________________
//  Generic gate,
	void Gate::recalc() {}
	void Gate::on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		if (debug()) {
			std::cout << "AndGate " << this->name() << " received event " << name << std::endl;
		}
		recalc();
	}

	Gate::Gate(const std::vector<Connection *> &in, bool inverted, const std::string &a_name):
		Device(a_name), m_in(in), m_out(Vdd), m_inverted(inverted) {
		for (size_t n = 0; n < m_in.size(); ++n)
			DeviceEvent<Connection>::subscribe<Gate>(this, &Gate::on_change, m_in[n]);
		recalc();
	}
	Gate::~Gate() {
		for (size_t n = 0; n < m_in.size(); ++n)
			DeviceEvent<Connection>::unsubscribe<Gate>(this, &Gate::on_change, m_in[n]);
	}
	void Gate::connect(size_t a_pos, Connection &in) {
		if (a_pos+1 > m_in.size()) m_in.resize(a_pos+1);
		if (m_in[a_pos])
			DeviceEvent<Connection>::unsubscribe<Gate>(this, &Gate::on_change, m_in[a_pos]);
		m_in[a_pos] = &in;
		DeviceEvent<Connection>::subscribe<Gate>(this, &Gate::on_change, m_in[a_pos]);
	}

	void Gate::disconnect(size_t a_pos) {
		if (a_pos+1 > m_in.size()) return;
		if (m_in[a_pos])
			DeviceEvent<Connection>::unsubscribe<Gate>(this, &Gate::on_change, m_in[a_pos]);
	}

	void Gate::inputs(const std::vector<Connection *> &in) { m_in = in; }
	Connection& Gate::rd() { return m_out; }

//___________________________________________________________________________________
// A buffer takes a weak high impedence input and outputs a strong signal
	ABuffer::ABuffer(Connection &in, const std::string &a_name):
			Gate({&in}, false, a_name) {
	}
	Connection &ABuffer::rd() { return Gate::rd(); }

//___________________________________________________________________________________
// Inverts a high impedence input and outputs a signal
	Inverter::Inverter(Connection &in, const std::string &a_name):
			Gate({&in}, true, a_name) {
	}
	Connection &Inverter::rd() { return Gate::rd(); }

//___________________________________________________________________________________
//  And gate, also nand for invert=true, or possibly doubles as buffer or inverter
	void AndGate::recalc() {
		std::vector<Connection *> &in = inputs();

		if (!in.size()) return;
		bool sig = in[0]->signal();
		for (size_t n = 1; n < in.size(); ++n) {
			sig = sig & in[n]->signal();
		}
		rd().set_value((inverted() ^ sig) * Vdd, false);
	}

	AndGate::AndGate(const std::vector<Connection *> &in, bool inverted, const std::string &a_name)
	: Gate(in, inverted, a_name){}

	AndGate::~AndGate() {}

//___________________________________________________________________________________
//  Or gate, also nor for invert=true, or possibly doubles as buffer or inverter
	void OrGate::recalc() {
		std::vector<Connection *> &in = inputs();
		if (!in.size()) return;
		if (debug()) std::cout << (inverted()?"N":"") << "OR." << name() << "(";
		bool sig = in[0]->signal();
		if (debug()) std::cout << in[0]->name() << "[" << sig << "]";
		for (size_t n = 1; n < in.size(); ++n) {
			sig = sig | in[n]->signal();
			if (debug()) std::cout << ", " << in[n]->name()  << "[" << in[n]->signal() << "]";
		}
		if (debug()) std::cout << ") = " << (inverted() ^ sig) << std::endl;
		rd().set_value((inverted() ^ sig) * Vdd, false);
	}

	OrGate::OrGate(const std::vector<Connection *> &in, bool inverted, const std::string &a_name)
	: Gate(in, inverted, a_name) {}
	OrGate::~OrGate() {}


//___________________________________________________________________________________
//  XOr gate, also xnor for invert=true, or possibly doubles as buffer or inverter
	void XOrGate::recalc() {
		std::vector<Connection *> &in = inputs();
		if (!in.size()) return;

		bool sig = in[0]->signal();
		if (debug()) std::cout << sig;
		for (size_t n = 1; n < in.size(); ++n) {
			if (debug()) std::cout << "^" << in[n]->signal();
			sig = sig ^ in[n]->signal();
		}
		if (debug()) std::cout << " = " << sig << std::endl;
		rd().set_value((inverted() ^ sig) * Vdd, false);
	}

	XOrGate::XOrGate(const std::vector<Connection *> &in, bool inverted, const std::string &a_name)
	: Gate(in, inverted, a_name) {}
	XOrGate::~XOrGate() {}

//___________________________________________________________________________________
// A wire can be defined as a collection of connections
//
//   Connections may be impeded (high impedance) or unimpeded (low impedence).
//   Impeded connections are treated as outputs (to the wire), whilst impeded
//  connections are treated as inputs.
//
//   This may seem backward, but it's a wire, and current flowing from an unimpeded
//  connection into a wire raises a voltage potential in the wire, which we can
//  reflect on the impeded output connections.
//
//   To calculate the voltage on a wire, we find the lowest input voltage for all
//  unimpeded (input) connections.  We can then update the voltage for all
//  impeded (output) connections.
//
//   If a wire has no unimpeded connections, the voltage on the wire is indeterminate.


	float Wire::recalc() {
		indeterminate = true;
		if (debug()) std::cout << "read wire " << name() << ": [";
		float sum_conductance = 0;
		float sum_v_over_R = 0;
		for (auto conn = connections.begin(); conn != connections.end(); ++conn) {
			if (debug()) {
				std::cout << (conn == connections.begin()?"":", ");
				std::cout << (*conn)->name();
			}
			float v = (*conn)->rd(false);
			if ((*conn)->impeded())  {
				if (debug()) std::cout << "[o]: ";
			} else {
				indeterminate = false;
				float iR = (*conn)->conductance();
				if (debug()) std::cout << "[i]: ";
				sum_conductance += iR;
				sum_v_over_R += v * iR;
			}
		}
		float V = Vss;
		if (not indeterminate) {
			m_sum_conductance = sum_conductance;
			m_sum_v_over_R = sum_v_over_R;
			V = sum_v_over_R/sum_conductance;
		}
		if (debug()) {
			if (indeterminate) {
				std::cout << "] is indeterminate" << std::endl;
			} else {
				std::cout << "] = " << V << "v" << std::endl;
			}
		}
		return V;
	}

	bool Wire::assert_voltage() {  // fires signals to any impeded connections
		float V = recalc();
		bool changed = Voltage != V;
		if (debug()) {
			if (!indeterminate) {
				std::cout << "Wire: " << name() << " is at " << (indeterminate?"unknown":"");
				std::cout << V << "v";
				std::cout << std::endl;
			}
			std::cout << name() << ": changing Voltage from " << Voltage << " to " << V << std::endl;
		}
		Voltage = V;
		for (auto conn = connections.begin(); conn != connections.end(); ++conn) {
			if ((*conn)->impeded()) {
				if (indeterminate)
					(*conn)->determinate(false);
				else {
					(*conn)->set_value(V, true);
				}
			} else if (not indeterminate) {
				(*conn)->set_vdrop(V);
			}
		}
		return changed;
	}

	void Wire::queue_change(){  // Add a voltage change event to the queue
		if (assert_voltage()) {        // determine wire voltage and update impeded connections
			eq.queue_event(new DeviceEvent<Wire>(*this, "Wire Voltage Change"));
			eq.process_events();
		}
	}

	void Wire::on_connection_change(Connection *conn, const std::string &name, const std::vector<BYTE> &data) {
		if (debug()) {
			const char *direction = " <-- ";
			if (conn->impeded()) direction = " ->| ";
			std::cout << "Wire " << this->name() << direction << "Event " << conn->name() << " changed to " << conn->rd() << "V [";
			std::cout << (conn->impeded()?"o":"i") << "]" << std::endl;
		}
		queue_change();
	}

	Wire::Wire(const std::string &a_name): Device(a_name), indeterminate(true), Voltage(0), m_sum_conductance(0), m_sum_v_over_R(0) {}

	Wire::Wire(Connection &from, Connection &to, const std::string &a_name): Device(a_name), indeterminate(true), Voltage(0) {
		connect(from); connect(to);
	}

	Wire::~Wire() {
		eq.remove_events_for(this);
		for (auto conn=connections.begin(); conn != connections.end(); ++conn) {
			DeviceEvent<Connection>::unsubscribe<Wire>(this, &Wire::on_connection_change, *conn);
		}
	}

	void Wire::connect(Connection &connection, const std::string &a_name) {
		connections.push_back(&connection);
		if (a_name.length()) connection.name(a_name);
		DeviceEvent<Connection>::subscribe<Wire>(this, &Wire::on_connection_change, &connection);
		queue_change();
	}
	void Wire::disconnect(const Connection &connection) {
		for (auto conn=connections.begin(); conn != connections.end(); ++conn) {
			if (&(**conn) == &connection) {
				DeviceEvent<Connection>::unsubscribe<Wire>(this, &Wire::on_connection_change,
						const_cast<Connection *>(&connection));
				connections.erase(conn);
				break;
			}
		}
		queue_change();
	}

	float Wire::rd(bool include_vdrop) {
		if (debug()) std::cout << name() << ": rd() = " << Voltage << std::endl;
		return Voltage;
	}

	bool Wire::determinate() { return !indeterminate; }
	bool Wire::signal() {
		float V = rd();
		return V > Vdd/2.0;
	}

//___________________________________________________________________________________
// A buffer whos output impedence depends on a third state signal

	void Tristate::pr_debug_info(const std::string &what) {
		std::cout << "  *** on " << what << ":" << this->name();
		std::cout << " gate=" << ((m_gate->signal()^m_invert_gate)?"high":"low") << ": output set to " << (m_out.impeded()?"high impedence":"");
		if (!m_out.impeded()) std::cout << m_out.rd();
		std::cout << std::endl;
	}


	void Tristate::recalc_output() {
		bool impeded = !m_gate->signal();
		bool out = m_in->signal();
		if (m_invert_output) out = !out;
		if (m_invert_gate) impeded = !impeded;
		if (impeded) out = false;
		m_out.set_value(out * Vdd, impeded);
	}

	void Tristate::on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc_output();
		if (debug()) pr_debug_info("input change");
	}

	void Tristate::on_gate_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc_output();
		if (debug()) pr_debug_info("gate change");
	}

	Tristate::Tristate(Connection &in, Connection &gate, bool invert_gate, bool invert_output, const std::string &a_name):
		Device(a_name), m_in(&in), m_gate(&gate), m_out(name()+"::out"), m_invert_gate(invert_gate), m_invert_output(invert_output) {
		DeviceEvent<Connection>::subscribe<Tristate>(this, &Tristate::on_change, m_in);
		DeviceEvent<Connection>::subscribe<Tristate>(this, &Tristate::on_gate_change, m_gate);

		m_out.name(name()+".out");
		recalc_output();
	}
	Tristate::~Tristate() {
		DeviceEvent<Connection>::unsubscribe<Tristate>(this, &Tristate::on_change, m_in);
		DeviceEvent<Connection>::unsubscribe<Tristate>(this, &Tristate::on_gate_change, m_gate);
	}
	void Tristate::set_name(const std::string &a_name) {
		name(a_name);
		m_out.name(name()+".output");
	};
	bool Tristate::signal() const { return m_out.signal(); }
	bool Tristate::impeded() const { return m_out.impeded(); }
	bool Tristate::inverted() const { return m_invert_output; }
	bool Tristate::gate_invert() const { return m_invert_gate; }

	Tristate& Tristate::inverted(bool v) { m_invert_output = v; recalc_output(); return *this; }
	Tristate& Tristate::gate_invert(bool v) { m_invert_gate = v; recalc_output(); return *this; }

	void Tristate::wr(float in) { m_in->set_value(in, true); }
	void Tristate::input(Connection &in) {
		DeviceEvent<Connection>::unsubscribe<Tristate>(this, &Tristate::on_change, m_in);
		m_in = &in; recalc_output();
		DeviceEvent<Connection>::subscribe<Tristate>(this, &Tristate::on_change, m_in);
		if (debug()) pr_debug_info("input replaced");
	}
	void Tristate::gate(Connection &gate) {
		DeviceEvent<Connection>::unsubscribe<Tristate>(this, &Tristate::on_gate_change, m_gate);
		m_gate = &gate;
		DeviceEvent<Connection>::subscribe<Tristate>(this, &Tristate::on_gate_change, m_gate);
		recalc_output();
		if (debug()) pr_debug_info("gate replaced");
	}

	Connection& Tristate::input() { return *m_in; }
	Connection& Tristate::gate() { return *m_gate; }
	Connection& Tristate::rd() { return m_out; }

//___________________________________________________________________________________
// Clamps a voltabe between a lower and upper bound.

	void Clamp::on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		if (!m_in) return;
		float in = m_in->rd();
		bool changed = false;
		if (in < m_lo) {
			in = m_lo; changed = true;
		}
		if (in > m_hi) {
			in = m_hi; changed = true;
		}
		if (changed)
			m_in->set_value(in, m_in->impeded());
	}

	Clamp::Clamp(Connection &in, float vLow, float vHigh):
		m_in(&in), m_lo(vLow), m_hi(vHigh) {
		DeviceEvent<Connection>::subscribe<Clamp>(this, &Clamp::on_change, m_in);
	};
	Clamp::~Clamp() {
		if (m_in) DeviceEvent<Connection>::unsubscribe<Clamp>(this, &Clamp::on_change, m_in);
	}

	void Clamp::reclamp(Connection &in) {
		if (m_in) DeviceEvent<Connection>::unsubscribe<Clamp>(this, &Clamp::on_change, m_in);
		m_in = &in;
		DeviceEvent<Connection>::subscribe<Clamp>(this, &Clamp::on_change, m_in);
	}
	void Clamp::unclamp(){
		if (m_in) DeviceEvent<Connection>::unsubscribe<Clamp>(this, &Clamp::on_change, m_in);
		m_in = NULL;
	}

//___________________________________________________________________________________
// A relay, such as a reed relay.  A signal applied closes the relay.  Functionally,
// this is almost identical to a Tristate.

	void Relay::recalc_output() {
		if (m_sw == NULL || m_in == NULL)
			return;
		bool impeded = !m_sw->signal(); // open circuit if not signal
		float out = m_in->rd();
		m_out.set_value(out, impeded);
	}

	void Relay::on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc_output();
	}

	void Relay::on_sw_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc_output();
	}

	Relay::Relay(Connection &in, Connection &sw, const std::string &a_name):
		Device(a_name), m_in(&in), m_sw(&sw), m_out(name()+"::out") {
		recalc_output();
		DeviceEvent<Connection>::subscribe<Relay>(this, &Relay::on_change, m_in);
		DeviceEvent<Connection>::subscribe<Relay>(this, &Relay::on_sw_change, m_sw);
	}
	Relay::~Relay() {
		if (m_in) DeviceEvent<Connection>::unsubscribe<Relay>(this, &Relay::on_change, m_in);
		if (m_sw) DeviceEvent<Connection>::unsubscribe<Relay>(this, &Relay::on_sw_change, m_sw);
	}
	bool Relay::signal() { return m_out.signal(); }
	void Relay::in(Connection &in) {
		if (m_in) DeviceEvent<Connection>::unsubscribe<Relay>(this, &Relay::on_change, m_in);
		m_in = &in;
		DeviceEvent<Connection>::subscribe<Relay>(this, &Relay::on_change, m_in);
	}
	void Relay::sw(Connection &sw) {
		if (m_sw) DeviceEvent<Connection>::unsubscribe<Relay>(this, &Relay::on_change, m_sw);
		m_sw = &sw;
		DeviceEvent<Connection>::subscribe<Relay>(this, &Relay::on_change, m_sw);
	}

	Connection& Relay::in() { return *m_in; }
	Connection& Relay::sw() { return *m_sw; }
	Connection& Relay::rd() { return m_out; }


//___________________________________________________________________________________
// A generalised D flip flop or a latch, depending on how we use it

	void Latch::on_clock_change(Connection *Ck, const std::string &name, const std::vector<BYTE> &data) {
		if (!m_D) return;
		if (debug()) std::cout << this->name() << ": Ck is " << Ck->signal() << std::endl;
		if ((m_positive ^ (!Ck->signal()))) {
			m_Q.set_value(m_D->rd());
			if (debug()) std::cout << this->name() << ": Q was set to " << m_Q.rd() << std::endl;
		}
	}

	void Latch::on_data_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		if (!m_Ck) return;
		if (m_positive ^ (!m_Ck->signal())) {
			if (debug()) std::cout << this->name() << ": D is " << D->signal() << std::endl;
			m_Q.set_value(D->rd());
		}
	}

	Latch::Latch(Connection &D, Connection &Ck, bool positive, bool clocked):
		m_D(&D), m_Ck(&Ck), m_Q(Vss), m_Qc(m_Q), m_positive(positive), m_clocked(clocked) {
		DeviceEvent<Connection>::subscribe<Latch>(this, &Latch::on_clock_change, m_Ck);
		if (!m_clocked)
			DeviceEvent<Connection>::subscribe<Latch>(this, &Latch::on_data_change, m_D);
	}
	Latch::~Latch() {
		if (m_Ck) DeviceEvent<Connection>::unsubscribe<Latch>(this, &Latch::on_clock_change, m_Ck);
		if (!m_clocked && m_D != NULL)
			DeviceEvent<Connection>::unsubscribe<Latch>(this, &Latch::on_data_change, m_D);
	}

	void Latch::D(Connection &a_D) {
		if (!m_clocked && m_D != NULL)
			DeviceEvent<Connection>::unsubscribe<Latch>(this, &Latch::on_data_change, m_D);
		m_D = &a_D;
		if (!m_clocked)
			DeviceEvent<Connection>::subscribe<Latch>(this, &Latch::on_data_change, m_D);
	}
	void Latch::Ck(Connection &a_Ck) {
		if (m_Ck) DeviceEvent<Connection>::unsubscribe<Latch>(this, &Latch::on_clock_change, m_Ck);
		m_Ck = &a_Ck;
		DeviceEvent<Connection>::subscribe<Latch>(this, &Latch::on_clock_change, m_Ck);
	}

	void Latch::positive(bool a_positive) {
		m_positive = a_positive;
	}
	void Latch::clocked(bool a_clocked) {
		if (!m_clocked && m_D != NULL)
			DeviceEvent<Connection>::unsubscribe<Latch>(this, &Latch::on_data_change, m_D);
		m_clocked = a_clocked;
		if (!m_clocked && m_D != NULL)
			DeviceEvent<Connection>::subscribe<Latch>(this, &Latch::on_data_change, m_D);
	}

	bool Latch::clocked() { return m_clocked; }
	void Latch::set_name(const std::string &a_name) {
		name(a_name);
		m_Q.name(a_name+"::Q");
		m_Qc.name(a_name+"::Qc");
	}

	Connection& Latch::D() {return *m_D; }
	Connection& Latch::Ck() {return *m_Ck; }
	Connection& Latch::Q() {return m_Q; }
	Connection& Latch::Qc() {return m_Qc; }


//___________________________________________________________________________________
//  A multiplexer.  The "select" signals are bits which make up an index into the input.
//    multiplexers can route both digital and analog signals.
	void Mux::calculate_select() {
		m_idx = 0;
		for (int n = m_select.size()-1; n >= 0; --n) {
			m_idx = m_idx << 1;
			m_idx |= (m_select[n]->signal()?1:0);
		}
		if (m_select.size() > 1)
			std::cout << "Select is " << (int)m_idx << "; size is " << m_select.size() << std::endl;
		if (m_idx >= m_in.size()) throw(name() + std::string(": Multiplexer index beyond input bounds"));
	}

	void Mux::set_output() {
		float value = m_in[m_idx]->rd();
		if (debug()) std::cout << "MUX." << name() << " sel(" << (int)m_idx << ") = " << value << std::endl;
		m_out.set_value(value);
	}

	void Mux::on_change(Connection *D, const std::string &name) {
		if (D == m_in[m_idx])     // only pays attention to the selected input
			set_output();
	}

	void Mux::on_select(Connection *D, const std::string &name) {
		calculate_select();
		set_output();
	}

	Mux::Mux(const std::vector<Connection *> &in, const std::vector<Connection *> &select, const std::string &a_name):
		Device(a_name), m_in(in), m_select(select), m_out(Vdd),  m_idx(0) {
		if (m_select.size() > 8) throw(name() + ": Mux supports a maximum of 8 bits, or 256 inputs");
		for (size_t n = 0; n < m_in.size(); ++n) {
			DeviceEvent<Connection>::subscribe<Mux>(this, &Mux::on_change, m_in[n]);
		}
		for (size_t n = 0; n < m_select.size(); ++n) {
			DeviceEvent<Connection>::subscribe<Mux>(this, &Mux::on_select, m_select[n]);
		}
		m_out.name(a_name + "::out");
		calculate_select();
		set_output();
	}
	Mux::~Mux() {
		for (size_t n = 0; n < m_in.size(); ++n) {
			DeviceEvent<Connection>::unsubscribe<Mux>(this, &Mux::on_change, m_in[n]);
		}
		for (size_t n = 0; n < m_select.size(); ++n) {
			DeviceEvent<Connection>::unsubscribe<Mux>(this, &Mux::on_select, m_select[n]);
		}
	}
	Connection& Mux::in(int n) { return *m_in[n]; }
	Connection& Mux::select(int n) { return *m_select[n]; }
	Connection& Mux::rd() { return m_out; }
	size_t Mux::no_inputs() { return m_in.size(); }
	size_t Mux::no_selects() { return m_select.size(); }


//______________________________________________________________
// A FET approximation (voltage controlled switch).
	void FET::recalc() {
		bool active = m_gate.signal() ^ (not m_is_nType);

		float in = m_in.rd(), out = m_out.rd();

		if (!m_in.determinate()) in = out;
		if (!m_out.determinate()) out = in;

		if (debug()) {
			std::cout << (m_is_nType?"n":"p");
			std::cout << "FET: " << name() << "; in=" << in << "; out=" << out << " gate signal=" << m_gate.signal();
			std::cout << std::endl;
		}

		if (active) {
			m_out.conductance(m_in.conductance());
			m_in.set_value(in, false);
			m_out.set_value(in, false);
		} else {
			m_in.set_value(in, true);
			m_out.set_value(in, true);
		}
	}

	void FET::on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc();
	}

	void FET::on_output_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		bool active = m_gate.signal() ^ (not m_is_nType);
		if (active) m_in.set_vdrop(m_out.rd());
	}

	FET::FET(Connection &in, Connection &gate, bool is_nType, bool dbg):
	m_in(in), m_gate(gate), m_is_nType(is_nType) {
		debug(dbg);
		DeviceEvent<Connection>::subscribe<FET>(this, &FET::on_change, &m_in);
		DeviceEvent<Connection>::subscribe<FET>(this, &FET::on_change, &m_gate);
		DeviceEvent<Connection>::subscribe<FET>(this, &FET::on_output_change, &m_out);
		recalc();
	}
	FET::~FET() {
		DeviceEvent<Connection>::unsubscribe<FET>(this, &FET::on_change, &m_in);
		DeviceEvent<Connection>::unsubscribe<FET>(this, &FET::on_change, &m_gate);
		DeviceEvent<Connection>::unsubscribe<FET>(this, &FET::on_output_change, &m_out);
	}
	const Connection& FET::in() const { return m_in; }
	const Connection& FET::gate() const { return m_gate; }
	Connection& FET::rd() { return m_out; }


//___________________________________________________________________________________
// Prevents a jittering signal from toggling between high/low states.

	void Schmitt::recalc() {
		//  inv   sig   en    ( xor function )
		//  0     0     0
		//  0     1     1
		//  1     0     1
		//  1     1     0
		bool enabled = m_gate_invert ^ m_enable.signal();
		if (debug()) std::cout << "Schmitt: " << name() << ": enabled=" << (enabled?"true":"false");
		float Vout = m_out.rd();
		if (!enabled)     // high impedence
			m_out.set_value(Vss, true);
		else {
			double Vin = m_in.rd();
			if ((Vin > m_hi && Vout < m_hi) || (Vin < m_lo && Vout > m_lo)) {
				Vout = m_out_invert?(!m_in.signal())*Vdd:m_in.signal()*Vdd;
				if (debug()) std::cout << " : Vin=" << Vin << " : Vout=" << Vout;
				m_out.set_value(Vout, false);
			} else if ((Vin > m_hi && Vout > m_hi) || (Vin < m_lo && Vout < m_lo)) {
				Vout = m_out_invert?(!m_in.signal())*Vdd:m_in.signal()*Vdd;
				if (debug()) std::cout << " : Vin=" << Vin << " : Vout=" << Vout;
				m_out.set_value(Vout, false);
			} else if (debug())
				std::cout << " : Vin=" << Vin;
		}
		if (debug()) std::cout << std::endl;
	}

	void Schmitt::on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc();
	}

	Schmitt::Schmitt(Connection &in, Connection &en, bool impeded, bool gate_invert, bool out_invert):
		m_in(in), m_enable(en), m_out(Vss, impeded), m_gate_invert(gate_invert), m_out_invert(out_invert) {
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, &m_in);
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, &m_enable);
		recalc();
	}
	Schmitt::Schmitt(Connection &in, bool impeded, bool out_invert):
		m_in(in), m_enable(m_enabled), m_out(Vss, impeded), m_gate_invert(false), m_out_invert(out_invert) {
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, &m_in);
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, &m_enable);
		m_enable.set_value(Vdd, true);     // No enable signal, so always true
		recalc();
	}
	Schmitt::~Schmitt() {
		DeviceEvent<Connection>::unsubscribe<Schmitt>(this, &Schmitt::on_change, &m_in);
		DeviceEvent<Connection>::unsubscribe<Schmitt>(this, &Schmitt::on_change, &m_enable);
	}

	void Schmitt::gate_invert(bool invert) { m_gate_invert = invert; recalc(); }
	void Schmitt::out_invert(bool invert) { m_out_invert = invert; recalc(); }

	Connection& Schmitt::in() { return m_in; }
	Connection& Schmitt::en() { return m_enable; }
	Connection& Schmitt::rd() { return m_out; }


//___________________________________________________________________________________
// Trace signals.  One GUI representation will be a graphical signal tracer.

	void SignalTrace::crop(time_stamp current_ts) {
		time_stamp start_ts = current_ts - m_duration_us;
		for (auto &c: m_values) {
			auto &q = m_times[c];
			while (q.size()) {
				auto &data = q.front();
				if (data.ts > start_ts) {
//					std::cout << c->name() << ": " << q.size() << " elements remain in queue" << std::endl;
					break;
				}
				m_initial[c] = data.v;
				q.pop();
			}
		}
	}

	void SignalTrace::on_connection_change(Connection *c, const std::string &name, const std::vector<BYTE> &data) {
		time_stamp current_ts = current_time_us();
		crop(current_ts);
		auto &q = m_times[c];
		q.push(DataPoint(current_ts, c->rd()));
	}

	SignalTrace::SignalTrace(const std::vector<Connection *> &in, const std::string &a_name) : Device(a_name), m_values(in) {
		duration_us(20000000);        // 20s (real time seconds)
		for (auto v: m_values) {
			m_initial[v] = v->rd();
			DeviceEvent<Connection>::subscribe<SignalTrace>(this, &SignalTrace::on_connection_change, v);
		}
	}

	SignalTrace::~SignalTrace() {
		for (auto v: m_values) {
			DeviceEvent<Connection>::unsubscribe<SignalTrace>(this, &SignalTrace::on_connection_change, v);
		}
	}

	// returns a collated map of map<connection*, queue<datapoint> where each connection has an equal queue length.
	// and all columns are for the same time stamp.
	std::map<Connection *, std::queue<SignalTrace::DataPoint> > SignalTrace::collate() {
		std::map<Connection *, float > l_initial = m_initial;
		std::map<Connection *, std::queue<DataPoint> > l_times = m_times;
		std::map<Connection *, std::queue<DataPoint> > l_collated;

		while (true) {
			bool l_found = false;
			time_stamp lowest_ts;
			for (auto c: m_values) {
				if (not l_times[c].empty()) {
					if (not l_found) {
						l_found = true;
						lowest_ts = l_times[c].front().ts;
					} else if (lowest_ts > l_times[c].front().ts) {
						lowest_ts = l_times[c].front().ts;
					}
				}
			}

			if (not l_found)
				return l_collated;   // all done

			for (auto c: m_values) {  // add a column
				if (not l_times[c].empty()) {
					auto &data = l_times[c].front();
					if (lowest_ts == data.ts) {
						l_initial[c] = data.v;
						l_times[c].pop();
					}
				}
				l_collated[c].push(DataPoint(lowest_ts, l_initial[c]));
			}
		}
	}

	time_stamp SignalTrace::current_us() const { return current_time_us(); }
	time_stamp SignalTrace::first_us() const {
		bool first = true;
		time_stamp ts = current_us();
		for (auto &row: m_times ) {
			if (!row.second.empty()) {
				if (first) {
					ts = row.second.front().ts;
					first = false;
				} else {
					auto &data = row.second.front();
					ts = ts > data.ts ? data.ts : ts;
				}
			}
		}
		return ts;
	}

	void SignalTrace::duration_us(long a_duration_us) {
		m_duration_us = std::chrono::duration<long, std::micro>(a_duration_us);
	}

	const std::vector<Connection *> & SignalTrace::traced() { return m_values; }


//___________________________________________________________________________________
// A binary counter.  If clock is set, it is synchronous, otherwise asynch ripple.
	void Counter::on_signal(Connection *c, const std::string &name, const std::vector<BYTE> &data) {
		if (not m_clock) {            // enabled
			m_overflow = false;
			if (c->signal() ^ (not m_rising)) {
				m_value = m_value + 1;     // asynchronous ripple counter
				if (m_value & (1 << m_bits.size())) {
					m_value = 0;
					m_overflow = true;
				}
				set_value(m_value);
			}
		} else {
			m_signal = c->signal();
		}
	}

	// synchronous counter on clock signal
	void Counter::on_clock(Connection *c, const std::string &name, const std::vector<BYTE> &data) {
		eq.process_events();
		if (c->signal() ^ (not m_rising)) {   // rising clock
			m_overflow = false;
			m_value = m_value + (m_signal?1:0);   // clock current input signal into the bus
			if (m_value & (1 << m_bits.size())) {
				m_value = 0;
				m_overflow = true;
			}
			m_signal = false;       // triggered
			set_value(m_value);
		}
	}

	Counter::Counter(unsigned int nbits, unsigned long a_value): Device(),
			m_in(m_dummy), m_clock(NULL), m_rising(true), m_signal(true) {
		assert(nbits < sizeof(a_value) * 8);
		m_bits.resize(nbits);
		set_value(a_value);
	}

	Counter::Counter(const Connection &a_in, bool rising, size_t nbits, unsigned long a_value, const Connection *a_clock):
		Device(), m_in(a_in), m_clock(a_clock), m_rising(rising), m_signal(true) {
		assert(nbits < sizeof(a_value) * 8);
		m_bits.resize(nbits);
		set_value(a_value);

		DeviceEvent<Connection>::subscribe<Counter>(this, &Counter::on_signal, &m_in);
		if (m_clock) {
			DeviceEvent<Connection>::subscribe<Counter>(this, &Counter::on_clock, m_clock);
		}
	}

	Counter::~Counter() {
		if (&m_in != &m_dummy)
			DeviceEvent<Connection>::unsubscribe<Counter>(this, &Counter::on_signal, &m_in);
		if (m_clock) {
			DeviceEvent<Connection>::unsubscribe<Counter>(this, &Counter::on_clock, m_clock);
		}
	}

	void Counter::set_name(const std::string &a_name) {
		name(a_name);
		for (size_t n = 0; n < m_bits.size(); ++n) {
			m_bits[n].name(a_name + int_to_hex(n, "::"));
		}
	}

	void Counter::set_value(unsigned long a_value) {
		m_value = a_value;
		for (size_t n = 0; n < m_bits.size(); ++n) {
			m_bits[n].set_value(((a_value & 1) != 0) * Vdd, true);
			a_value >>= 1;
		}
	}

	Connection & Counter::bit(size_t n) {
		assert(n < m_bits.size());
		return m_bits[n];
	}

	std::vector<Connection *> Counter::databits() {
		std::vector<Connection *> l_bits;  // we use "pointer to connection" so we can easily plug into other components
		for (auto &bit: m_bits) {
			l_bits.push_back(&bit);
		}
		return l_bits;
	}


