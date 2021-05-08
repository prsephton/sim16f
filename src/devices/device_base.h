#pragma once

#include <iostream>
#include <queue>
#include <tuple>
#include <map>
#include <string>
#include <cstring>
#include <functional>
#include <mutex>
#include "../utils/smart_ptr.h"
#include "constants.h"

//__________________________________________________________________________________________________
// Device definitions
class Device {
	std::string m_name;
  public:
	static constexpr float Vss = 0.0;
	static constexpr float Vdd = 5.0;

	Device(): m_name("") {}
	Device(const Device &d) {
		name(d.name());
	}
	Device(const std::string name): m_name(name) {}

	Device &operator=(const Device &d) {
		name(d.name());
		return *this;
	}

	virtual ~Device() {};
	const std::string &name() const { return m_name; }
	void name(const std::string &a_name) { m_name = a_name; }
};


//___________________________________________________________________________________
// A base class for device events that can be queued.
class QueueableEvent {
  public:
	virtual ~QueueableEvent() {}
	virtual void fire_event() = 0;
	virtual bool compare(Device *d) = 0;
};

//___________________________________________________________________________________
// We can have a single event queue for all devices, and process events
// in sequence.  Events themselves are derived from a base class.
class DeviceEventQueue {
	static std::queue< SmartPtr<QueueableEvent> >events;
	static std::mutex mtx;

  public:
	void queue_event(QueueableEvent *event) {
		mtx.lock();
		events.push(event);
		mtx.unlock();
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
		for (n=0; n<100; ++n)
			if (events.empty())
				break;
			else {
				mtx.lock();
				auto event = events.front(); events.pop();
				mtx.unlock();
				event->fire_event();
			}
		if (n == 100)
			std::cout << "Possible event loop detected" << std::endl;
	}
};

//___________________________________________________________________________________
// A pub-sub for device events.  For example, when the clock triggers, we can put
// one of these events in a queue.
// There is only one list of subscribers per device.  The CPU implementation will
// traverse all known devices, and fire any outstanding events.
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

	virtual void fire_event() {
		for(each_subscriber s = subscribers.begin(); s!= subscribers.end(); ++s) {
			const T *instance = std::get<1>(s->first);
			if (!instance || instance == m_device) {
				try {
					T * device = dynamic_cast<T *>(m_device);
					if (device == NULL) throw(std::string("Null device"));
					s->second(device, m_eventname, m_data);
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
		subscribers[KeyType{ob, instance, &callback}] = std::bind(callback, ob, _1, _2);
	}


	template<class Q> static void subscribe (Q *ob,  void (Q::*callback)(T *device, const std::string &event, const std::vector<BYTE> &data), const T *instance = NULL) {
		using namespace std::placeholders;
		subscribers[KeyType{ob, instance, &callback}] = std::bind(callback, ob, _1, _2, _3);
	}

	template<class Q> static void unsubscribe(void *ob, void (Q::*callback)(T *device, const std::string &event), T *instance = NULL) {
		auto key = KeyType{ob, instance, &callback};
		if (subscribers.find(key) != subscribers.end())
			subscribers.erase(key);
	}

	template<class Q> static void unsubscribe(void *ob, void (Q::*callback)(T *device, const std::string &event, const std::vector<BYTE> &data), T *instance = NULL) {
		auto key = KeyType{ob, instance, &callback};
		if (subscribers.find(key) != subscribers.end())
			subscribers.erase(key);
	}
};


//___________________________________________________________________________________
// The hardware architecture uses several sequential logic components.  It would be
// really nice to be able to emulate the way the hardware works by stringing together
// elements and reacting to events that cause status to change.
//
// Here's a basic connection.  Changes to connection values always spawn device events.
//
class Connection: public Device {
	float m_V;
	bool  m_impeded;
	bool  m_determinate;
	DeviceEventQueue q;

	void queue_change(){  // Add a voltage change event to the queue
		q.queue_event(new DeviceEvent<Connection>(*this, "Voltage Change"));
	}
  public:
	Connection(const std::string &a_name=""):
		Device(a_name), m_V(Vss), m_impeded(true), m_determinate(false) {};
	Connection(float V, bool impeded, const std::string &a_name=""):
		Device(a_name), m_V(V), m_impeded(impeded), m_determinate(false) {
	};
	Connection(bool on, bool impeded, const std::string &a_name=""):
		Device(a_name), m_V(on?Vdd:Vss), m_impeded(impeded), m_determinate(false) {
	};

	virtual	~Connection() {
		DeviceEventQueue q;
		q.remove_events_for(this);
	}

	float rd() { return m_V; }
	bool impeded() { return m_impeded; }
	bool determinate() { return m_determinate || !m_impeded; }
	void determinate(bool on) { m_determinate = on; }
	bool signal() { return m_V > Vdd/2.0; }
	void set_value(float V, bool impeded) {
		if (m_V != V || m_impeded != impeded) {
			m_V = V, m_impeded = impeded;
			queue_change();
		}
	}
};

//___________________________________________________________________________________
// A wire can be defined as a collection of connections
class Wire: public Device {
	std::vector< Connection* > connections;
	bool indeterminate;
	DeviceEventQueue q;
	bool debug = false;


	void queue_change(){  // Add a voltage change event to the queue
		q.queue_event(new DeviceEvent<Wire>(*this, "Wire Voltage Change"));
	}

	void on_connection_change(Connection *conn, const std::string &name, const std::vector<BYTE> &data) {
		const char *direction = " <-- ";
		if (conn->impeded()) direction = " ->| ";
		if (debug) std::cout << this->name() << direction << name << " changed to " << conn->rd() << "V" << std::endl;
		assert();         // determine wire voltage and update impeded connections
		queue_change();
	}

  public:
	Wire(const std::string &a_name=""): Device(a_name), indeterminate(true) {}

	Wire(Connection &from, Connection &to, const std::string &a_name=""): Device(a_name), indeterminate(true) {
		connect(from); connect(to);
	}
	void connect(Connection &connection) {
		connections.push_back(&connection);
		DeviceEvent<Connection>::subscribe<Wire>(this, &Wire::on_connection_change, &connection);
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
	}
	float rd() {
		float V;
		indeterminate = true;
		if (name() == "RA2::pin") debug = true;

		if (debug) std::cout << "read wire " << name() << ": ";
		for (auto conn = connections.begin(); conn != connections.end(); ++conn) {
			if (debug) std::cout << (*conn)->name()<<" ";
			if ((*conn)->impeded()) {   // Find lowest unimpeded connection
				(*conn)->determinate(false);
				if (debug) std::cout << "[i]";
			} else {
				float v = (*conn)->rd();
				if (indeterminate) {
					V = v;
					indeterminate = false;
				} else {
					V = (V>v)?v:V;
				}
				if (debug) std::cout << "[" << v << "]";
			}
		}
		if (indeterminate) {
			V = Vss;     // If no unimpeded sources or drains, then indeterminate
		} else {
			for (auto conn = connections.begin(); conn != connections.end(); ++conn) {
				(*conn)->determinate(true);
			}
		}
		if (debug) std::cout << "=" << V << "V" << std::endl;
		debug = false;
		return V;
	}
	void assert() {  // fires signals to any impeded connections
		float V = rd();
		if (debug) std::cout << "Wire: " << name() << " is " << (indeterminate?"unknown":"");
		if (debug) std::cout << V << "V";
		for (auto conn = connections.begin(); conn != connections.end(); ++conn) {
			if ((*conn)->impeded()) {
				(*conn)->set_value(V, true);
			}
		}
		if (debug) std::cout << std::endl;
	}
	bool determinate() { return !indeterminate; }
	bool signal() {
		float V = rd();
		return V > Vdd/2.0;
	}
};

//___________________________________________________________________________________
// A generalised D flip flop or a latch, depending on how we use it
class Latch: public Device {
	Connection &m_D;
	Connection &m_Ck;
	Connection m_Q;
	Connection m_Qc;
	bool m_positive;
	bool m_pulsed;

	void on_clock_change(Connection *Ck, const std::string &name, const std::vector<BYTE> &data) {
		std::cout << this->name() << ": Ck is " << Ck->signal() << std::endl;
		if (m_positive) {
			if (Ck->signal()) {       // leading edge only
				m_Q.set_value(m_D.rd(), false);
				m_Qc.set_value(!m_Q.signal() * Vdd, false);
			}
		} else {
			if (! Ck->signal()) {    // falling edge only
				m_Q.set_value(m_D.rd(), false);
				m_Qc.set_value(!m_Q.signal() * Vdd, false);
			}
		}
	}

	void on_data_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		if (!m_pulsed && (m_positive ^ (!m_Ck.signal()))) {
			std::cout << this->name() << ": D is " << D->signal() << std::endl;
			m_Q.set_value(D->rd(), false);
			m_Qc.set_value(!m_Q.signal() * Vdd, false);
		}
	}

  public:   // pulsed==true simulates a D flip flop, otherwise its a transparent latch
	Latch(Connection &D, Connection &Ck, bool positive=false, bool pulsed=false):
		m_D(D), m_Ck(Ck), m_Q(Vss, false), m_Qc(Vdd, false), m_positive(positive), m_pulsed(pulsed) {
		if (m_pulsed)
			DeviceEvent<Connection>::subscribe<Latch>(this, &Latch::on_clock_change, &m_Ck);
		else
			DeviceEvent<Connection>::subscribe<Latch>(this, &Latch::on_data_change, &m_D);
	}

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
// A buffer takes a weak high impedence input and outputs a strong signal
class ABuffer: public Device {
  protected:
	Connection &m_in;
	Connection m_out;

  public:
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		m_out.set_value(D->signal() * Vdd, false);
	}

	ABuffer(Connection &in): m_in(in), m_out(Vss, false) {
		DeviceEvent<Connection>::subscribe<ABuffer>(this, &ABuffer::on_change, &m_in);
	}
	virtual ~ABuffer() {}
	void wr(float in) { m_in.set_value(in, true); }
	Connection &rd() { return m_out; }
};

//___________________________________________________________________________________
// A buffer whos output impedence depends on a third state signal
class Tristate: public Device {
  protected:
	Connection &m_in;
	Connection &m_gate;
	Connection m_out;
	bool m_invert_gate;
	bool m_invert_output;


	void recalc_output() {
		bool impeded = !m_gate.signal();
		bool out = m_in.signal();
		if (m_invert_output) out = !out;
		if (m_invert_gate) impeded = !impeded;
		m_out.set_value(out * Vdd, impeded);
	}

	void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc_output();
	}

	void on_gate_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc_output();
	}

  public:
	Tristate(Connection &in, Connection &gate, bool invert_gate=false, bool invert_output=false, const std::string &a_name="ts"):
		Device(a_name), m_in(in), m_gate(gate), m_out(name()+"::out"), m_invert_gate(invert_gate), m_invert_output(invert_output) {
		DeviceEvent<Connection>::subscribe<Tristate>(this, &Tristate::on_change, &m_in);
		DeviceEvent<Connection>::subscribe<Tristate>(this, &Tristate::on_gate_change, &m_gate);
		recalc_output();
	}
	bool signal() { return m_out.signal(); }
	bool impeded() { return m_out.impeded(); }
	bool inverted() { return m_invert_output; }
	bool gate_invert() { return m_invert_gate; }

	void wr(float in) { m_in.set_value(in, true); }
	Connection &rd() { return m_out; }
};


//___________________________________________________________________________________
// A relay, such as a reed relay.  A signal applied closes the relay
class Relay: public Device {
  protected:
	Connection &m_in;
	Connection &m_sw;
	Connection m_out;

	void recalc_output() {
		bool impeded = !m_sw.signal(); // open circuit if not signal
		bool out = m_in.signal();
		m_out.set_value(out * Vdd, impeded);
	}

	void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc_output();
	}

	void on_sw_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		recalc_output();
	}

  public:
	Relay(Connection &in, Connection &sw, const std::string &a_name="sw"):
		Device(a_name), m_in(in), m_sw(sw), m_out(name()+"::out") {
		DeviceEvent<Connection>::subscribe<Relay>(this, &Relay::on_change, &m_in);
		DeviceEvent<Connection>::subscribe<Relay>(this, &Relay::on_sw_change, &m_sw);
		recalc_output();
	}
	bool signal() { return m_out.signal(); }

	Connection &in() { return m_in; }
	Connection &sw() { return m_sw; }
	Connection &rd() { return m_out; }
};


//___________________________________________________________________________________
// Inverts a high impedence input and outputs a signal
class Inverter: public ABuffer {

	virtual void on_change(Connection *D, const std::string &name) {
		m_out.set_value(!(D->signal()) * Vdd, false);
	}

  public:
	Inverter(Connection &in): ABuffer(in){
		m_out.set_value(!(in.signal()) * Vdd, false);
	}
};

//___________________________________________________________________________________
//  And gate, also nand for invert=true, or possibly doubles as buffer or inverter
class AndGate: public Device {
	std::vector<Connection *> m_in;
	Connection m_out;
	bool m_inverted;

	virtual void on_change(Connection *D, const std::string &name) {
		bool sig = true;
		for (size_t n = 0; n < m_in.size(); ++n) {
			sig = sig && m_in[n]->signal();
		}
		m_out.set_value((m_inverted ^ sig) * Vdd, false);
	}

  public:
	AndGate(const std::vector<Connection *> &in, bool inverted=false, const std::string &a_name=""):
		Device(a_name), m_in(in), m_out(Vdd, false), m_inverted(inverted) {
		bool sig = true;
		for (size_t n = 0; n < m_in.size(); ++n) {
			sig = sig && m_in[n]->signal();
			DeviceEvent<Connection>::subscribe<AndGate>(this, &AndGate::on_change, m_in[n]);
		}
		m_out.set_value((m_inverted ^ sig) * Vdd, false);
	}
	Connection &rd() { return m_out; }
};

//___________________________________________________________________________________
//  Or gate, also nor for invert=true, or possibly doubles as buffer or inverter
class OrGate: public Device {
	std::vector<Connection *> m_in;
	Connection m_out;
	bool m_inverted;


	virtual void on_change(Connection *D, const std::string &name) {
		bool sig = false;
		for (size_t n = 0; n < m_in.size(); ++n) {
			sig = sig || m_in[n]->signal();
		}
		m_out.set_value((m_inverted ^ sig) * Vdd, false);
	}

  public:
	OrGate(const std::vector<Connection *> &in, bool inverted=false, const std::string &a_name=""):
		Device(a_name), m_in(in), m_out(Vdd, false), m_inverted(inverted) {
		bool sig = false;
		for (size_t n = 0; n < m_in.size(); ++n) {
			sig = sig || m_in[n]->signal();
			DeviceEvent<Connection>::subscribe<OrGate>(this, &OrGate::on_change, m_in[n]);
		}
		m_out.set_value((m_inverted ^ sig) * Vdd, false);
	}
	Connection &rd() { return m_out; }
};

//___________________________________________________________________________________
//  A multiplexer.  The "select" signals are bits which make up an index into the input.
//    multiplexers can route both digital and analog signals.
class Mux: public Device {
	std::vector<Connection *> m_in;
	std::vector<Connection *> m_select;
	Connection m_out;
	BYTE m_idx;

	void calculate_select() {
		m_idx = 0;
		for (int n = m_select.size()-1; n >= 0; --n) {
			m_idx = m_idx << 1;
			m_idx |= m_select[n]->signal();
		}
		if (m_idx >= m_in.size()) throw(name() + std::string(": Multilexer index beyond input bounds"));
	}

	void set_output() {
		m_out.set_value(m_in[m_idx]->rd(), false);
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
	Mux(const std::vector<Connection *> &in, const std::vector<Connection *> &select, const std::string &a_name=""):
		Device(a_name), m_in(in), m_select(select), m_out(Vdd, false),  m_idx(0) {
		if (m_select.size() > 8) throw(name() + ": Mux supports a maximum of 8 bits, or 256 inputs");
		for (size_t n = 0; n < m_in.size(); ++n) {
			DeviceEvent<Connection>::subscribe<Mux>(this, &Mux::on_change, m_in[n]);
		}
		for (size_t n = 0; n < m_select.size(); ++n) {
			DeviceEvent<Connection>::subscribe<Mux>(this, &Mux::on_select, m_in[n]);
		}
		calculate_select();
		set_output();
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

	virtual void on_input_change(Connection *D, const std::string &name) {
		bool gate = m_gate.signal() ^ (!m_is_nType);
		m_out.set_value(gate * m_in.rd(), gate);
	}

	virtual void on_output_change(Connection *D, const std::string &name) {
		bool gate = m_gate.signal() ^ (!m_is_nType);
		m_in.set_value(gate * m_out.rd(), gate);
	}

public:
	FET(Connection &in, Connection &gate, bool is_nType = true):
	m_in(in), m_gate(gate), m_is_nType(is_nType) {
		DeviceEvent<Connection>::subscribe<FET>(this, &FET::on_input_change, &m_in);
		DeviceEvent<Connection>::subscribe<FET>(this, &FET::on_input_change, &m_gate);
		DeviceEvent<Connection>::subscribe<FET>(this, &FET::on_output_change, &m_out);
		DeviceEvent<Connection>::subscribe<FET>(this, &FET::on_output_change, &m_gate);
	}
	Connection &rd() { return m_out; }
};

//___________________________________________________________________________________
// Prevents a jittering signal from toggling between high/low states.
class Schmitt: public Device {
	Connection &m_in;
	Connection &m_enable;
	Connection m_out;
	bool       m_gate_invert;

	const float m_lo = (float)1.5;
	const float m_hi = (float)3.5;


	void on_enable(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		//  gi     s   o
		//  0      0   0
		//  0      1   1
		//  1      0   1
		//  1      1   0
		if (m_gate_invert ^ D->signal()) {
			m_out.set_value(m_out.rd(), false);
		} else {
			m_out.set_value(m_out.rd(), true);
		}
	}

	void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		float in = m_in.rd();
		float out = m_out.rd();
		if (m_gate_invert ^ m_enable.signal()) {
			if (out < m_lo) {
				if (in < m_hi)
					out = Vss;
				else
					out = Vdd;
			} else if (out > m_hi) {
				if (in > m_lo)
					out = Vdd;
				else
					out = Vss;
			}
			m_out.set_value(out, false);
		}
	}

  public:
	Schmitt(Connection &in, Connection &en, bool impeded=false, bool gate_invert=true):
		m_in(in), m_enable(en), m_out(Vss, impeded), m_gate_invert(gate_invert) {
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, &m_in);
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_enable, &m_enable);
		m_enable.set_value(Vss, true); // start enabled if gate_invert
	}
	Connection &in() { return m_in; }
	Connection &en() { return m_enable; }
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
		if (in < m_lo) in = m_lo;
		if (in > m_hi) in = m_hi;
		m_in.set_value(in, false);
	}

  public:
	Clamp(Connection &in, float vLow=0.0, float vHigh=5.0):
		m_in(in), m_lo(vLow), m_hi(vHigh) {
		DeviceEvent<Connection>::subscribe<Clamp>(this, &Clamp::on_change, &m_in);
	};
};

