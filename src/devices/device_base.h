/*
 * Define basic devices representing connections, wires and basic components which we can combine to
 * approximate the function of various ports and features provided by the PIC16f chip.
 *
 * Our mantra here, is that it is impossible to accurately simulate even a real world resistor, let
 * alone some of the more complex devices such as mosfets or BJT's in C++.  So we won't try.
 *
 * We instead build simple approximations.  We largely ignore resistance, treating it as an absolute.
 * Current flies out of the window.  We only work with voltages as far as we can get away with it,
 * and even then, everything is either at ground, at Vdd, or indeterminate.
 *
 * Here, we also implement an event queue which allows changes in circuit connections to be propagated
 * throughout the rest of the circuit.
 */
#pragma once

#include <iostream>
#include <queue>
#include <tuple>
#include <map>
#include <set>
#include <string>
#include <cstring>
#include <functional>
#include <mutex>
#include "../utils/smart_ptr.h"
#include "constants.h"

//__________________________________________________________________________________________________
// Device definitions.  A basic device may be named and have a debug flag.
class Device {
	std::string m_name;
	bool m_debug;
  public:
	static constexpr float Vss = 0.0;
	static constexpr float Vdd = 5.0;

	Device(): m_name(""), m_debug(false) {}
	Device(const Device &d): m_debug(false) {
		name(d.name());
	}
	Device(const std::string name): m_name(name), m_debug(false) {}

	Device &operator=(const Device &d) {
		name(d.name());
		return *this;
	}

	virtual ~Device() {}
	void debug(bool flag) { m_debug = flag; }
	bool debug() const { return m_debug; }
	const std::string &name() const { return m_name; }
	void name(const std::string &a_name) { m_name = a_name; }
};


//___________________________________________________________________________________
// A base class for device events that can be queued.
class QueueableEvent {
  public:
	virtual ~QueueableEvent() {}
	virtual void fire_event(bool debug=false) = 0;
	virtual bool compare(Device *d) = 0;
};

//___________________________________________________________________________________
// We can have a single event queue for all devices, and process events
// in sequence.  Events themselves are derived from a base class.
class DeviceEventQueue {
	static std::queue< SmartPtr<QueueableEvent> >events;
	static std::mutex mtx;

  public:
	static bool debug;

	void queue_event(QueueableEvent *event) {
		mtx.lock();
		events.push(event);
		mtx.unlock();
	}

	void clear() {
		while (!events.empty()) {
			events.pop();
		}
	}

	int size() {
		return events.size();
	}

	void remove_events_for(Device *d) {
		mtx.lock();
		static std::queue< SmartPtr<QueueableEvent> >tmp;
		while (!events.empty()) {
			auto event = events.front(); events.pop();
			if (!event->compare(d)) tmp.push(event);
		}
		while (!tmp.empty()) {
			auto event = tmp.front(); tmp.pop(); events.push(event);
		}

		mtx.unlock();
	}

	void process_events() {
		int n;
		try {
			for (n=0; n<100; ++n)
				if (events.empty())
					break;
				else {
					mtx.lock();
					auto event = events.front(); events.pop();
					mtx.unlock();
					event->fire_event(debug);
				}
		} catch (std::exception &e) {
			std::cout << e.what() << std::endl;
		} catch (...) {}

		if (n == 100)
			std::cout << "Possible event loop detected" << std::endl;
	}
};

//___________________________________________________________________________________
// A pub-sub for device events.  For example, when the clock triggers, we can put
// one of these events in a queue.
// There is only one list of subscribers per device.  The CPU implementation will
// traverse all known devices, and fire any outstanding events.  Whenever we change
// a connection, we also queue an event for that change, and process the queue
// directly after, effectively rippling that change through the circuit.
//
//  For example, we can do::
//    class abc {
//        void callback_for_device(Device &d, const std::string &name) {
//            ... do stoff
//        }
//    }
//
//    abc() {
//       Device mydev("this is my device");
//       DeviceEvent<Device>.subscribe<abc>(this, &callback_for_device);      // generic subscription for all Device instances
//       DeviceEvent<Device>.subscribe<abc>(this, &callback_for_device, &mydev); // specific subscription just to mydev events
//    }
//
//  The other form of callback also passes data as a vector of bytes, where the data depends on what the event produces
//
//  To queue a new event, we do:
//
//     DeviceEventQueue q;
//     q.queue_event(new EventQueue<Device>(*this, "a new event happened"))
//  or having an existing device, (eg. mydev above) we can do
//     q.queue_event(new EventQueue(&mydev, "a new event happened"))

template <class T> class DeviceEvent: public QueueableEvent {
	Device           *m_device;
	std::string       m_eventname;
	std::vector<BYTE> m_data;

	typedef std::function<void(T *device, const std::string &event, const std::vector<BYTE> &data)> Callback;

	typedef std::tuple<void *, const T *, void *> KeyType;            // Object, Source

	virtual bool compare(Device *d){ return d == m_device; }

  private:
	typedef std::map<KeyType, Callback> registry;
	typedef typename DeviceEvent<T>::registry::iterator each_subscriber;
	static registry subscribers;

  public:
	DeviceEvent(T &device, const std::string &eventname):
		m_device(&device), m_eventname(eventname), m_data(std::vector<BYTE>()) {}

	DeviceEvent(T &device, const std::string &eventname, const std::vector<BYTE> &data):
		m_device(&device), m_eventname(eventname), m_data(data) {}


	virtual void fire_event(bool debug=false) {
		for(each_subscriber s = subscribers.begin(); s!= subscribers.end(); ++s) {
			const T *instance = std::get<1>(s->first);
			if (!instance || instance == m_device) {
				try {
					T * device = dynamic_cast<T *>(m_device);
					if (device == NULL) throw(std::string("Null device"));
					if (debug)
						std::cout << "Event: " << device->name() << ": " << m_eventname << ": data[" << m_data.size() << "]" << std::endl;
					s->second(device, m_eventname, m_data);
					if (debug)
						std::cout << "Event returned" << std::endl;
				} catch (const std::string &e) {
					std::cout << "An error occurred whle processing a device event: " << e << "\n";
				} catch (std::exception &e) {
					std::cout << "Exception raised whle processing a device event: " << e.what() << "\n";
				}
			}
		}
	}

	template<class Q> static void subscribe (Q *ob,  void (Q::*callback)(T *device, const std::string &event), const T *instance = NULL) {
		using namespace std::placeholders;
		subscribers[KeyType{ob, instance, *(void **)&callback}] = std::bind(callback, ob, _1, _2);
	}


	template<class Q> static void subscribe (Q *ob,  void (Q::*callback)(T *device, const std::string &event, const std::vector<BYTE> &data), const T *instance = NULL) {
		using namespace std::placeholders;
		subscribers[KeyType{ob, instance, *(void **)&callback}] = std::bind(callback, ob, _1, _2, _3);
	}

	template<class Q> static void unsubscribe(void *ob, void (Q::*callback)(T *device, const std::string &event), T *instance = NULL) {
		auto key = KeyType{ob, instance, *(void **)&callback};
		if (subscribers.find(key) != subscribers.end()) {
			subscribers.erase(key);
		}
	}


	template<class Q> static void unsubscribe(void *ob, void (Q::*callback)(T *device, const std::string &event, const std::vector<BYTE> &data), const T *instance = NULL) {
		auto key = KeyType{ob, instance, *(void **)&callback};
		if (subscribers.find(key) != subscribers.end()) {
			subscribers.erase(key);
//		} else {
//			std::cout << "unsubscribe " << instance->name() << " Key not found!" << std::endl;
//			std::cout << "Subscriber registry size=" << subscribers.size() << std::endl;
//			std::cout << "Provided key=";
//			std::cout << "(" << ob << " : " << instance << " : " << &callback << ")" << std::endl;
//			std::cout << "registry keys:" << std::endl;
//			for(each_subscriber s = subscribers.begin(); s!= subscribers.end(); ++s) {
//				const void *l_ob = std::get<0>(s->first);
//				const T    *l_inst = std::get<1>(s->first);
//				const void *l_ref = std::get<2>(s->first);
//				std::cout << "(" << l_ob << " : " << l_inst << " : " << l_ref << ")" << std::endl;
//			}
		}
	}
};


//___________________________________________________________________________________
// The hardware architecture uses several sequential logic components.  It would be
// really nice to be able to emulate the way the hardware works by stringing together
// elements and reacting to events that cause status to change.
//
// Here's a basic connection.  Changes to connection values always spawn and
// process device events.
//
// Note:  Store conductance (1/R) instead of R because we use it way more often.
//
class Connection: public Device {
	float m_V;                   //  A voltage on the connection
	float m_conductance;         //  Internal resistance [inverse] (1/ohm)
	bool  m_impeded;             //  The connection has an infinite resistance
	bool  m_determinate;         //  We know what the value of the voltage is
	float m_vDrop;               //  The voltage drop over this connection
	DeviceEventQueue eq;

	const float min_R = 1.0e-9;
	const float max_R = 1.0e+9;

  protected:
	void queue_change(){  // Add a voltage change event to the queue
		eq.queue_event(new DeviceEvent<Connection>(*this, "Voltage Change"));
		if (debug())
			std::cout << name() << ": processing events" << std::endl;
		eq.process_events();
		if (debug())
			std::cout << name() << ": done processing events" << std::endl;
	}

	virtual float calc_voltage_drop(float out_voltage) {
		float V = rd(false);
		if (debug()) {
			std::cout << name() << ": Calculating vDrop = out_voltage["<<out_voltage << "] - V[" ;
			std::cout << V << "] = " << out_voltage - V << std::endl;
		}
		return out_voltage - V;
	}

	bool impeded_suppress_change(bool a_impeded) {
		if (m_impeded != a_impeded) {
			if (debug())
				std::cout << "Connection " << name() << ": impeded " << m_impeded << " -> " << a_impeded << std::endl;
			m_impeded = a_impeded;
			if (m_impeded) m_vDrop = 0;
			return true;
		}
		return false;
	}


  public:
	Connection(const std::string &a_name=""):
		Device(a_name), m_V(Vss), m_conductance(1.0e+4), m_impeded(true), m_determinate(false), m_vDrop(0) {};
	Connection(float V, bool impeded=true, const std::string &a_name=""):
		Device(a_name), m_V(V), m_conductance(1.0e+4), m_impeded(impeded), m_determinate(true), m_vDrop(0) {
	};

	virtual	~Connection() { eq.remove_events_for(this); }

	virtual float rd(bool include_vdrop=false) const {
		float val = m_V + include_vdrop * vDrop();
		if (debug()) {
			if (include_vdrop)
				std::cout << "Connection " << name() << ": read value ["<< m_V << " + " << vDrop() << "] = " << val << std::endl;
			else
				std::cout << "Connection " << name() << ": read value = " << val << std::endl;
		}
		return val;
	}

	float vDrop() const { return m_vDrop; }

	virtual bool signal() const { return rd(true) > Vdd/2.0; }
	virtual bool impeded() const { return m_impeded; }
	virtual bool determinate() const { return m_determinate || !impeded(); }
	virtual float set_vdrop(float out_voltage) {
		m_vDrop = calc_voltage_drop(out_voltage);
		if (debug()) std::cout << "vDrop: " << name() << "= " << m_vDrop << std::endl;
		return m_vDrop;
	}
//	virtual bool determinate() const { return m_determinate; }

	void impeded(bool a_impeded) {
		if (impeded_suppress_change(a_impeded)) queue_change();
	}
	void determinate(bool on) {
		if (debug())
			std::cout << name() << ": determinate " << m_determinate << " -> " << on << std::endl;
		m_determinate = on;
	}
	void conductance(float iR) {  // set internal resistanve
		if (iR >= min_R and iR <= max_R) {
			m_conductance = iR;
		} else if (iR < min_R) {
			impeded(true);
		}
	}
	virtual void R(float a_R) {  // set internal resistanve
		conductance(1/a_R);
	}

	virtual float conductance() const {  // internal resistanve
		return m_conductance;
	}
	virtual float R() const {  // internal resistanve
		return 1/m_conductance;
	}
	virtual float I() const {  // maximum current given V & R
		return m_vDrop * m_conductance;
	}
	virtual void set_value(float V, bool a_impeded) {
		if (m_V != V || impeded() != a_impeded || !determinate()) {
			determinate(true);     // the moment we change the value of a connection the value is determined
			impeded_suppress_change(a_impeded);
			m_V = V;
			m_vDrop = 0;
			if (debug())
				std::cout << "Connection " << name() << ": set value V = " << V << std::endl;
			queue_change();
		}
	}
};


//___________________________________________________________________________________
//   A terminal for connections.  It is impeded by default, but any
// inputs connected to the pin will allow the pin itself to become an input.
// This makes a pin function a little like a "wire", but at the same time
// it is also itself a "connection".
//   A terminal without connections is always impeded (treated as an output).

class Terminal: public Connection {
	std::set<Connection*> m_connects;
	bool m_terminal_impeded;

  protected:

	void recalc() {
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

	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc();
		for (auto &c : m_connects ) {
			if (c->impeded()) {           // set voltage on outputs
				c->set_value(rd(false), true);
			}
		}
	}

	virtual float calc_voltage_drop(float out_voltage) {
		float total_vDrop = Connection::calc_voltage_drop(out_voltage); // vDrop = out_voltage - V
		if ( not impeded() and m_connects.size()) {    // if we have inputs, update them
			for (auto &c : m_connects ) {
				if (not c->impeded()) {
					c->set_vdrop(out_voltage);
				}
			}
		}
		return total_vDrop;
	}

  public:
	Terminal(const std::string name="Pin"): Connection(name), m_connects(), m_terminal_impeded(true) {}
	Terminal(float V, const std::string name="Pin"): Connection(V, true, name), m_connects(), m_terminal_impeded(true) {}
	virtual ~Terminal() {
		for (auto &c : m_connects ) {
			DeviceEvent<Connection>::unsubscribe<Terminal>(this, &Terminal::on_change, c);
		}
	}
	void connect(Connection &c) {
		if (m_connects.find(&c) == m_connects.end()) {
			m_connects.insert(&c);
			recalc();
			DeviceEvent<Connection>::subscribe<Terminal>(this, &Terminal::on_change, &c);
		}
	}
	void disconnect(Connection &c) {
		if (m_connects.find(&c) != m_connects.end()) {
			DeviceEvent<Connection>::unsubscribe<Terminal>(this, &Terminal::on_change, &c);
			m_connects.erase(&c);
			recalc();
		}
	}

	virtual void set_value(float V, bool a_impeded) {
		// NOTE: When our actual internal voltage is being determined by a pool of connections with at
		//       least one input, we cannot (should not) override the internal voltage by using
		//       set_value().   We can achieve the same effect as set_value() by setting default
		//       direction, and calling set_vdrop() instead.
		if (m_connects.size() and not m_terminal_impeded) {
			Connection::impeded_suppress_change(a_impeded);
			float drop = vDrop();
			set_vdrop(V);
			if (drop != vDrop()) queue_change();
		} else {
			Connection::set_value(V, a_impeded);
		}
	}

	void impeded(bool a_impeded) { Connection::impeded(a_impeded); }
	virtual bool impeded() const { if (m_connects.size()) return m_terminal_impeded; return Connection::impeded(); }
	virtual float rd(bool include_vdrop=true) const {
		return Connection::rd(include_vdrop);
	}
};


//___________________________________________________________________________________
// Voltage source.   Always constant, no matter what.  Overrides connection V.
class Voltage: public Terminal {
	float m_voltage;
public:
	Voltage(float V, const std::string name="Vin"): Terminal(V, name), m_voltage(V) {}

	virtual bool impeded() const { return false; }
	virtual float rd(bool include_vdrop=true) const { return m_voltage; }
};

//___________________________________________________________________________________
// A Weak Voltage source.  If there are any unimpeded connections, we calculate the
// lowest unimpeded, otherwise the highest impeded connection as the voltage
class PullUp: public Connection {
public:
	PullUp(float V, const std::string name="Vin"): Connection(V, false, name) {
		R(1.0e+6);
	}

	virtual bool impeded() const { return false; }
};

//___________________________________________________________________________________
// Ground.  Always zero.
class Ground: public Voltage {
public:
	Ground(): Voltage(0, "GND") {}

	virtual bool impeded() const { return false; }
	virtual float rd(bool include_vdrop=true) const { return Vss; }
};


//___________________________________________________________________________________
// An inverted connection.  If the connection was high, we return low, and vica versa.
class Inverse: public Connection {
	Connection &c;

	virtual void on_connection_change(Connection *c, const std::string &name, const std::vector<BYTE> &data)  {
		queue_change();
	}

  public:

	Inverse(Connection &a_c): Connection(), c(a_c) {
		DeviceEvent<Connection>::subscribe<Inverse>(this, &Inverse::on_connection_change, &c);
	}
	virtual  ~Inverse() {
		DeviceEvent<Connection>::unsubscribe<Inverse>(this, &Inverse::on_connection_change, &c);
	}

	virtual bool signal() const { return !c.signal(); }
	virtual float rd(bool include_vdrop=true) const { return signal()?Vdd:Vss; }
	virtual bool impeded() const { return c.impeded(); }
	virtual bool determinate() const { return c.determinate() || !impeded(); }

	virtual void set_value(float V, bool a_impeded) { c.set_value(V, a_impeded); }
	void impeded(bool a_impeded) { c.impeded(a_impeded); }
	void determinate(bool on) { c.determinate(on); }
};

//___________________________________________________________________________________
// Allows us to treat a connection as an output by setting impedence low.  An impeded
// connection has an arbitrary high resistance, while an unimpeded connection has
// an arbitrarily low resistance (a current source).
// In this digital model, we don't particularly care what the resistance is, or how
// much current flows.

class Output: public Connection {
	Connection &c;
	bool wrapper;

	virtual void on_connection_change(Connection *c, const std::string &name, const std::vector<BYTE> &data)  {
		queue_change();
	}

  public:
	Output(): Connection(), c(*this), wrapper(false) {}
	Output(Connection &a_c): Connection(), c(a_c), wrapper(true) {
		DeviceEvent<Connection>::subscribe<Output>(this, &Output::on_connection_change, &c);
	}
	Output(float V, const std::string &a_name=""): Connection(V, false, a_name), c(*this), wrapper(false) {};
	virtual ~Output() {
		if (wrapper)
			DeviceEvent<Connection>::unsubscribe<Output>(this, &Output::on_connection_change, &c);
	}

	virtual bool signal() const { if (wrapper) return c.signal(); return Connection::signal(); }
	virtual float rd(bool include_vdrop=true) const { if (wrapper) return c.rd(); return Connection::rd(); }
	virtual bool impeded() const { return false; }
	virtual bool determinate() const { return true; }

	virtual void set_value(float V, bool a_impeded=true) { if (wrapper) c.set_value(V, false); else Connection::set_value(V, false); }
	void impeded(bool a_impeded) { if (wrapper) c.impeded(false); else Connection::impeded(false); }
	void determinate(bool on) { if (wrapper) c.determinate(true); else Connection::determinate(true); }
};


//___________________________________________________________________________________
// An output connection is unimpeded- basically, this means that multiple inputs
// will allow current to flow between them.  Using Output() and Input() for the
// same Connection(), we can control exactly how we want to define components.  For
// example, an AND gate will have one or more Inputs with an unimpeded Output.

class Input: public Connection {
	Connection &c;
	bool wrapper;

	virtual void on_connection_change(Connection *c, const std::string &name, const std::vector<BYTE> &data)  {
		queue_change();
	}

  public:
	Input(): Connection(), c(*this), wrapper(false) {}
	Input(Connection &a_c): Connection(), c(a_c), wrapper(true) {
		DeviceEvent<Connection>::subscribe<Input>(this, &Input::on_connection_change, &c);
	}
	Input(float V, const std::string &a_name=""): Connection(V, false, a_name), c(*this), wrapper(false) {};
	virtual ~Input() {
		if (wrapper)
			DeviceEvent<Connection>::unsubscribe<Input>(this, &Input::on_connection_change, &c);
	}

	virtual bool signal() const { if (wrapper) return c.signal(); return Connection::signal(); }
	virtual float rd(bool include_vdrop=true) const { if (wrapper) return c.rd(); return Connection::rd(); }
	virtual bool impeded() const { return true; }
	virtual bool determinate() const { if (wrapper) return c.determinate(); return Connection::determinate(); }

	virtual void set_value(float V, bool a_impeded=true) { if (wrapper) c.set_value(V, true); else Connection::set_value(V, true); }
	void impeded(bool a_impeded) { if (wrapper) c.impeded(true); else Connection::impeded(true); }
	void determinate(bool on) { if (wrapper) c.determinate(on); else Connection::determinate(on); }
};

//___________________________________________________________________________________
// A buffer takes a weak high impedence input and outputs a strong signal
class ABuffer: public Device {
  protected:
	Connection &m_in;
	Output m_out;

  public:
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		if (debug()) {
			std::cout << "Buffer " << this->name() << " received event " << name << std::endl;
		}
		m_out.set_value(D->rd());
	}

	ABuffer(Connection &in, const std::string &a_name=""): Device(a_name), m_in(in), m_out(Vss) {
		DeviceEvent<Connection>::subscribe<ABuffer>(this, &ABuffer::on_change, &m_in);
	}
	virtual ~ABuffer() {
		DeviceEvent<Connection>::unsubscribe<ABuffer>(this, &ABuffer::on_change, &m_in);
	}
	void wr(float in) { m_in.set_value(in, true); }
	Connection &rd() { return m_out; }
};

//___________________________________________________________________________________
// Inverts a high impedence input and outputs a signal
class Inverter: public ABuffer {

	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		m_out.set_value(!(D->signal()) * Vdd);
	}

  public:
	Inverter(Connection &in, const std::string &a_name=""): ABuffer(in, a_name) {
		m_out.set_value(!(in.signal()) * Vdd);
	}
};

//___________________________________________________________________________________
//  And gate, also nand for invert=true, or possibly doubles as buffer or inverter
class AndGate: public Device {
	std::vector<Connection *> m_in;
	Output m_out;
	bool m_inverted;

	void recalc() {
		if (!m_in.size()) return;
		bool sig = m_in[0]->signal();
		for (size_t n = 1; n < m_in.size(); ++n) {
			sig = sig & m_in[n]->signal();
		}
		m_out.set_value((m_inverted ^ sig) * Vdd);
	}
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		if (debug()) {
			std::cout << "AndGate " << this->name() << " received event " << name << std::endl;
		}
		recalc();
	}

  public:
	AndGate(const std::vector<Connection *> &in, bool inverted=false, const std::string &a_name=""):
		Device(a_name), m_in(in), m_out(Vdd), m_inverted(inverted) {
		for (size_t n = 0; n < m_in.size(); ++n)
			DeviceEvent<Connection>::subscribe<AndGate>(this, &AndGate::on_change, m_in[n]);
		recalc();
	}
	virtual ~AndGate() {
		for (size_t n = 0; n < m_in.size(); ++n)
			DeviceEvent<Connection>::unsubscribe<AndGate>(this, &AndGate::on_change, m_in[n]);
	}
	void inputs(const std::vector<Connection *> &in) { m_in = in; }
	Connection &rd() { return m_out; }
};

//___________________________________________________________________________________
//  Or gate, also nor for invert=true, or possibly doubles as buffer or inverter
class OrGate: public Device {
	std::vector<Connection *> m_in;
	Output m_out;
	bool m_inverted;

	void recalc() {
		if (!m_in.size()) return;
		if (debug()) std::cout << (m_inverted?"N":"") << "OR." << name() << "(";
		bool sig = m_in[0]->signal();
		if (debug()) std::cout << m_in[0]->name() << "[" << sig << "]";
		for (size_t n = 1; n < m_in.size(); ++n) {
			sig = sig | m_in[n]->signal();
			if (debug()) std::cout << ", " << m_in[n]->name()  << "[" << m_in[n]->signal() << "]";
		}
		if (debug()) std::cout << ") = " << (m_inverted ^ sig) << std::endl;
		m_out.set_value((m_inverted ^ sig) * Vdd);
	}

	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc();
	}

  public:
	OrGate(const std::vector<Connection *> &in, bool inverted=false, const std::string &a_name=""):
		Device(a_name), m_in(in), m_out(Vdd), m_inverted(inverted) {
		for (size_t n = 0; n < m_in.size(); ++n)
			DeviceEvent<Connection>::subscribe<OrGate>(this, &OrGate::on_change, m_in[n]);
		recalc();
	}
	virtual ~OrGate() {
		for (size_t n = 0; n < m_in.size(); ++n)
			DeviceEvent<Connection>::unsubscribe<OrGate>(this, &OrGate::on_change, m_in[n]);
	}
	bool inverted() { return m_inverted; }
	Connection &rd() { return m_out; }
};

//___________________________________________________________________________________
//  Or gate, also nor for invert=true, or possibly doubles as buffer or inverter
class XOrGate: public Device {
	std::vector<Connection *> m_in;
	Output m_out;
	bool m_inverted;

	void recalc() {
		if (!m_in.size()) return;

		bool sig = m_in[0]->signal();
		if (debug()) std::cout << sig;
		for (size_t n = 1; n < m_in.size(); ++n) {
			if (debug()) std::cout << "^" << m_in[n]->signal();
			sig = sig ^ m_in[n]->signal();
		}
		if (debug()) std::cout << " = " << sig << std::endl;
		m_out.set_value((m_inverted ^ sig) * Vdd);
	}

	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc();
	}

  public:
	XOrGate(const std::vector<Connection *> &in, bool inverted=false, const std::string &a_name=""):
		Device(a_name), m_in(in), m_out(Vdd), m_inverted(inverted) {

		for (size_t n = 0; n < m_in.size(); ++n)
			DeviceEvent<Connection>::subscribe<XOrGate>(this, &XOrGate::on_change, m_in[n]);
		recalc();
	}
	virtual ~XOrGate() {
		for (size_t n = 0; n < m_in.size(); ++n)
			DeviceEvent<Connection>::unsubscribe<XOrGate>(this, &XOrGate::on_change, m_in[n]);
	}
	bool inverted() { return m_inverted; }
	Connection &rd() { return m_out; }
};

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

class Wire: public Device {
	std::vector< Connection* > connections;
	bool indeterminate;
	DeviceEventQueue eq;
	float Voltage;
	float m_sum_conductance;
	float m_sum_v_over_R;


	float recalc() {
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

	bool assert_voltage() {  // fires signals to any impeded connections
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

	void queue_change(){  // Add a voltage change event to the queue
		if (assert_voltage()) {        // determine wire voltage and update impeded connections
			eq.queue_event(new DeviceEvent<Wire>(*this, "Wire Voltage Change"));
			eq.process_events();
		}
	}

	void on_connection_change(Connection *conn, const std::string &name, const std::vector<BYTE> &data) {
		if (debug()) {
			const char *direction = " <-- ";
			if (conn->impeded()) direction = " ->| ";
			std::cout << "Wire " << this->name() << direction << "Event " << conn->name() << " changed to " << conn->rd() << "V [";
			std::cout << (conn->impeded()?"o":"i") << "]" << std::endl;
		}
		queue_change();
	}

  public:
	Wire(const std::string &a_name=""): Device(a_name), indeterminate(true), Voltage(0), m_sum_conductance(0), m_sum_v_over_R(0) {}

	Wire(Connection &from, Connection &to, const std::string &a_name=""): Device(a_name), indeterminate(true), Voltage(0) {
		connect(from); connect(to);
	}

	virtual	~Wire() {
		eq.remove_events_for(this);
		for (auto conn=connections.begin(); conn != connections.end(); ++conn) {
			DeviceEvent<Connection>::unsubscribe<Wire>(this, &Wire::on_connection_change, *conn);
		}
	}

	void connect(Connection &connection, const std::string &a_name="") {
		connections.push_back(&connection);
		if (a_name.length()) connection.name(a_name);
		DeviceEvent<Connection>::subscribe<Wire>(this, &Wire::on_connection_change, &connection);
		queue_change();
	}
	void disconnect(const Connection &connection) {
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

	float rd(bool include_vdrop=false) {
		if (debug()) std::cout << name() << ": rd() = " << Voltage << std::endl;
		return Voltage;
	}

	bool determinate() { return !indeterminate; }
	bool signal() {
		float V = rd();
		return V > Vdd/2.0;
	}
};

//___________________________________________________________________________________
// A buffer whos output impedence depends on a third state signal
class Tristate: public Device {

	void pr_debug_info(const std::string &what) {
		std::cout << "  *** on " << what << ":" << this->name();
		std::cout << " gate=" << ((m_gate->signal()^m_invert_gate)?"high":"low") << ": output set to " << (m_out.impeded()?"high impedence":"");
		if (!m_out.impeded()) std::cout << m_out.rd();
		std::cout << std::endl;
	}

  protected:
	Connection *m_in;
	Connection *m_gate;
	Connection m_out;
	bool m_invert_gate;
	bool m_invert_output;


	void recalc_output() {
		bool impeded = !m_gate->signal();
		bool out = m_in->signal();
		if (m_invert_output) out = !out;
		if (m_invert_gate) impeded = !impeded;
		if (impeded) out = false;
		m_out.set_value(out * Vdd, impeded);
	}

	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc_output();
		if (debug()) pr_debug_info("input change");
	}

	virtual void on_gate_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc_output();
		if (debug()) pr_debug_info("gate change");
	}

  public:
	Tristate(Connection &in, Connection &gate, bool invert_gate=false, bool invert_output=false, const std::string &a_name="ts"):
		Device(a_name), m_in(&in), m_gate(&gate), m_out(name()+"::out"), m_invert_gate(invert_gate), m_invert_output(invert_output) {
		DeviceEvent<Connection>::subscribe<Tristate>(this, &Tristate::on_change, m_in);
		DeviceEvent<Connection>::subscribe<Tristate>(this, &Tristate::on_gate_change, m_gate);

		m_out.name(name()+".out");
		recalc_output();
	}
	virtual ~Tristate() {
		DeviceEvent<Connection>::unsubscribe<Tristate>(this, &Tristate::on_change, m_in);
		DeviceEvent<Connection>::unsubscribe<Tristate>(this, &Tristate::on_gate_change, m_gate);
	}
	void set_name(const std::string &a_name) {
		name(a_name);
		m_out.name(name()+".output");
	};
	bool signal() const { return m_out.signal(); }
	bool impeded() const { return m_out.impeded(); }
	bool inverted() const { return m_invert_output; }
	bool gate_invert() const { return m_invert_gate; }

	Tristate &inverted(bool v) { m_invert_output = v; recalc_output(); return *this; }
	Tristate &gate_invert(bool v) { m_invert_gate = v; recalc_output(); return *this; }

	void wr(float in) { m_in->set_value(in, true); }
	void input(Connection &in) {
		DeviceEvent<Connection>::unsubscribe<Tristate>(this, &Tristate::on_change, m_in);
		m_in = &in; recalc_output();
		DeviceEvent<Connection>::subscribe<Tristate>(this, &Tristate::on_change, m_in);
		if (debug()) pr_debug_info("input replaced");
	}
	void gate(Connection &gate) {
		DeviceEvent<Connection>::unsubscribe<Tristate>(this, &Tristate::on_gate_change, m_gate);
		m_gate = &gate;
		DeviceEvent<Connection>::subscribe<Tristate>(this, &Tristate::on_gate_change, m_gate);
		recalc_output();
		if (debug()) pr_debug_info("gate replaced");
	}

	Connection &input() { return *m_in; }
	Connection &gate() { return *m_gate; }
	Connection &rd() { return m_out; }
};

//___________________________________________________________________________________
// Clamps a voltabe between a lower and upper bound.
class Clamp: public Device {
	Connection &m_in;
	float m_lo;
	float m_hi;

	void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		float in = m_in.rd();
		bool changed = false;
		if (in < m_lo) {
			in = m_lo; changed = true;
		}
		if (in > m_hi) {
			in = m_hi; changed = true;
		}
		if (changed)
			m_in.set_value(in, m_in.impeded());
	}

  public:
	Clamp(Connection &in, float vLow=0.0, float vHigh=5.0):
		m_in(in), m_lo(vLow), m_hi(vHigh) {
		DeviceEvent<Connection>::subscribe<Clamp>(this, &Clamp::on_change, &m_in);
	};
	~Clamp() {
		DeviceEvent<Connection>::unsubscribe<Clamp>(this, &Clamp::on_change, &m_in);
	}
};


//___________________________________________________________________________________
// A relay, such as a reed relay.  A signal applied closes the relay.  Functionally,
// this is almost identical to a Tristate.
class Relay: public Device {
  protected:
	Connection &m_in;
	Connection &m_sw;
	Connection m_out;

	void recalc_output() {
		bool impeded = !m_sw.signal(); // open circuit if not signal
		float out = m_in.rd();
		m_out.set_value(out, impeded);
	}

	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc_output();
	}

	virtual void on_sw_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc_output();
	}

  public:
	Relay(Connection &in, Connection &sw, const std::string &a_name="sw"):
		Device(a_name), m_in(in), m_sw(sw), m_out(name()+"::out") {
		recalc_output();
		DeviceEvent<Connection>::subscribe<Relay>(this, &Relay::on_change, &m_in);
		DeviceEvent<Connection>::subscribe<Relay>(this, &Relay::on_sw_change, &m_sw);
	}
	virtual ~Relay() {
		DeviceEvent<Connection>::unsubscribe<Relay>(this, &Relay::on_change, &m_in);
		DeviceEvent<Connection>::unsubscribe<Relay>(this, &Relay::on_sw_change, &m_sw);
	}
	bool signal() { return m_out.signal(); }

	Connection &in() { return m_in; }
	Connection &sw() { return m_sw; }
	Connection &rd() { return m_out; }
};


//___________________________________________________________________________________
// A generalised D flip flop or a latch, depending on how we use it
class Latch: public Device {
	Connection &m_D;
	Connection &m_Ck;
	Output m_Q;
	Inverse m_Qc;
	bool m_positive;
	bool m_clocked;

	void on_clock_change(Connection *Ck, const std::string &name, const std::vector<BYTE> &data) {
		if (debug()) std::cout << this->name() << ": Ck is " << Ck->signal() << std::endl;
		if ((m_positive ^ (!Ck->signal()))) {
			m_Q.set_value(m_D.rd());
			if (debug()) std::cout << this->name() << ": Q was set to " << m_Q.rd() << std::endl;
		}
	}

	void on_data_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		if (m_positive ^ (!m_Ck.signal())) {
			if (debug()) std::cout << this->name() << ": D is " << D->signal() << std::endl;
			m_Q.set_value(D->rd());
		}
	}

  public:   // pulsed==true simulates a D flip flop, otherwise its a transparent latch
	Latch(Connection &D, Connection &Ck, bool positive=false, bool clocked=true):
		m_D(D), m_Ck(Ck), m_Q(Vss), m_Qc(m_Q), m_positive(positive), m_clocked(clocked) {
		DeviceEvent<Connection>::subscribe<Latch>(this, &Latch::on_clock_change, &m_Ck);
		if (!m_clocked)
			DeviceEvent<Connection>::subscribe<Latch>(this, &Latch::on_data_change, &m_D);
	}
	virtual ~Latch() {
		DeviceEvent<Connection>::unsubscribe<Latch>(this, &Latch::on_clock_change, &m_Ck);
		if (!m_clocked)
			DeviceEvent<Connection>::unsubscribe<Latch>(this, &Latch::on_data_change, &m_D);
	}

	bool clocked() { return m_clocked; }
	void set_name(const std::string &a_name) {
		name(a_name);
		m_Q.name(a_name+"::Q");
		m_Qc.name(a_name+"::Qc");
	}

	Connection &D() {return m_D; }
	Connection &Ck() {return m_Ck; }
	Connection &Q() {return m_Q; }
	Connection &Qc() {return m_Qc; }
};

//___________________________________________________________________________________
//  A multiplexer.  The "select" signals are bits which make up an index into the input.
//    multiplexers can route both digital and analog signals.
class Mux: public Device {
	std::vector<Connection *> m_in;
	std::vector<Connection *> m_select;
	Output m_out;
	BYTE m_idx;

	void calculate_select() {
		m_idx = 0;
		for (int n = m_select.size()-1; n >= 0; --n) {
			m_idx = m_idx << 1;
			m_idx |= m_select[n]->signal();
		}
		if (m_idx >= m_in.size()) throw(name() + std::string(": Multiplexer index beyond input bounds"));
	}

	void set_output() {
		float value = m_in[m_idx]->rd();
		if (debug()) std::cout << "MUX." << name() << " sel(" << (int)m_idx << ") = " << value << std::endl;
		m_out.set_value(value);
	}

	virtual void on_change(Connection *D, const std::string &name) {
		if (D == m_in[m_idx])     // only pays attention to the selected input
			set_output();
	}

	virtual void on_select(Connection *D, const std::string &name) {
		calculate_select();
		set_output();
	}

  public:
	Mux(const std::vector<Connection *> &in, const std::vector<Connection *> &select, const std::string &a_name="mux"):
		Device(a_name), m_in(in), m_select(select), m_out(Vdd),  m_idx(0) {
		if (m_select.size() > 8) throw(name() + ": Mux supports a maximum of 8 bits, or 256 inputs");
		for (size_t n = 0; n < m_in.size(); ++n) {
			DeviceEvent<Connection>::subscribe<Mux>(this, &Mux::on_change, m_in[n]);
		}
		for (size_t n = 0; n < m_select.size(); ++n) {
			DeviceEvent<Connection>::subscribe<Mux>(this, &Mux::on_select, m_select[n]);
		}
		calculate_select();
		set_output();
	}
	virtual ~Mux() {
		for (size_t n = 0; n < m_in.size(); ++n) {
			DeviceEvent<Connection>::unsubscribe<Mux>(this, &Mux::on_change, m_in[n]);
		}
		for (size_t n = 0; n < m_select.size(); ++n) {
			DeviceEvent<Connection>::unsubscribe<Mux>(this, &Mux::on_select, m_select[n]);
		}
	}
	Connection &in(int n) { return *m_in[n]; }
	Connection &select(int n) { return *m_select[n]; }
	Connection &rd() { return m_out; }
	size_t no_inputs() { return m_in.size(); }
	size_t no_selects() { return m_select.size(); }
};

class FET: public Device {
	Connection &m_in;
	Connection &m_gate;
	Connection m_out;
	bool       m_is_nType;

	void recalc() {
		float in = m_in.rd(), out = m_out.rd();
		if (!m_in.determinate()) in = out;
		if (!m_out.determinate()) out = in;

		if (debug()) {
			std::cout << (m_is_nType?"n":"p");
			std::cout << "FET: " << name() << "; in=" << in << "; out=" << out << " gate signal=" << m_gate.signal();
			std::cout << std::endl;
		}

		if (m_is_nType) {
			if (m_gate.signal()) {
				if (m_in.rd() != out) m_in.set_value(out, false);
				if (m_out.rd() != in) m_out.set_value(in, false);
			} else {
				m_in.set_value(in, true);
				m_out.set_value(in, true);
			}
		} else {
			if (m_gate.signal()) {
				m_in.set_value(in, true);
				m_out.set_value(in, true);
			} else {
				if (m_in.rd() != out) m_in.set_value(out, false);
				if (m_out.rd() != in) m_out.set_value(in, false);
			}
		}
	}
	virtual void on_change(Connection *D, const std::string &name) {
		recalc();
	}

public:
	FET(Connection &in, Connection &gate, bool is_nType = true, bool dbg=false):
	m_in(in), m_gate(gate), m_is_nType(is_nType) {
		debug(dbg);
		DeviceEvent<Connection>::subscribe<FET>(this, &FET::on_change, &m_in);
		DeviceEvent<Connection>::subscribe<FET>(this, &FET::on_change, &m_gate);
//		DeviceEvent<Connection>::subscribe<FET>(this, &FET::on_change, &m_out);
		recalc();
	}
	virtual ~FET() {
		DeviceEvent<Connection>::unsubscribe<FET>(this, &FET::on_change, &m_in);
		DeviceEvent<Connection>::unsubscribe<FET>(this, &FET::on_change, &m_gate);
//		DeviceEvent<Connection>::unsubscribe<FET>(this, &FET::on_change, &m_out);
	}
	const Connection &in() const { return m_in; }
	const Connection &gate() const { return m_gate; }
	Connection &rd() { return m_out; }
};

//___________________________________________________________________________________
// Prevents a jittering signal from toggling between high/low states.
class Schmitt: public Device {
	Connection &m_in;
	Connection &m_enable;
	Connection m_enabled;
	Connection m_out;
	bool       m_gate_invert;
	bool       m_out_invert;

	const float m_lo = Vdd / 10.0 * 4;
	const float m_hi = Vdd / 10.0 * 6;

	void recalc() {
		//  inv   sig   en    ( xor function )
		//  0     0     0
		//  0     1     1
		//  1     0     1
		//  1     1     0
		bool enabled = m_gate_invert ^ m_enable.signal();

		float Vout = m_out.rd();
		if (!enabled)     // high impedence
			m_out.set_value(Vss, true);
		else {
			double Vin = m_in.rd();
			if ((Vin > m_hi && Vout < m_hi) || (Vin < m_lo && Vout > m_lo)) {
				Vout = m_out_invert?(!m_in.signal())*Vdd:m_in.signal()*Vdd;
				m_out.set_value(Vout, false);
			} else if ((Vin > m_hi && Vout > m_hi) || (Vin < m_lo && Vout < m_lo)) {
				Vout = m_out_invert?(!m_in.signal())*Vdd:m_in.signal()*Vdd;
				m_out.set_value(Vout, false);
			}
		}
	}

	void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc();
	}

  public:
	Schmitt(Connection &in, Connection &en, bool impeded=false, bool gate_invert=true, bool out_invert=false):
		m_in(in), m_enable(en), m_out(Vss, impeded), m_gate_invert(gate_invert), m_out_invert(out_invert) {
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, &m_in);
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, &m_enable);
		recalc();
	}
	Schmitt(Connection &in, bool impeded=false, bool out_invert=false):
		m_in(in), m_enable(m_enabled), m_out(Vss, impeded), m_gate_invert(false), m_out_invert(out_invert) {
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, &m_in);
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, &m_enable);
		m_enable.set_value(Vdd, true);     // No enable signal, so always true
		recalc();
	}
	virtual ~Schmitt() {
		DeviceEvent<Connection>::unsubscribe<Schmitt>(this, &Schmitt::on_change, &m_in);
		DeviceEvent<Connection>::unsubscribe<Schmitt>(this, &Schmitt::on_change, &m_enable);
	}

	void gate_invert(bool invert) { m_gate_invert = invert; recalc(); }
	void out_invert(bool invert) { m_out_invert = invert; recalc(); }

	Connection &in() { return m_in; }
	Connection &en() { return m_enable; }
	Connection &rd() { return m_out; }
};


