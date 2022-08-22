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
	void queue_change();
	virtual float calc_voltage_drop(float out_voltage);
	bool impeded_suppress_change(bool a_impeded);

  public:
	Connection(const std::string &a_name="");
	Connection(float V, bool impeded=true, const std::string &a_name="");
	virtual	~Connection();

	virtual float rd(bool include_vdrop=false) const;

	float vDrop() const;
	virtual bool signal() const;
	virtual bool impeded() const;
	virtual bool determinate() const;
	virtual float set_vdrop(float out_voltage);
	void impeded(bool a_impeded);

	void determinate(bool on);
	void conductance(float iR);
	virtual void R(float a_R);
	virtual float conductance() const;
	virtual float R() const;
	virtual float I() const;
	virtual void set_value(float V, bool a_impeded);
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

	void recalc();
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);
	virtual float calc_voltage_drop(float out_voltage);

  public:
	Terminal(const std::string name="Pin");
	Terminal(float V, const std::string name="Pin");
	virtual ~Terminal();
	void connect(Connection &c);
	void disconnect(Connection &c);
	virtual void set_value(float V, bool a_impeded);
	void impeded(bool a_impeded);
	virtual bool impeded() const;
	virtual float rd(bool include_vdrop=true) const;
};


//___________________________________________________________________________________
// Voltage source.   Always constant, no matter what.  Overrides connection V.
class Voltage: public Terminal {
	float m_voltage;
public:
	Voltage(float V, const std::string name="Vin");

	virtual bool impeded() const;
	virtual float rd(bool include_vdrop=true) const;
};

//___________________________________________________________________________________
// A Weak Voltage source.  If there are any unimpeded connections, we calculate the
// lowest unimpeded, otherwise the highest impeded connection as the voltage
class PullUp: public Connection {
public:
	PullUp(float V, const std::string name="Vin");
	virtual bool impeded() const;
};

//___________________________________________________________________________________
// Ground.  Always zero.
class Ground: public Voltage {
public:
	Ground();
	virtual bool impeded() const;
	virtual float rd(bool include_vdrop=true) const;
};


//___________________________________________________________________________________
// An inverted connection.  If the connection was high, we return low, and vica versa.
class Inverse: public Connection {
	Connection &c;

	virtual void on_connection_change(Connection *conn, const std::string &name, const std::vector<BYTE> &data);

  public:

	Inverse(Connection &a_c);
	virtual  ~Inverse();

	virtual void set_value(float V, bool a_impeded);
	void impeded(bool a_impeded);
	void determinate(bool on);
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

	virtual void on_connection_change(Connection *c, const std::string &name, const std::vector<BYTE> &data);

  public:
	Output();
	Output(Connection &a_c);
	Output(float V, const std::string &a_name="");
	virtual ~Output();
	virtual bool signal() const;
	virtual float rd(bool include_vdrop=true) const;
	virtual bool impeded() const;
	virtual bool determinate() const;

	virtual void set_value(float V, bool a_impeded=true);
	void impeded(bool a_impeded);
	void determinate(bool on);
};


//___________________________________________________________________________________
// An output connection is unimpeded- basically, this means that multiple inputs
// will allow current to flow between them.  Using Output() and Input() for the
// same Connection(), we can control exactly how we want to define components.  For
// example, an AND gate will have one or more Inputs with an unimpeded Output.

class Input: public Connection {
	Connection &c;
	bool wrapper;

	virtual void on_connection_change(Connection *c, const std::string &name, const std::vector<BYTE> &data);

  public:
	Input();
	Input(Connection &a_c);
	Input(float V, const std::string &a_name="");
	virtual ~Input();

	virtual bool signal() const;
	virtual float rd(bool include_vdrop=true) const;
	virtual bool impeded() const;
	virtual bool determinate() const;

	virtual void set_value(float V, bool a_impeded=true);
	void impeded(bool a_impeded);
	void determinate(bool on);
};

//___________________________________________________________________________________
// A buffer takes a weak high impedence input and outputs a strong signal
class ABuffer: public Device {
  protected:
	Connection &m_in;
	Output m_out;

  public:
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

	ABuffer(Connection &in, const std::string &a_name="");
	virtual ~ABuffer();
	void wr(float in);
	Connection &rd();
};

//___________________________________________________________________________________
// Inverts a high impedence input and outputs a signal
class Inverter: public ABuffer {

	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	Inverter(Connection &in, const std::string &a_name="");
};

//___________________________________________________________________________________
//  And gate, also nand for invert=true, or possibly doubles as buffer or inverter
class AndGate: public Device {
	std::vector<Connection *> m_in;
	Output m_out;
	bool m_inverted;

	void recalc();
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	AndGate(const std::vector<Connection *> &in, bool inverted=false, const std::string &a_name="");
	virtual ~AndGate();
	void inputs(const std::vector<Connection *> &in);
	Connection &rd();
};

//___________________________________________________________________________________
//  Or gate, also nor for invert=true, or possibly doubles as buffer or inverter
class OrGate: public Device {
	std::vector<Connection *> m_in;
	Output m_out;
	bool m_inverted;

	void recalc();

	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	OrGate(const std::vector<Connection *> &in, bool inverted=false, const std::string &a_name="");
	virtual ~OrGate();
	bool inverted();
	Connection &rd();
};

//___________________________________________________________________________________
//  Or gate, also nor for invert=true, or possibly doubles as buffer or inverter
class XOrGate: public Device {
	std::vector<Connection *> m_in;
	Output m_out;
	bool m_inverted;

	void recalc();
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	XOrGate(const std::vector<Connection *> &in, bool inverted=false, const std::string &a_name="");
	virtual ~XOrGate();
	bool inverted();
	Connection &rd();
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


	float recalc();
	bool assert_voltage();
	void queue_change();
	void on_connection_change(Connection *conn, const std::string &name, const std::vector<BYTE> &data);

  public:
	Wire(const std::string &a_name="");
	Wire(Connection &from, Connection &to, const std::string &a_name="");
	virtual	~Wire();

	void connect(Connection &connection, const std::string &a_name="");
	void disconnect(const Connection &connection);

	float rd(bool include_vdrop=false);
	bool determinate();
	bool signal();
};

//___________________________________________________________________________________
// A buffer whos output impedence depends on a third state signal
class Tristate: public Device {

	void pr_debug_info(const std::string &what);

  protected:
	Connection *m_in;
	Connection *m_gate;
	Connection m_out;
	bool m_invert_gate;
	bool m_invert_output;


	void recalc_output();
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);
	virtual void on_gate_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	Tristate(Connection &in, Connection &gate, bool invert_gate=false, bool invert_output=false, const std::string &a_name="ts");
	virtual ~Tristate();
	void set_name(const std::string &a_name);
	bool signal() const;
	bool impeded() const;
	bool inverted() const;
	bool gate_invert() const;

	Tristate &inverted(bool v);
	Tristate &gate_invert(bool v);

	void wr(float in);
	void input(Connection &in);
	void gate(Connection &gate);

	Connection &input();
	Connection &gate();
	Connection &rd();
};

//___________________________________________________________________________________
// Clamps a voltabe between a lower and upper bound.
class Clamp: public Device {
	Connection &m_in;
	float m_lo;
	float m_hi;

	void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	Clamp(Connection &in, float vLow=0.0, float vHigh=5.0);
	~Clamp();
};


//___________________________________________________________________________________
// A relay, such as a reed relay.  A signal applied closes the relay.  Functionally,
// this is almost identical to a Tristate.
class Relay: public Device {
  protected:
	Connection &m_in;
	Connection &m_sw;
	Connection m_out;

	void recalc_output();
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);
	virtual void on_sw_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	Relay(Connection &in, Connection &sw, const std::string &a_name="sw");
	virtual ~Relay();
	bool signal();

	Connection &in();
	Connection &sw();
	Connection &rd();
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

	void on_clock_change(Connection *Ck, const std::string &name, const std::vector<BYTE> &data);
	void on_data_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:   // pulsed==true simulates a D flip flop, otherwise its a transparent latch
	Latch(Connection &D, Connection &Ck, bool positive=false, bool clocked=true);
	virtual ~Latch();

	bool clocked();
	void set_name(const std::string &a_name);

	Connection &D();
	Connection &Ck();
	Connection &Q();
	Connection &Qc();
};

//___________________________________________________________________________________
//  A multiplexer.  The "select" signals are bits which make up an index into the input.
//    multiplexers can route both digital and analog signals.
class Mux: public Device {
	std::vector<Connection *> m_in;
	std::vector<Connection *> m_select;
	Output m_out;
	BYTE m_idx;

	void calculate_select();
	void set_output();
	virtual void on_change(Connection *D, const std::string &name);
	virtual void on_select(Connection *D, const std::string &name);

  public:
	Mux(const std::vector<Connection *> &in, const std::vector<Connection *> &select, const std::string &a_name="mux");
	virtual ~Mux();
	Connection &in(int n);
	Connection &select(int n);
	Connection &rd();
	size_t no_inputs();
	size_t no_selects();
};


//______________________________________________________________
// A FET approximation (voltage controlled switch).
class FET: public Device {
	Connection &m_in;
	Connection &m_gate;
	Connection m_out;
	bool       m_is_nType;

	void recalc();
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);
	virtual void on_output_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

public:
	FET(Connection &in, Connection &gate, bool is_nType = true, bool dbg=false);
	virtual ~FET();
	const Connection& in() const;
	const Connection& gate() const;
	Connection &rd();
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

	void recalc();
	void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	Schmitt(Connection &in, Connection &en, bool impeded=false, bool gate_invert=true, bool out_invert=false);
	Schmitt(Connection &in, bool impeded=false, bool out_invert=false);
	virtual ~Schmitt();

	void gate_invert(bool invert);
	void out_invert(bool invert);

	Connection &in();
	Connection &en();
	Connection &rd();
};

