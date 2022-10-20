#include <sstream>
#include "device_base.h"
#include "../utils/utility.h"
//_______________________________________________________________________________________________
//  Event queue static definitions
bool DeviceEventQueue::debug = false;
std::mutex DeviceEventQueue::mtx;
std::queue< SmartPtr<QueueableEvent> >DeviceEventQueue::events;
Connection Simulation::m_clock;
double Simulation::m_speed = 1.0;

LockUI DeviceEventQueue::m_ui_lock(false);

template <class T> class
	DeviceEvent<T>::registry  DeviceEvent<T>::subscribers;

// The hardware architecture uses several sequential logic components.  It would be
// really nice to be able to emulate the way the hardware works by stringing together
// elements and reacting to events that cause status to change.
//
// Here's a basic connection.  Changes to connection values always spawn and
// process device events.
//
// Note:  Store conductance (1/R) instead of R, because we use it way more often.
//

	void Slot::set_total_R(double a_total_R, double Gin, double Iin, double Idrop) {
		Connection *c = dynamic_cast<Connection *>(connection);

		double resistance_for = 1/Gin;
		double R = a_total_R + resistance_for;
//		if (not float_equiv(total_R, R)) {
			total_R = R;
			c->set_total_R();
			double resistance_ratio = total_R?(resistance_for / total_R):0;
			double Vin = c->rd(false);
			vdrop = -Vin * resistance_ratio;
			c->set_vdrop();

			std::cout << connection->name() << " -> " << dev->name();
			std::cout << ": recalc_total_R();  G=" << Gin << ", R=" << a_total_R;
			std::cout << "; total_R = " << total_R;
			std::cout << "; Vin=" << Vin;
			std::cout << "; Ratio=" << resistance_ratio;
			std::cout << "; Vdrop=" << vdrop;
			std::cout << std::endl;
//		}
	}

	void Slot::set_impeded() {
		Connection *c = dynamic_cast<Connection *>(connection);
		total_R = 1e+12;
		vdrop = 0;
		c->set_vdrop();
	}

	bool Slot::impeded() {
		return total_R >= 1e+12;
	}

	double Slot::calc_conductance() {
		double Gin, Iin, Idrop;
		dev->calc_conductance_precedents(Gin, Iin, Idrop);
		return Gin;
	}

	double Slot::calc_resistance_ratio() {
		Connection *c = dynamic_cast<Connection *>(connection);
		recalc_total_R();
		return c->R() / total_R;
	}

	double Slot::resistance_for() {
		double G = calc_conductance();
		return 1/G;
	}

	void Slot::recalc_total_R() {
		dev->recalc_total_R();
	}

	Slot::Slot(Device *a_dev, Device *a_connection): dev(a_dev), connection(a_connection) {}

	Slot *Connection::slot(Device *d){
		Slot *l_slot = new Slot(d, this);
		m_slots.insert(l_slot);
		queue_change(false, ": New slot");
		return l_slot;
	}

	bool Connection::unslot(Slot *slot_id){
		if (m_slots.find(slot_id) == m_slots.end()) return false;
		m_slots.erase(slot_id); delete slot_id;
		reset_vdrop();
		return true;
	}

	void Connection::queue_change(bool process_q, const std::string &a_comment){  // Add a voltage change event to the queue
		std::string detail = name() + std::string(": Voltage Change") + a_comment;
		eq.queue_event(new DeviceEvent<Connection>(*this, detail));
		if (debug()) std::cout << detail << (process_q?": process_queue":"") << std::endl;
//		if (process_q) eq.process_events();
	}

	bool Connection::impeded_suppress_change(bool a_impeded) {
		if (m_impeded != a_impeded) {
			if (debug())
				std::cout << "Connection " << name() << ": impeded " << m_impeded << " -> " << a_impeded << std::endl;
			m_impeded = a_impeded;
			if (m_impeded) reset_vdrop();
			return true;
		}
		return false;
	}

	void Connection::calc_conductance_antecedents(double &Gout, double &Iout) {
		Gout = 0;  Iout = 0;
		for (auto slot: m_slots)
			if (not slot->impeded()) {
				Iout +=  slot->vdrop * conductance();
				Gout += conductance();
			}
	}

	void Connection::set_total_R() {}   // at top of chain

	bool Connection::recalc_total_R() {  // traverses all chains to end, then does set_total_R
		if (not m_slots.size()) return false;
		for (auto &slot: m_slots)
			slot->recalc_total_R();
		return true;
	}

	void Connection::set_vdrop() {
		double old_vdrop = m_vdrop;
		m_vdrop = 0;
		for (auto slot: m_slots)
			if (fabs(slot->vdrop) > fabs(m_vdrop))
				m_vdrop = slot->vdrop;
		if (not float_equiv(m_vdrop, old_vdrop, 1.0e-12)) {
			queue_change(true, std::string(": Vdrop changed from ") + as_text(old_vdrop) + " to " + as_text(m_vdrop));
		}
	}

	void Connection::reset_vdrop() {
		bool changed = false;
		for (auto slot: m_slots) {
			changed = changed || slot->vdrop != 0;
			slot->vdrop = 0;
		}
//		if (changed)
//			queue_change(false);
	}

	Connection::Connection(const std::string &a_name):
		Device(a_name), m_V(Vss), m_conductance(1.0e+4), m_impeded(true), m_determinate(false) {
	};

	Connection::Connection(double V, bool impeded, const std::string &a_name):
		Device(a_name), m_V(V), m_conductance(1.0e+4), m_impeded(impeded), m_determinate(true) {
	};

	Connection::~Connection() {
		unslot_all_slots();
		eq.remove_events_for(this);
	}

	double Connection::rd(bool include_vdrop) const {
		double val = m_V + include_vdrop * vDrop();
		if (false and debug()) {
			if (include_vdrop)
				std::cout << "Connection " << name() << ": read value ["<< m_V << " + " << vDrop() << "] = " << val << std::endl;
			else
				std::cout << "Connection " << name() << ": read value = " << val << std::endl;
		}
		return val;
	}

	bool Connection::signal() const { return rd(true) > Vdd/2.0; }
	bool Connection::impeded() const { return m_impeded; }
	bool Connection::determinate() const { return m_determinate || !impeded(); }
	double Connection::vDrop() const { return m_vdrop; }

	void Connection::impeded(bool a_impeded) {
		if (impeded_suppress_change(a_impeded)) queue_change(true, ": impeded status");
	}

	void Connection::determinate(bool on) {
//		if (debug())
//			std::cout << name() << ": determinate " << m_determinate << " -> " << on << std::endl;
		m_determinate = on;
	}

	void Connection::conductance(double iR) {  // set internal resistanve
		if (iR >= min_R && iR <= max_R) {
			m_conductance = iR;
		} else if (iR < min_R) {
			impeded(true);
		}
	}

	void Connection::R(double a_R) {  // set internal resistance
		conductance(a_R>0?1/a_R:1/max_R);
	}

	double Connection::conductance() const {  // internal resistance
		return m_conductance;
	}

	double Connection::R() const {  // internal resistanve
		double c = conductance();
		return  c>0?1/c:max_R;
	}

	double Connection::I() const {  // maximum current given V & R
		return vDrop() * conductance();
	}

	std::string Connection::info() 	{
		std::ostringstream l_info;
		l_info << Device::info() << std::endl;
		if (impeded()) {
			l_info << "Impeded = true" << std::endl;
			l_info << "V = " << unit_text(rd(), "V") << std::endl;
		} else {
			l_info << "Vin = " << unit_text(rd(false), "V") << std::endl;
			l_info << "Vdrop = " << unit_text(vDrop(), "V") << std::endl;
			l_info << "Vout = " << unit_text(rd(true), "V") << std::endl;
			l_info << "R = " << unit_text(R(), "Ω") << std::endl;
			l_info << "I = " << unit_text(I(), "A") << std::endl;
			l_info << "P = " << unit_text(I()*vDrop(), "W") << std::endl;
		}

		return l_info.str();
	}

	void Connection::set_value(double V, bool a_impeded) {
		if (!float_equiv(m_V, V, 1e-4) || m_impeded != a_impeded || !determinate()) {
			determinate(true);     // the moment we change the value of a connection the value is determined
			impeded_suppress_change(a_impeded);
//			m_V = V - vDrop();   // vOut = V + vDrop
			m_V = V;
			if (!impeded()) reset_vdrop();
//			if (debug())
//				std::cout << "Connection " << name() << ": set value V = " << V << std::endl;
			queue_change(true, std::string(": set_value=") + as_text(V));
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
			double sum_conductance = 0;
			double sum_v_over_R = 0;
			double sum_vdrop_over_R = 0;

			calc_conductance_precedents(sum_conductance, sum_v_over_R, sum_vdrop_over_R);

			m_terminal_impeded =
					std::isnan(sum_v_over_R) or
					std::isnan(sum_vdrop_over_R) or
					std::isnan(sum_conductance) or
					(sum_conductance == 0);

			double input_voltage = rd(false);
			if (not m_terminal_impeded) {
				if (false and debug()) {
					std::cout << name() << ": Itotal=" << sum_v_over_R;
					std::cout << "; Gtotal=" << sum_conductance;
					std::cout << "; VdropTotalI=" << sum_vdrop_over_R;
					std::cout << std::endl;
				}

				input_voltage = calculate_voltage(sum_v_over_R, sum_vdrop_over_R, sum_conductance);
				Connection::set_value(input_voltage, Connection::impeded());
			}

//			if (debug()) std::cout << name() << ": input_voltage=" << input_voltage << std::endl;
			for (auto &c : m_connects ) {
				if (c.first->impeded()) {           // set voltage on outputs
					c.first->set_value(input_voltage, true);
				}
			}
		}
	}

	void Terminal::calc_conductance_precedents(double &Gin, double &Iin, double &Idrop) {
		Gin = 0;  Iin = 0; Idrop = 0;
		for (auto &c : m_connects ) {
			Gin += c.first->conductance();
			Iin += c.first->rd(false) * c.first->conductance();
			Idrop += c.second->vdrop * c.first->conductance();
		}
	}

	void Terminal::set_total_R() {
		double Gout=0, Iout=0;
		double Gin=0, Iin=0, Idrop=0;
		calc_conductance_antecedents(Gout, Iout);
		if (not Gout) m_terminal_impeded = true;

		if (impeded()) {
			for (auto &c : m_connects )
				c.second->set_impeded();
		} else {
			double total_R = R() + Gout?1/Gout:0;
			calc_conductance_precedents(Gin, Iin, Idrop);
			for (auto &c : m_connects )
				c.second->set_total_R(total_R, Gin, Iin, Idrop);
		}
	}

	// Vc is sum(V/R).  Ci is total conductance.
	//
	// For a voltage source, we can add or remove voltage here; for
	// resistors, we can add conductance.
	double Terminal::calculate_voltage(double Iin, double Idrop, double Gin) {
		double V = Iin / Gin;         // V = sum(Vi/Ri) * sum(1/Ri)  ; i = 1..n
		return V;       // the calculated voltage becomes our input voltage
	}

	void Terminal::on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc();
		recalc_total_R();
	}

	bool Terminal::recalc_total_R() {
		if (not Connection::recalc_total_R()) {  // no more outgoing; turn it around
			if (impeded()) {
				for (auto &c : m_connects )    // last in chain
					c.second->set_impeded();
			} else {
				double Gin, Iin, Idrop;
				calc_conductance_precedents(Gin, Iin, Idrop);
				for (auto &c : m_connects )    // last in chain
					c.second->set_total_R(R(), Gin, Iin, Idrop);
			}
		}
		return true;
	}

	bool Terminal::unslot(void *slot_id){
		for (auto &c : m_connects ) {
			if (c.second == (Slot *)slot_id) {
				DeviceEvent<Connection>::unsubscribe<Terminal>(this, &Terminal::on_change, c.first);
				m_connects.erase(c.first);
				recalc_total_R();
				return true;
			}
		}
		return false;
	}

	Terminal::Terminal(const std::string name): Connection(name), m_connects(),
			m_terminal_impeded(true) {
	}
	Terminal::Terminal(double V, const std::string name): Connection(V, true, name), m_connects(),
			m_terminal_impeded(true) {
	}
	Terminal::~Terminal() {
		for (auto &c : m_connects ) {
			DeviceEvent<Connection>::unsubscribe<Terminal>(this, &Terminal::on_change, c.first);
			c.second->unslot();
		}
	}

	bool Terminal::connect(Connection &c) {
		if (m_connects.find(&c) == m_connects.end()) {
			m_connects[&c] = c.slot(this);
			DeviceEvent<Connection>::subscribe<Terminal>(this, &Terminal::on_change, &c);
			return true;
		} else {
			disconnect(c);
			return false;
		}
	}

	void Terminal::disconnect(Connection &c) {
		if (m_connects.find(&c) != m_connects.end()) {
			DeviceEvent<Connection>::unsubscribe<Terminal>(this, &Terminal::on_change, &c);
			if (c.unslot(m_connects[&c]))
				m_connects.erase(&c);
			recalc_total_R();
		}
	}

	void Terminal::set_value(double V, bool a_impeded) {
		// NOTE: When our actual internal voltage is being determined by a pool of connections with at
		//       least one input, we cannot (should not) override the internal voltage by using
		//       set_value().   We can achieve the same effect as set_value() by setting default
		//       direction, and calling set_vdrop() instead.
		if (!m_terminal_impeded) {
			Connection::impeded_suppress_change(a_impeded);
			double drop = vDrop();
//			for (auto &conn: m_connects)
//				conn.second->set_vdrop(V);
			if (not float_equiv(drop, vDrop())) queue_change(false);
		} else {
			Connection::set_value(V, a_impeded);
		}
	}

	void Terminal::impeded(bool a_impeded) { Connection::impeded(a_impeded); }
	bool Terminal::impeded() const { if (m_connects.size()) return m_terminal_impeded; return Connection::impeded(); }
	double Terminal::rd(bool include_vdrop) const {
		return Connection::rd(include_vdrop);
	}

	//___________________________________________________________________________________
	// A capacitor.  A time frequency driven analog component.
	// Changes in voltage are recalculated periodically, as determined by a signal
	// from Simulation::clock().

	// Vc is sum(V/R).  Ci is total conductance.
	double Capacitor::calculate_voltage(double Ic, double IdropC, double Ci) {
		auto ts = current_time_us();
		auto dT = ((ts - m_T).count() / 1000000.0) * Simulation::speed();   // dT in seconds
		if (dT > 0.01)  { // 10 ms steps.  We don't want to flood the message queue!
			double R = 1/Ci;          // Resistance into Cap
			double tau = R * m_F;     // Time constant  RC
			double dI = IdropC * (1 - exp(-dT / tau));     //  Current in over time period

			if (not float_equiv(dI, 0, Ic / 10000)) {
				double Vc = Ic * R;       // Voltage at input when fully charged
				double Vr = rd(false);    // voltage drop
				double dV = -dI * R;      // IR = V - qR / RC:  V = IR + qR/RC
				m_T = ts;
				double V = dV + Vr;
				m_R = V / (dI - IdropC);

				if (debug()) {
					std::cout << "R=" << R+Connection::R();
					std::cout << "; dT=" << dT;
					std::cout << "; tau=" << tau;
					std::cout << "; I=" << m_I;
					std::cout << "; Idrop=" << IdropC;
					std::cout << "; Vc=" << Vc << "; Cap dV=" << dV << "[" << rd(false) + dV << "]" << std::endl;
				}
				return V;
			}
		}
		return rd(false);
	}

	void Capacitor::reset() {
		set_value(0, false);
		m_T = current_time_us();
		m_I = m_R = 0;
	}

	bool Capacitor::connect(Connection &c) {
		reset();
		return Terminal::connect(c);
	}

	void Capacitor::on_clock(Connection *c, const std::string &a_name, const std::vector<BYTE>&a_data) {
		recalc();
		recalc_total_R();
	}
	double Capacitor::F() { return m_F; }
	void Capacitor::F(double a_F) { m_F = a_F; }

	Capacitor::Capacitor(double V, const std::string &a_name): Terminal(V, a_name) {
		m_F = 1e-6;
		reset();
		DeviceEvent<Connection>::subscribe<Capacitor>(this, &Capacitor::on_clock, &Simulation::clock());
	};
	Capacitor::Capacitor(const std::string name): Terminal(name) {
		m_F = 1e-6;
		reset();
		DeviceEvent<Connection>::subscribe<Capacitor>(this, &Capacitor::on_clock, &Simulation::clock());
	};
	Capacitor::~Capacitor() {
		DeviceEvent<Connection>::unsubscribe<Capacitor>(this, &Capacitor::on_clock, &Simulation::clock());
	}

	//___________________________________________________________________________________
	// An Inductor.  A time frequency driven analog component.

	// Vc is sum(V/R).  Ci is total conductivity.
	double Inductor::calculate_voltage(double Ic, double IdropC, double Ci) {
		auto ts = current_time_us();
		auto dT = ((ts - m_T).count() / 1000000.0) * Simulation::speed();   // dT in seconds
		if (dT > 0.01)  { // 10 ms steps.  We don't want to flood the message queue!
			double R = 1/Ci;
			double tau =  m_H / R;                  // time constant L/R
			double Vc = Ic * R;                     // Voltage at input of R
			double Vr = (Ic+IdropC) * R;            // Current Voltage

			double V = Vr * exp(-dT / tau);
			double dV = Vr - V;

			if (not float_equiv(dV, 0, Vc / 10000)) {
				double dI = dV * R;
//				m_I = dI - IdropC;
				m_I = m_I - IdropC + dI;
				m_R = V / m_I - 1/Connection::conductance();
				m_T = ts;
				if (debug()) {
					std::cout << "tau=" << tau;
					std::cout << "; Vc=" << Vc;
					std::cout << "; Vr=" << Vr;
					std::cout << "; R=" << R;
					std::cout << "; Rout=" << Connection::R();
					std::cout << "; dT=" << dT;
					std::cout << "; dV=" << dV;
					std::cout << "; dI=" << dI;
					std::cout << "; V=" << V;
					std::cout << "; I=" << m_I << std::endl;
				}
				return V;
			}
		}
		return rd(false);
	}


	void Inductor::reset() {
		m_R = 1e+6;
		m_I = -1e-6;
		m_T = current_time_us();
	}

	bool Inductor::connect(Connection &c) {
		reset();
		return Terminal::connect(c);
	}

	void Inductor::on_clock(Connection *c, const std::string &a_name, const std::vector<BYTE>&a_data) {
		recalc();
		recalc_total_R();
	}
	double Inductor::H() { return m_H; }
	void Inductor::H(double a_H) { m_H = a_H; }

	Inductor::Inductor(double V, const std::string &a_name): Terminal(V, a_name) {
		m_H = 1e-2;
		reset();
		DeviceEvent<Connection>::subscribe<Inductor>(this, &Inductor::on_clock, &Simulation::clock());
	};
	Inductor::Inductor(const std::string name): Terminal(name) {
//		debug(true);
		m_H = 1e-2;
		reset();
		DeviceEvent<Connection>::subscribe<Inductor>(this, &Inductor::on_clock, &Simulation::clock());
	};
	Inductor::~Inductor() {
		DeviceEvent<Connection>::unsubscribe<Inductor>(this, &Inductor::on_clock, &Simulation::clock());
	}


//___________________________________________________________________________________
// Voltage source.   Always constant, no matter what.  Overrides connection V.
	Voltage::Voltage(double V, const std::string name): Terminal(V, name) {}

	bool Voltage::impeded() const { return false; }
//	double Voltage::rd(bool include_vdrop) const { return m_voltage + include_vdrop * vDrop(); }


//___________________________________________________________________________________
// A Weak Voltage source.  If there are any unimpeded connections, we calculate the
// lowest unimpeded, otherwise the highest impeded connection as the voltage
	PullUp::PullUp(double V, const std::string name): Connection(V, false, name) {
		R(1.0e+6);
	}

	bool PullUp::impeded() const { return false; }


//___________________________________________________________________________________
// Ground.  Always zero.
	Ground::Ground(): Voltage(0, "GND") {}

	bool Ground::impeded() const { return false; }
	double Ground::rd(bool include_vdrop) const { return Vss; }


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

	void Inverse::set_value(double V, bool a_impeded) { c.set_value(V, a_impeded); }
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

	Output::Output(): Terminal(), c(*this), wrapper(false) {}
	Output::Output(Connection &a_c): Terminal(), c(a_c), wrapper(true) {
		DeviceEvent<Connection>::subscribe<Output>(this, &Output::on_connection_change, &c);
	}
	Output::Output(double V, const std::string &a_name): Terminal(V, a_name), c(*this), wrapper(false) {};
	Output::~Output() {
		if (wrapper)
			DeviceEvent<Connection>::unsubscribe<Output>(this, &Output::on_connection_change, &c);
	}

	bool Output::signal() const { if (wrapper) return c.signal(); return Connection::signal(); }
	double Output::rd(bool include_vdrop) const { if (wrapper) return c.rd(); return Connection::rd(); }
	bool Output::impeded() const { return true; }
	bool Output::determinate() const { return true; }

	void Output::set_value(double V, bool a_impeded) { if (wrapper) c.set_value(V, false); else Connection::set_value(V, false); }
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
	Input::Input(double V, const std::string &a_name): Connection(V, false, a_name), c(*this), wrapper(false) {};
	Input::~Input() {
		if (wrapper)
			DeviceEvent<Connection>::unsubscribe<Input>(this, &Input::on_connection_change, &c);
	}

	bool Input::signal() const { if (wrapper) return c.signal(); return Connection::signal(); }
	double Input::rd(bool include_vdrop) const { if (wrapper) return c.rd(); return Connection::rd(); }
	bool Input::impeded() const { return true; }
	bool Input::determinate() const { if (wrapper) return c.determinate(); return Connection::determinate(); }

	void Input::set_value(double V, bool a_impeded) { if (wrapper) c.set_value(V, true); else Connection::set_value(V, true); }
	void Input::impeded(bool a_impeded) { if (wrapper) c.impeded(true); else Connection::impeded(true); }
	void Input::determinate(bool on) { if (wrapper) c.determinate(on); else Connection::determinate(on); }

//___________________________________________________________________________________
//  Generic gate,
	void Gate::recalc() {
		std::vector<Connection *> &in = inputs();
		if (!in.size()) return;
		bool sig = in[0]?in[0]->signal():false;
		rd().set_value((inverted() ^ sig) * Vdd, false);
	}

	void Gate::on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		if (debug()) {
			std::cout << "Gate " << this->name() << " received event " << name << std::endl;
		}
		recalc();
	}

	Gate::Gate(const std::vector<Connection *> &in, bool inverted, const std::string &a_name):
		Device(a_name), m_in(in), m_out(Vdd), m_inverted(inverted) {
		for (size_t n = 0; n < m_in.size(); ++n)
			if (m_in[n]) DeviceEvent<Connection>::subscribe<Gate>(this, &Gate::on_change, m_in[n]);
		recalc();
	}

	Gate::~Gate() {
		for (size_t n = 0; n < m_in.size(); ++n)
			if (m_in[n]) DeviceEvent<Connection>::unsubscribe<Gate>(this, &Gate::on_change, m_in[n]);
	}
	bool Gate::connect(size_t a_pos, Connection &in) {
		if (a_pos+1 > m_in.size()) m_in.resize(a_pos+1);
		if (m_in[a_pos]) DeviceEvent<Connection>::unsubscribe<Gate>(this, &Gate::on_change, m_in[a_pos]);
		m_in[a_pos] = &in;
		if (m_in[a_pos]) DeviceEvent<Connection>::subscribe<Gate>(this, &Gate::on_change, m_in[a_pos]);
		recalc();
		return true;
	}

	void Gate::disconnect(size_t a_pos) {
		if (a_pos+1 > m_in.size()) return;
		if (m_in[a_pos]) DeviceEvent<Connection>::unsubscribe<Gate>(this, &Gate::on_change, m_in[a_pos]);
		m_in[a_pos] = NULL;
		recalc();
	}

	void Gate::inputs(const std::vector<Connection *> &in) { m_in = in; }
	Connection& Gate::rd() { return m_out; }

//___________________________________________________________________________________
// A buffer takes a weak high impedence input and outputs a strong signal
	ABuffer::ABuffer(Connection &in, const std::string &a_name):
			Gate({&in}, false, a_name) {
	}

//___________________________________________________________________________________
// Inverts a high impedence input and outputs a signal
	Inverter::Inverter(Connection &in, const std::string &a_name):
			Gate({&in}, true, a_name) {
	}

//___________________________________________________________________________________
//  And gate, also nand for invert=true, or possibly doubles as buffer or inverter
	void AndGate::recalc() {
		std::vector<Connection *> &in = inputs();

		if (!in.size()) return;
		bool sig = in[0]?in[0]->signal():false;
		for (size_t n = 1; n < in.size(); ++n) {
			if (in[n]) sig = sig && in[n]->signal();
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
		bool sig = in[0]?in[0]->signal():false;
		if (debug()) std::cout << in[0]->name() << "[" << sig << "]";
		for (size_t n = 1; n < in.size(); ++n) {
			if (in[n]) sig = sig || in[n]->signal();
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

		bool sig = in[0]?in[0]->signal():false;
		if (debug()) std::cout << sig;
		for (size_t n = 1; n < in.size(); ++n) {
			if (debug()) std::cout << "^" << in[n]->signal();
			if (in[n]) sig = sig ^ in[n]->signal();
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


	double Wire::recalc() {
		indeterminate = true;
		if (debug()) std::cout << "read wire " << name() << ": [";
		double sum_conductance = 0;
		double sum_v_over_R = 0;
		for (auto conn = connections.begin(); conn != connections.end(); ++conn) {
			if (debug()) {
				std::cout << (conn == connections.begin()?"":", ");
				std::cout << conn->first->name();
			}
			double v = conn->first->rd(false);
			if (conn->first->impeded())  {
				if (debug()) std::cout << "[o]: ";
			} else {
				indeterminate = false;
				double iR = conn->first->conductance();
				if (debug()) std::cout << "[i]: ";
				sum_conductance += iR;
				sum_v_over_R += v * iR;
			}
		}
		double V = Vss;
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
		double V = recalc();
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
			if (conn->first->impeded()) {
				if (indeterminate)
					conn->first->determinate(false);
				else {
					conn->first->set_value(V, true);
				}
			} else if (not indeterminate) {
				conn->first->set_vdrop();
//				conn->second->calculate_vdrop(V);
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
		for (auto &conn: connections) {
			DeviceEvent<Connection>::unsubscribe<Wire>(this, &Wire::on_connection_change, conn.first);
		}
	}

	bool Wire::connect(Connection &connection, const std::string &a_name) {
		connections[&connection] = connection.slot(this);
		if (a_name.length()) connection.name(a_name);
		DeviceEvent<Connection>::subscribe<Wire>(this, &Wire::on_connection_change, &connection);
		queue_change();
		return true;
	}
	void Wire::disconnect(const Connection &connection) {
		for (auto conn=connections.begin(); conn != connections.end(); ++conn) {
			if (conn->first == &connection) {
				DeviceEvent<Connection>::unsubscribe<Wire>(this, &Wire::on_connection_change,
						const_cast<Connection *>(&connection));
				if (conn->first->unslot(conn->second))
					connections.erase(conn);
				break;
			}
		}
		queue_change();
	}

	double Wire::rd(bool include_vdrop) {
		if (debug()) std::cout << name() << ": rd() = " << Voltage << std::endl;
		return Voltage;
	}

	bool Wire::determinate() { return !indeterminate; }
	bool Wire::signal() {
		double V = rd();
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
		bool impeded = m_gate?!m_gate->signal():false;
		bool out = m_in?m_in->signal():false;
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
		if (m_in) DeviceEvent<Connection>::unsubscribe<Tristate>(this, &Tristate::on_change, m_in);
		if (m_gate) DeviceEvent<Connection>::unsubscribe<Tristate>(this, &Tristate::on_gate_change, m_gate);
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

	void Tristate::wr(double in) { m_in->set_value(in, true); }
	void Tristate::input(Connection *in) {
		if (m_in) DeviceEvent<Connection>::unsubscribe<Tristate>(this, &Tristate::on_change, m_in);
		m_in = in; recalc_output();
		if (m_in) DeviceEvent<Connection>::subscribe<Tristate>(this, &Tristate::on_change, m_in);
		if (debug()) pr_debug_info("input replaced");
	}
	void Tristate::gate(Connection * gate) {
		if (m_gate) DeviceEvent<Connection>::unsubscribe<Tristate>(this, &Tristate::on_gate_change, m_gate);
		m_gate = gate;
		if (m_gate) DeviceEvent<Connection>::subscribe<Tristate>(this, &Tristate::on_gate_change, m_gate);
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
		double in = m_in->rd();
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

	Clamp::Clamp(Connection &in, double vLow, double vHigh):
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
		double out = m_in->rd();
		m_out.set_value(impeded?0:out, impeded);
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
	void Relay::in(Connection *in) {
		if (m_in) DeviceEvent<Connection>::unsubscribe<Relay>(this, &Relay::on_change, m_in);
		m_in = in;
		if (m_in)DeviceEvent<Connection>::subscribe<Relay>(this, &Relay::on_change, m_in);
	}
	void Relay::sw(Connection *sw) {
		if (m_sw) DeviceEvent<Connection>::unsubscribe<Relay>(this, &Relay::on_change, m_sw);
		m_sw = sw;
		if (m_sw) DeviceEvent<Connection>::subscribe<Relay>(this, &Relay::on_change, m_sw);
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

	void Latch::D(Connection *a_D) {
		if (!m_clocked && m_D != NULL)
			DeviceEvent<Connection>::unsubscribe<Latch>(this, &Latch::on_data_change, m_D);
		m_D = a_D;
		if (!m_clocked)
			DeviceEvent<Connection>::subscribe<Latch>(this, &Latch::on_data_change, m_D);
	}
	void Latch::Ck(Connection *a_Ck) {
		if (m_Ck) DeviceEvent<Connection>::unsubscribe<Latch>(this, &Latch::on_clock_change, m_Ck);
		m_Ck = a_Ck;
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
			m_idx |= (m_select[n] && m_select[n]->signal()?1:0);
		}
		if (m_select.size() > 1)
			std::cout << "Select is " << (int)m_idx << "; size is " << m_select.size() << std::endl;
		if (m_idx >= m_in.size()) throw(name() + std::string(": Multiplexer index beyond input bounds"));
	}

	void Mux::set_output() {
		double value = m_in[m_idx]?m_in[m_idx]->rd():0;
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

	void Mux::in(int n, Connection *c) {
		if (m_in[n]) DeviceEvent<Connection>::unsubscribe<Mux>(this, &Mux::on_change, m_in[n]);
		m_in[n] = c;
		if (m_in[n]) DeviceEvent<Connection>::subscribe<Mux>(this, &Mux::on_change, m_in[n]);
	}

	void Mux::select(int n, Connection *c) {
		if (m_select[n]) DeviceEvent<Connection>::unsubscribe<Mux>(this, &Mux::on_select, m_select[n]);
		m_select[n] = c;
		if (m_select[n]) DeviceEvent<Connection>::subscribe<Mux>(this, &Mux::on_select, m_select[n]);
	}

	void Mux::configure(int input_count, int gate_count) {
		unsubscribe_all();
		m_in.resize(input_count);
		m_select.resize(gate_count);
		subscribe_all();
		calculate_select();
		set_output();
	}

	void Mux::subscribe_all() {
		for (size_t n = 0; n < m_in.size(); ++n) {
			if (m_in[n]) DeviceEvent<Connection>::subscribe<Mux>(this, &Mux::on_change, m_in[n]);
		}
		for (size_t n = 0; n < m_select.size(); ++n) {
			if (m_select[n]) DeviceEvent<Connection>::subscribe<Mux>(this, &Mux::on_select, m_select[n]);
		}
	}

	void Mux::unsubscribe_all() {
		for (size_t n = 0; n < m_in.size(); ++n) {
			if (m_in[n]) DeviceEvent<Connection>::unsubscribe<Mux>(this, &Mux::on_change, m_in[n]);
		}
		for (size_t n = 0; n < m_select.size(); ++n) {
			if (m_select[n]) DeviceEvent<Connection>::unsubscribe<Mux>(this, &Mux::on_select, m_select[n]);
		}
	}

	Mux::Mux(const std::vector<Connection *> &in, const std::vector<Connection *> &select, const std::string &a_name):
		Device(a_name), m_in(in), m_select(select), m_out(Vdd),  m_idx(0) {
		if (m_select.size() > 8) throw(name() + ": Mux supports a maximum of 8 bits, or 256 inputs");
		subscribe_all();
		m_out.name(a_name + "::out");
		calculate_select();
		set_output();
	}
	Mux::~Mux() { unsubscribe_all(); }
	Connection& Mux::in(int n) { return *m_in[n]; }
	Connection& Mux::select(int n) { return *m_select[n]; }
	Connection& Mux::rd() { return m_out; }
	size_t Mux::no_inputs() { return m_in.size(); }
	size_t Mux::no_selects() { return m_select.size(); }


//______________________________________________________________
// A FET approximation (voltage controlled switch).
	void FET::recalc() {
		bool active = m_gate.signal() ^ (not m_is_nType);

		double in = m_in.rd(), out = m_out.rd();

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
		//		if (active) m_in_slot->calculate_vdrop(m_out.rd());
		if (active) m_in.set_vdrop();
	}

	FET::FET(Connection &in, Connection &gate, bool is_nType, bool dbg):
	m_in(in), m_gate(gate), m_is_nType(is_nType) {
		m_in_slot = m_in.slot(this);
		debug(dbg);
		DeviceEvent<Connection>::subscribe<FET>(this, &FET::on_change, &m_in);
		DeviceEvent<Connection>::subscribe<FET>(this, &FET::on_change, &m_gate);
		DeviceEvent<Connection>::subscribe<FET>(this, &FET::on_output_change, &m_out);
		recalc();
	}
	FET::~FET() {
//		if (m_in_slot) m_in.unslot(m_in_slot);
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

		bool enabled = m_enable?m_gate_invert ^ m_enable->signal():false;
		if (debug()) std::cout << "Schmitt: " << name() << ": enabled=" << (enabled?"true":"false");
		double Vout = m_out.rd();
		if (!enabled)     // high impedence
			m_out.set_value(Vss, true);
		else if (m_in) {
			double Vin = m_in->rd();
			if ((Vin > m_hi && Vout < m_hi) || (Vin < m_lo && Vout > m_lo)) {
				Vout = m_out_invert?(!m_in->signal())*Vdd:m_in->signal()*Vdd;
				if (debug()) std::cout << " : Vin=" << Vin << " : Vout=" << Vout;
				m_out.set_value(Vout, false);
			} else if ((Vin > m_hi && Vout > m_hi) || (Vin < m_lo && Vout < m_lo)) {
				Vout = m_out_invert?(!m_in->signal())*Vdd:m_in->signal()*Vdd;
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

	Schmitt::Schmitt(): m_in(NULL), m_enable(NULL), m_out(Vss, false), m_gate_invert(false), m_out_invert(false) {
		debug(true);
	}

	Schmitt::Schmitt(Connection &in, Connection &en, bool impeded, bool gate_invert, bool out_invert):
		m_in(&in), m_enable(&en), m_out(Vss, impeded), m_gate_invert(gate_invert), m_out_invert(out_invert) {
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, m_in);
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, m_enable);
		recalc();
	}
	Schmitt::Schmitt(Connection &in, bool impeded, bool out_invert):
		m_in(&in), m_enable(&m_enabled), m_out(Vss, impeded), m_gate_invert(false), m_out_invert(out_invert) {
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, m_in);
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, m_enable);
		m_enable->set_value(Vdd, true);     // No enable signal, so always true
		recalc();
	}
	Schmitt::~Schmitt() {
		DeviceEvent<Connection>::unsubscribe<Schmitt>(this, &Schmitt::on_change, m_in);
		DeviceEvent<Connection>::unsubscribe<Schmitt>(this, &Schmitt::on_change, m_enable);
	}

	void Schmitt::gate_invert(bool invert) { m_gate_invert = invert; recalc(); }
	void Schmitt::out_invert(bool invert) { m_out_invert = invert; recalc(); }

	void Schmitt::set_input(Connection *in) {
		if (m_in) DeviceEvent<Connection>::unsubscribe<Schmitt>(this, &Schmitt::on_change, m_in);
		m_in = in;
		if (m_in) DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, m_in);
	};

	void Schmitt::set_gate(Connection *en) {
		if (m_enable) DeviceEvent<Connection>::unsubscribe<Schmitt>(this, &Schmitt::on_change, m_enable);
		m_enable = en;
		if (m_enable) DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, m_enable);
	};

	Connection& Schmitt::in() { return *m_in; }
	Connection& Schmitt::en() { return *m_enable; }
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

	bool SignalTrace::add_trace(Connection *c, int a_posn) {
		if ((size_t)a_posn > m_values.size()) return false;
		m_values.insert(m_values.begin()+a_posn, c);
		m_initial[c] = c->rd();
		DeviceEvent<Connection>::subscribe<SignalTrace>(this, &SignalTrace::on_connection_change, c);
		return true;
	}

	bool SignalTrace::has_trace(Connection *c) {
		for (size_t n=0; n < m_values.size(); ++n)
			if (m_values[n] == c)
				return true;
		return false;
	}

	void SignalTrace::remove_trace(Connection *c) {
		for (size_t n=0; n < m_values.size(); ++n)
			if (m_values[n] == c) {
				DeviceEvent<Connection>::unsubscribe<SignalTrace>(this, &SignalTrace::on_connection_change, c);
				m_values.erase(m_values.begin()+n);
				m_initial.erase(c);
				m_times.erase(c);
			}
	}

	int SignalTrace::slot_id(int a_id) {
		return a_id;
	}

	bool SignalTrace::unslot(void *slot_id) {
		return false;
	}

	void SignalTrace::clear_traces() {
		for (auto c: m_values)
			DeviceEvent<Connection>::unsubscribe<SignalTrace>(this, &SignalTrace::on_connection_change, c);
		m_values.clear();
		m_initial.clear();
		m_times.clear();
	}

	void SignalTrace::on_connection_change(Connection *c, const std::string &name, const std::vector<BYTE> &data) {
		time_stamp current_ts = current_time_us();
		crop(current_ts);
		auto &q = m_times[c];
		q.push(DataPoint(current_ts, c->rd()));
	}

	SignalTrace::SignalTrace(const std::vector<Connection *> &in, const std::string &a_name) : Device(a_name), m_values(in) {
		duration_us(20000000);        // 20s (real time seconds)
		for (auto &v: m_values) {
			m_initial[v] = v->rd();
			DeviceEvent<Connection>::subscribe<SignalTrace>(this, &SignalTrace::on_connection_change, v);
		}
	}

	SignalTrace::~SignalTrace() {
		for (auto &v: m_values) {
			DeviceEvent<Connection>::unsubscribe<SignalTrace>(this, &SignalTrace::on_connection_change, v);
		}
	}

	// returns a collated map of map<connection*, queue<datapoint> where each connection has an equal queue length.
	// and all columns are for the same time stamp.
	std::map<Connection *, std::queue<SignalTrace::DataPoint> > SignalTrace::collate() {
		std::map<Connection *, double > l_initial = m_initial;
		std::map<Connection *, std::queue<DataPoint> > l_times = m_times;
		std::map<Connection *, std::queue<DataPoint> > l_collated;

		while (true) {
			bool l_found = false;
			time_stamp lowest_ts;
			for (auto &c: m_values) {
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

			for (auto &c: m_values) {  // add a column
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
// A binary counter.  If clock is set, it is synchronous, otherwise a ripple.
	void Counter::on_signal(Connection *c, const std::string &name, const std::vector<BYTE> &data) {
		if (not m_clock) {            // enabled
			m_overflow = false;
			if (m_ripple) {
				m_value = m_value + 1;     // asynchronous ripple counter
			} else if (c->signal() ^ (not m_rising)) {
				m_value = m_value + 1;     // synchronous ripple counter
			}
			if (m_value & (1 << m_bits.size())) {
				m_value = 0;
				m_overflow = true;
			}
			set_value(m_value);
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
			m_in(NULL), m_clock(NULL), m_rising(true), m_ripple(true), m_signal(true) {
		if (m_rising) m_ripple = false;
		assert(nbits < sizeof(a_value) * 8);
		m_bits.resize(nbits);
		set_value(a_value);
	}

	Counter::Counter(Connection &a_in, bool rising, size_t nbits, unsigned long a_value, Connection *a_clock):
		Device(), m_in(&a_in), m_clock(a_clock), m_rising(rising), m_ripple(true), m_signal(true) {
		if (m_rising) m_ripple = false;
		assert(nbits < sizeof(a_value) * 8);
		m_bits.resize(nbits);
		set_value(a_value);

		DeviceEvent<Connection>::subscribe<Counter>(this, &Counter::on_signal, m_in);
		if (m_clock) {
			DeviceEvent<Connection>::subscribe<Counter>(this, &Counter::on_clock, m_clock);
		}
	}

	Counter::~Counter() {
		if (m_in) DeviceEvent<Connection>::unsubscribe<Counter>(this, &Counter::on_signal, m_in);
		if (m_clock) DeviceEvent<Connection>::unsubscribe<Counter>(this, &Counter::on_clock, m_clock);
	}

	void Counter::set_input(Connection *c) {
		if (m_in) DeviceEvent<Connection>::unsubscribe<Counter>(this, &Counter::on_signal, m_in);
		m_in = c;
		if (m_in) DeviceEvent<Connection>::subscribe<Counter>(this, &Counter::on_signal, m_in);
	}

	void Counter::set_clock(Connection *c) {
		if (m_clock) DeviceEvent<Connection>::unsubscribe<Counter>(this, &Counter::on_clock, m_clock);
		m_clock = c;
		if (m_clock) DeviceEvent<Connection>::subscribe<Counter>(this, &Counter::on_clock, m_clock);
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


