#pragma once

#include <iostream>
#include <queue>
#include <tuple>
#include <map>
#include <string>
#include <cstring>
#include <functional>
#include "../utils/smart_ptr.h"
#include "constants.h"


//__________________________________________________________________________________________________
// Device definitions
class Device {
	std::string m_name;
  public:
	const float Vss = 0.0;
	const float Vdd = 5.0;

	Device(): m_name("") {}
	Device(const std::string name): m_name(name) {}

	virtual ~Device() {};
	const std::string &name() { return m_name; }
};


//___________________________________________________________________________________
// A base class for device events that can be queued.
class QueueableEvent {
  public:
	virtual ~QueueableEvent() {}
	virtual void fire_event() = 0;
};


//___________________________________________________________________________________
// We can have a single event queue for all devices, and process events
// in sequence.  Events themselves are derived from a base class.
class DeviceEventQueue {
	static std::queue< SmartPtr<QueueableEvent> >events;

  public:
	void queue_event(QueueableEvent *event) {
		events.push(event);
	}

	void process_events() {
		int n;
		for (n=0; n<100; ++n)
			if (events.empty())
				break;
			else {
				auto event = events.front(); events.pop();
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

	typedef std::tuple<void *, const T *> KeyType;            // Object, Source


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
			if (!instance || instance == m_device)
				s->second(dynamic_cast<T *>(m_device), m_eventname, m_data);
		}
	}

	template<class Q> static void subscribe (Q *ob,  void (Q::*callback)(T *device, const std::string &event), const T *instance = NULL) {
		using namespace std::placeholders;
		subscribers[KeyType{(void *)ob, instance}] = std::bind(callback, ob, _1, _2);
	}


	template<class Q> static void subscribe (Q *ob,  void (Q::*callback)(T *device, const std::string &event, const std::vector<BYTE> &data), const T *instance = NULL) {
		using namespace std::placeholders;
		subscribers[KeyType{(void *)ob, instance}] = std::bind(callback, ob, _1, _2, _3);
	}

	static void unsubscribe(void *ob, T *instance = NULL) {
		auto key = KeyType{ob, instance};
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
	DeviceEventQueue q;

	void queue_change(){  // Add a voltage change event to the queue
		q.queue_event(new DeviceEvent<Connection>(*this, "Voltage Change"));
	}
  public:
	Connection(): m_V(Vss), m_impeded(true) {};
	Connection(float V, bool impeded): m_V(V), m_impeded(impeded) {
		queue_change();
	};
	Connection(bool on, bool impeded): m_V(on?Vdd:Vss), m_impeded(impeded) {
		queue_change();
	};

	float rd() { return m_impeded?Vss:m_V; }
	bool impeded() { return m_impeded; }
	bool signal() { return m_V > Vdd/2.0; }


	void set_value(float V, bool impeded) {
		m_V = V, m_impeded = impeded;
		queue_change();
	}
};

//___________________________________________________________________________________
// A wire can be defined as a collection of connections
class Wire: public Device {
	std::vector<Connection> connections;
	bool indeterminate;
  public:
	Wire(): indeterminate(true) {}
	void connect(const Connection &connection) {
		connections.push_back(connection);
	}
	float rd() {
		float V = 50.0;
		indeterminate = true;
		for (auto conn = connections.begin(); conn != connections.end(); ++conn) {
			if (!conn->impeded()) {   // Find lowest unimpeded connection
				float v = conn->rd();
				V=(V>v)?v:V;
				indeterminate = false;
			}
		}
		if (indeterminate) {
			V = Vss;     // If no unimpeded sources or drains, then indeterminate
		}
		return V;
	}
	void assert() {  // fires signals to any impeded connections
		float V = rd();
		if (!indeterminate) {
			for (auto conn = connections.begin(); conn != connections.end(); ++conn) {
				if (conn->impeded()) {
					conn->set_value(V, true);
				}
			}
		}
	}
	float signal() {
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
		if (m_positive) {
			if (Ck->signal()) {       // leading edge only
				m_Q.set_value(m_D.rd(), false);
				m_Qc.set_value((float)(m_D.signal()?Vss:Vdd), false);
			}
		} else {
			if (! Ck->signal()) {    // falling edge only
				m_Q.set_value(m_D.rd(), false);
				m_Qc.set_value((float)(m_D.signal()?Vss:Vdd), false);
			}
		}
	}

	void on_data_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		if (m_positive ) {    // positive enable
			if (m_Ck.signal()) {
				m_Q.set_value(D->rd(), false);
				m_Qc.set_value(m_Q.signal() * Vdd, false);
			}
		} else {             // negative enable
			if (!m_Ck.signal()) {
				m_Q.set_value(D->rd(), false);
				m_Qc.set_value(m_Q.signal() * Vdd, false);
			}
		}
	}

  public:
	Latch(Connection &D, Connection &Ck, bool positive=false, bool pulsed=true):
		m_D(D), m_Ck(Ck), m_Q(Vss, false), m_Qc(Vdd, false), m_positive(positive), m_pulsed(pulsed) {
		DeviceEvent<Connection>::subscribe<Latch>(this, &Latch::on_clock_change, &m_Ck);
		if (!m_pulsed)
			DeviceEvent<Connection>::subscribe<Latch>(this, &Latch::on_data_change, &m_D);
	}

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
class Tristate: public ABuffer {
	Connection &m_gate;
	bool m_invert_gate;
	bool m_invert_output;

	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		bool impeded = m_gate.signal();
		bool out = D->signal();
		if (m_invert_output) out = !out;
		if (m_invert_gate) impeded = !impeded;
		m_out.set_value(out * Vdd, impeded);
	}

  public:
	Tristate(Connection &in, Connection &gate, bool invert_gate=false, bool invert_output=false):
		ABuffer(in), m_gate(gate), m_invert_gate(invert_gate), m_invert_output(invert_output) {
		DeviceEvent<Connection>::subscribe<ABuffer>(this, &ABuffer::on_change, &m_gate);
	}
};


//___________________________________________________________________________________
// Inverts a high impedence input and outputs a signal
class Inverter: public ABuffer {

	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		m_out.set_value(!(D->signal()) * Vdd, false);
	}

  public:
	Inverter(Connection in): ABuffer(in){}
};

//___________________________________________________________________________________
// Prevents a jittering signal from toggling between high/low states.
class Schmitt: public Device {
	Connection &m_in;
	Connection m_out;
	bool  m_impeded;
	const float m_lo = (float)1.5;
	const float m_hi = (float)3.5;


	void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data) {
		float in = m_in.rd();
		float out = m_out.rd();
		if (m_impeded)
			m_out.set_value(in, true);   // High impedence output
		else {
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
	Schmitt(Connection &in, bool impeded=false):
		m_in(in), m_out(Vss, false), m_impeded(impeded) {
		DeviceEvent<Connection>::subscribe<Schmitt>(this, &Schmitt::on_change, &m_in);
	}
	void set_impeded(bool impeded) {
		m_impeded = impeded;
		m_out.set_value(m_out.rd(), false);
	}
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

