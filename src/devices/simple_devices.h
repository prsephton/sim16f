#pragma once

#include "device_base.h"
#include "device_queue.h"

//___________________________________________________________________________________
// The hardware architecture uses several sequential logic components.  It would be
// really nice to be able to emulate the way the hardware works by stringing together
// elements and reacting to events that cause status to change.
//
// Here's a basic connection.  Changes to connection values always spawn and
// process device events.
//
// There is a [connection] instance for every output for a device.  This connection
//  has properties such as voltage and conductance for the device it represents.
// A Slot is used to represent a connection between devices.  It has an input
// connection, and a target device.  The same connection may be used in
// multiple slots.
//
// Not all components need slots- only those passive ones which affect
// a voltage at input.  Terminals, for example.
struct Slot: public ISlot {
	Device *dev;                  // target device
	Device *connection;           // this connection containing the slot

	Slot(Device *a_dev, Device *a_connection);
	virtual ~Slot() { }

	virtual void recalculate();
	virtual void unslot() { dev = NULL; }
};


class Connection: public Device {
	double m_V;                   //  A voltage on the connection
	double m_conductance;         //  Internal resistance [inverse] (1/ohm)
	bool   m_impeded;             //  The connection has an infinite resistance
	bool   m_determinate;         //  We know what the value of the voltage is
//	std::set<Slot *> m_slots;     //  The set of target slots for this connection.
	DeviceEventQueue eq;

  protected:
	bool impeded_suppress_change(bool a_impeded);

  public:
//	bool add_connection_slots(std::set<Slot *> &slots);
//	virtual std::vector<Device *> targets();

	virtual void query_voltage(int debug=0);
	virtual void update_voltage(double v);

	Connection(const std::string &a_name="");
	Connection(double V, bool impeded=true, const std::string &a_name="");
	virtual	~Connection();

	virtual void refresh();

	void queue_change(bool process_q = true, const std::string &a_comment="");
	virtual double rd(bool include_vdrop=true) const;

//	virtual bool connect(Connection &c) { return false; }
//	virtual void disconnect(Connection &c) {}
	virtual std::string info();

	double vDrop() const;
	virtual bool signal() const;
	virtual bool impeded() const;
	virtual bool determinate() const;
	void determinate(bool on);
	void conductance(double iR);
	virtual void R(double a_R);
	virtual double conductance() const;
	virtual double R() const;
//	virtual double I() const;
	virtual void impeded(bool a_impeded);
	virtual void set_value(double V, bool a_impeded);
};

//___________________________________________________________________________________
//  This singleton provides a clock signal to other components which may need
//  periodic updates or refresh cycles.
//  Simulation::speed() is a multiplier which controls how quickly
//
class Simulation {
	static Connection m_clock;
	static double m_speed;
  public:
	static Connection &clock() { return m_clock; }
	static double speed() { return m_speed; }   // a simulation speed multiplier
	static void speed(double a_speed) { m_speed = a_speed; }
	Simulation() {}
};

//___________________________________________________________________________________
//   A terminal for connections.  It is impeded by default, but any
// inputs connected to the pin will allow the pin itself to become an input.
// This makes a pin function a little like a "wire", but at the same time
// it is also itself a "connection".
//   A terminal without connections is always impeded (treated as an output).

class Terminal: public Connection, public PossibleTarget<Device> {
	bool m_terminal_impeded;
	int m_nslots = 1;

  protected:
	virtual void source_changed(Device *D, const std::string &name, const std::vector<BYTE> &data);
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	Terminal(const std::string &name="");
	Terminal(double V, const std::string &name="");
	virtual ~Terminal();

	virtual std::vector<Device *> sources() { return connected_sources(); }

	virtual void input_changed();
	virtual void query_voltage(int debug=0);
	void update_voltage(double v);
	void calc_conductance_precedents(double &Gin, double &Iin, double &Idrop);

	virtual int slot_id(int a_id) { return m_nslots++; }

	void impeded(bool a_impeded);
	virtual bool impeded() const;
	virtual double rd(bool include_vdrop=true) const;
};

//___________________________________________________________________________________
//  A resistor is just a terminal.

//___________________________________________________________________________________
// A [very basic] capacitor.
// This is a time frequency driven analog component.
// Changes in voltage are recalculated periodically, as determined by a signal
//   from Simulation::clock().
class Capacitor: public Terminal {
	double m_F;       // Capacitance in Farads
	double m_I = 0;   // current flowing
	double m_R = 0;   // Resistance factor
	time_stamp m_T;   // last time stamp

  protected:
	virtual void input_changed();
	void on_clock(Connection *c, const std::string &a_name, const std::vector<BYTE>&a_data);

  public:
	double F();
	void F(double a_F);
	void reset();
	double conductance() const { return 1/(1/Connection::conductance() + m_R); }
	virtual bool connect(Connection &c);
	Capacitor(const std::string name="");
	Capacitor(double V, const std::string &a_name);
	~Capacitor();
};


//___________________________________________________________________________________
// A [very basic] capacitor.
// This is a time frequency driven analog component.
class Inductor: public Terminal {
	double m_H;        // Inductance in Henry's
	time_stamp m_T;    // last time stamp
	double m_I = 0;    // Current
	double m_R = 0;

  protected:
	virtual void input_changed();
	void on_clock(Connection *c, const std::string &a_name, const std::vector<BYTE>&a_data);

  public:

	double H();
	void H(double a_H);
	void reset();
	double conductance() const { return 1/(1/Connection::conductance() + m_R); }
	virtual bool connect(Connection &c);
	Inductor(const std::string name="");
	Inductor(double V, const std::string &a_name);
	~Inductor();
};


//___________________________________________________________________________________
// Voltage source.   Always constant, no matter what.  Overrides connection V.
class Voltage: public Terminal {
//	double m_voltage;
  public:
	Voltage(double V, const std::string name="Vin");

	void voltage(double a_voltage) {
		Connection::set_value(a_voltage, false);
//		query_voltage();
	}
	virtual bool impeded() const;
	virtual bool determinate() const { return true; }

//	virtual double rd(bool include_vdrop=true) const;
};

//___________________________________________________________________________________
// A Weak Voltage source.  If there are any unimpeded connections, we calculate the
// lowest unimpeded, otherwise the highest impeded connection as the voltage
class PullUp: public Connection {
  public:
	PullUp(double V, const std::string name="Vin");
	virtual bool impeded() const;
	virtual bool determinate() const { return true; }
};

//___________________________________________________________________________________
// Ground.  Always zero.
class Ground: public Voltage {
  public:
	Ground();
	virtual bool impeded() const;
	virtual double rd(bool include_vdrop=true) const;
	virtual bool determinate() const { return true; }
	virtual double get_total_R() { return R(); }
};


//___________________________________________________________________________________
// An inverted connection.  If the connection was high, we return low, and vica versa.
class Inverse: public Connection {
	Connection &c;

	virtual void on_connection_change(Connection *conn, const std::string &name, const std::vector<BYTE> &data);

  public:

	Inverse(Connection &a_c);
	virtual  ~Inverse();

	virtual void set_value(double V, bool a_impeded);
	void impeded(bool a_impeded);
	void determinate(bool on);
};

//___________________________________________________________________________________
// Allows us to treat a connection as an output by setting impedence low.  An impeded
// connection has an arbitrary high resistance, while an unimpeded connection has
// an arbitrarily low resistance (a current source).
// In this digital model, we don't particularly care what the resistance is, or how
// much current flows.

class Output: public Terminal {
	Connection &c;
	bool wrapper;

	virtual void on_connection_change(Connection *c, const std::string &name, const std::vector<BYTE> &data);

  public:
	Output();
	Output(Connection &a_c);
	Output(double V, const std::string &a_name="");
	virtual ~Output();
	virtual bool signal() const;
	virtual double rd(bool include_vdrop=true) const;
	virtual bool impeded() const;
	virtual bool determinate() const;

	virtual void set_value(double V, bool a_impeded=true);
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
	Input(double V, const std::string &a_name="");
	virtual ~Input();

	virtual bool signal() const;
	virtual double rd(bool include_vdrop=true) const;
	virtual bool impeded() const;
	virtual bool determinate() const;

	virtual void set_value(double V, bool a_impeded=true);
	void impeded(bool a_impeded);
	void determinate(bool on);
};


//___________________________________________________________________________________
//  A generic gate
class Gate: public Device {
	std::vector<Connection *> m_in;
	Output m_out;
	bool m_inverted;

	virtual void recalc();
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	Gate(): Device(), m_inverted(false) {};
	Gate(const std::vector<Connection *> &in, bool inverted=false, const std::string &a_name="");
	virtual ~Gate();
	virtual bool connect(size_t a_pos, Connection &in);
	virtual void disconnect(size_t a_pos);
	void inverted(bool a_inverted) { m_inverted = a_inverted; }
	bool inverted() {return m_inverted; }
	void clone_output_name();
	std::vector<Connection *> &inputs() { return m_in; }
	void inputs(const std::vector<Connection *> &in);
	Connection &rd();
};

//___________________________________________________________________________________
// A buffer takes a weak high impedence input and outputs a strong signal
class ABuffer: public Gate {

	virtual void recalc();

  public:
	ABuffer(): Gate({}, false) {};
	ABuffer(Connection &in, const std::string &a_name="");
	virtual bool connect(size_t a_pos, Connection &in) { return Gate::connect(a_pos, in); }
	virtual bool connect(Connection &in) { return Gate::connect(0, in); }
};

//___________________________________________________________________________________
// Inverts a high impedence input and outputs a signal
class Inverter: public Gate {

  public:
	Inverter(): Gate({}, true) {};
	Inverter(Connection &in, const std::string &a_name="");
	virtual bool connect(size_t a_pos, Connection &in) { return Gate::connect(a_pos, in); }
	virtual bool connect(Connection &in) { return Gate::connect(0, in); }
};

//___________________________________________________________________________________
//  And gate, also nand for invert=true, or possibly doubles as buffer or inverter
class AndGate: public Gate {

	virtual void recalc();

  public:
	AndGate(): Gate() {};
	AndGate(const std::vector<Connection *> &in, bool inverted=false, const std::string &a_name="");
	virtual ~AndGate();
};

//___________________________________________________________________________________
//  Or gate, also nor for invert=true, or possibly doubles as buffer or inverter
class OrGate: public Gate {

	virtual void recalc();

  public:
	OrGate(): Gate() {};
	OrGate(const std::vector<Connection *> &in, bool inverted=false, const std::string &a_name="");
	virtual ~OrGate();
};

//___________________________________________________________________________________
//  Or gate, also nor for invert=true, or possibly doubles as buffer or inverter
class XOrGate: public Gate {

	virtual void recalc();

  public:
	XOrGate(): Gate() {}
	XOrGate(const std::vector<Connection *> &in, bool inverted=false, const std::string &a_name="");
	virtual ~XOrGate();
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

class Wire: public Device, public PossibleTarget<Device> {
	DeviceEventQueue eq;
//	std::map< Connection *, Slot * > connections;
//	bool indeterminate;
//	double Voltage;
//	double m_sum_conductance;
//	double m_sum_v_over_R;


//	double recalc();
//	bool assert_voltage();

	virtual void source_changed(Device *D, const std::string &name, const std::vector<BYTE> &data);
	void query_voltage(int debug=0);
	void queue_change();
//	void on_connection_change(Connection *conn, const std::string &name, const std::vector<BYTE> &data);

  public:
	Wire(const std::string &a_name="");
	Wire(Connection &from, Connection &to, const std::string &a_name="");
	virtual	~Wire();

	virtual std::vector<Device *> sources() { return connected_sources(); }

//	virtual bool connect(Connection &connection, const std::string &a_name="");
//	virtual void disconnect(const Connection &connection);

	double rd(bool include_vdrop=false);
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
	Tristate(): Device(), m_in(NULL), m_gate(NULL), m_invert_gate(false), m_invert_output(false) {}
	Tristate(Connection &in, Connection &gate, bool invert_gate=false, bool invert_output=false, const std::string &a_name="ts");
	virtual ~Tristate();
	void set_name(const std::string &a_name);
	bool signal() const;
	bool impeded() const;
	bool inverted() const;
	bool gate_invert() const;

	Tristate &inverted(bool v);
	Tristate &gate_invert(bool v);

	void wr(double in);
	void input(Connection *in);
	void gate(Connection *gate);

	Connection &input();
	Connection &gate();
	Connection &rd();
};

//___________________________________________________________________________________
// Clamps a voltage between a lower and upper bound.
class Clamp: public Device {
	Connection *m_in;
	double m_lo;
	double m_hi;

	void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	Clamp(): Device(), m_in(NULL), m_lo(0), m_hi(0) {}
	Clamp(Connection &in, double vLow=0.0, double vHigh=5.0);
	void reclamp(Connection &in);
	void unclamp();
	void limits(double a_lo, double a_hi) {m_lo = a_lo; m_hi = a_hi; }
	~Clamp();
};

//___________________________________________________________________________________
// A relay, such as a reed relay.  A signal applied closes the relay.  Functionally,
// this is almost identical to a TriState.
class Relay: public Device {
  protected:
	Connection *m_in;
	Connection *m_sw;
	Connection m_out;

	void recalc_output();
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);
	virtual void on_sw_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	Relay() : Device(), m_in(NULL), m_sw(NULL) {}
	Relay(Connection &in, Connection &sw, const std::string &a_name="sw");
	virtual ~Relay();
	bool signal();
	void in(Connection *in);
	void sw(Connection *sw);

	Connection &in();
	Connection &sw();
	Connection &rd();
};


//___________________________________________________________________________________
// A toggle switch.  A really simple device
class ToggleSwitch: public Device {
  protected:
	Connection *m_in;
	Connection m_out;
	bool m_closed;

	void recalc_output();
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	ToggleSwitch() : Device(), m_in(NULL), m_closed(false) { }
	ToggleSwitch(Connection &in, const std::string &a_name="sw");
	virtual ~ToggleSwitch();
	bool signal();
	void in(Connection *in);

	virtual std::vector<Device *> sources() { return {m_in}; }
	virtual std::vector<Device *> targets() { return {&m_out}; }

	virtual void input_changed();
	virtual double R() const;
	virtual const std::string &name() const { return Device::name(); }
	virtual void name(const std::string &a_name);

	bool closed();
	void closed(bool a_closed);
	Connection &in();
	Connection &rd();
};


//___________________________________________________________________________________
// A generalised D flip flop or a latch, depending on how we use it
class Latch: public Device {
	Connection *m_D;
	Connection *m_Ck;
	Output m_Q;
	Inverse m_Qc;
	bool m_positive;
	bool m_clocked;

	void on_clock_change(Connection *Ck, const std::string &name, const std::vector<BYTE> &data);
	void on_data_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:   // pulsed==true simulates a D flip flop, otherwise its a transparent latch
	Latch(): Device(), m_D(NULL), m_Ck(NULL), m_Qc(m_Q), m_positive(false),  m_clocked(true) {}
	Latch(Connection &D, Connection &Ck, bool positive=false, bool clocked=true);
	virtual ~Latch();

	void D(Connection *a_D);
	void Ck(Connection *a_Ck);
	void positive(bool a_positive);
	void clocked(bool a_clocked);
	bool positive();
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
	void subscribe_all();
	void unsubscribe_all();

  public:
	Mux(const std::vector<Connection *> &in, const std::vector<Connection *> &select, const std::string &a_name="mux");
	virtual ~Mux();

	void in(int n, Connection *c);
	void select(int n, Connection *c);
	void configure(int input_count, int gate_count);
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
	Slot 	  *m_slot = NULL;

	void recalc();
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);
	virtual void on_output_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);
	virtual Slot *slot(Device *d) { m_slot = new Slot(this, d); return m_slot; }
	virtual bool unslot(Device *dev) {
		if (m_slot) {
			delete m_slot;
			m_slot = NULL;
			return true;
		}
		return false;
	}

public:
	FET(Connection &in, Connection &gate, bool is_nType = true, bool dbg=false);
	virtual ~FET();
	const Connection& in() const;
	const Connection& gate() const;
	Connection &rd();
};

//______________________________________________________________
// A Diode approximation
class Diode: public Device {
	Connection &m_in;
	Connection m_out;
	Slot 	  *m_in_slot;

	void recalc();
	virtual void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);
	virtual bool unslot(Device *dev) { m_in_slot = NULL; return true; }
//	virtual std::vector<Device *> sources() { return {m_in}; }
//	virtual std::vector<Device *> targets() { return {&m_out}; }

public:
	Diode(Connection &in, Connection &gate, bool is_nType = true, bool dbg=false);
	virtual ~Diode();
	const Connection& in() const;
	Connection &rd();
};

//___________________________________________________________________________________
// Prevents a jittering signal from toggling between high/low states.
class Schmitt: public Device {
	Connection *m_in;
	Connection *m_enable;
	Connection 	m_enabled;
	Connection 	m_out;
	bool       	m_gate_invert;
	bool       	m_out_invert;

	const double m_lo = Vdd / 10.0 * 4;
	const double m_hi = Vdd / 10.0 * 6;

	void recalc();
	void on_change(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	Schmitt();
	Schmitt(Connection &in, Connection &en, bool impeded=false, bool gate_invert=true, bool out_invert=false);
	Schmitt(Connection &in, bool impeded=false, bool out_invert=false);
	virtual ~Schmitt();

	void gate_invert(bool invert);
	bool gate_invert() const { return m_gate_invert; }
	void out_invert(bool invert);
	bool out_invert() const { return m_out_invert; }

	void set_input(Connection *in);
	void set_gate(Connection *en);

	Connection &in();
	Connection &en();
	Connection &rd();
};


//___________________________________________________________________________________
// Trace signals.  One GUI representation will be a graphical signal tracer.
class SignalTrace: public Device {

  public:
	struct DataPoint {
		time_stamp ts;
		double v;
		DataPoint(time_stamp current_ts, double voltage) {
			ts = current_ts;
			v = voltage;
		}
		DataPoint(const DataPoint &dp): ts(dp.ts), v(dp.v) {
//			std::cout << "Copied data point " <<  ts.time_since_epoch().count() << " [" << v << "]" << std::endl;
		}
	};

  protected:
	std::vector<Connection *> m_values;
	std::chrono::microseconds m_duration_us;
	std::map<Connection *, std::queue<DataPoint> > m_times;
	std::map<Connection *, double > m_initial;

	void crop(time_stamp current_ts = current_time_us());
	void on_connection_change(Connection *c, const std::string &name, const std::vector<BYTE> &data);

  public:
	SignalTrace(const std::vector<Connection *> &in, const std::string &a_name="");
	~SignalTrace();

	virtual int slot_id(int a_id);
	virtual bool unslot(Device *dev);

	bool add_trace(Connection *c, int a_posn);
	bool has_trace(Connection *c);
	void remove_trace(Connection *c);
	void clear_traces();

	// returns a collated map of map<connection*, queue<datapoint> where each connection has an equal queue length.
	// and all columns are for the same time stamp.
	std::map<Connection *, std::queue<DataPoint> > collate();
	time_stamp current_us() const;
	time_stamp first_us() const;
	void set_duration(long a_duration_us);
	const std::vector<Connection *> & traced();
};

//___________________________________________________________________________________
// A binary counter.  If clock is set, it is synchronous, otherwise asynch ripple.
class Counter: public Device {
	Connection *m_in;
	Connection *m_clock;   // Synchronous counter
	bool m_rising;
	bool m_ripple;
	bool m_signal;
	Connection m_overflow;
	std::vector<Connection> m_bits;
	unsigned long m_value;
	Connection m_dummy;
	DeviceEventQueue eq;
	const unsigned int MAX_BITS = sizeof(m_value) * 8;

	void on_signal(Connection *c, const std::string &name, const std::vector<BYTE> &data);

	// synchronous counter on clock signal
	void on_clock(Connection *c, const std::string &name, const std::vector<BYTE> &data);

  public:
	Counter(unsigned int nbits, unsigned long a_value=0);
	Counter(Connection &a_in, bool rising, size_t nbits, unsigned long a_value=0, Connection *a_clock = NULL);
	~Counter();

	Connection &bit(size_t n);
	std::vector<Connection *> databits();
	void set_nbits(int nbits);
	void set_input(Connection *c);
	void set_clock(Connection *c);
	void set_name(const std::string &a_name);
	void set_value(unsigned long a_value);
	void set_rising(bool a_rising) { m_rising = a_rising; }
	bool get_rising() { return m_rising; }

	Connection *get_input() { return m_in; }
	Connection *get_clock() { return m_clock; }
	void asynch(bool a_ripple) { m_ripple = a_ripple; }  // first bit follows input
	bool is_sync() const { return (m_clock != NULL); }
	Connection &overflow() { return m_overflow; }
	size_t nbits() const { return m_bits.size(); }
	unsigned long get() const { return m_value; }

};

//___________________________________________________________________________________
// A binary Shift Register.
class ShiftRegister: public Device {
	Connection *m_in;
	Connection *m_shift_right;
	Connection *m_enable;
	Connection *m_clock;   // Synchronous ShiftRegister
	bool m_rising;
	Connection m_overflow;
	std::vector<Connection> m_bits;
	unsigned long long m_value;
	Connection m_high;
	Connection m_low;
	unsigned long long m_max_val;
	const unsigned int MAX_BITS = sizeof(m_value) * 8 - 1;

	// synchronous ShiftRegister on clock signal
	void on_clock(Connection *c, const std::string &name, const std::vector<BYTE> &data);

  public:
	ShiftRegister(unsigned int nbits, unsigned long a_value=0);
	ShiftRegister(Connection &a_in, bool rising, size_t nbits, unsigned long a_value=0, Connection *a_clock = NULL);
	~ShiftRegister();

	Connection &bit(size_t n);
	std::vector<Connection *> databits();
	void set_nbits(int nbits);

	void set_input(Connection *c);
	void set_clock(Connection *c);
	void set_shift_right(Connection *c);
	void set_enable(Connection *c);
	void set_name(const std::string &a_name);
	void set_value(unsigned long long a_value);
	void set_rising(bool a_rising);
	bool get_rising();

	Connection *get_input();
	Connection *get_clock();
	Connection *get_shift_right();
	Connection *get_enable();
	Connection &overflow();
	size_t nbits() const;
	unsigned long long get() const;

};


//___________________________________________________________________________________
// An LED panel.
class LEDPanel: public Device {
	std::vector<Connection *> m_bits;
	const unsigned int MAX_BITS = sizeof(long long);

	void on_changed(Connection *D, const std::string &name, const std::vector<BYTE> &data);

  public:
	LEDPanel(unsigned int nbits);
	~LEDPanel();

	double R() { return 230; }
	bool connect(Connection &from, int a_slot);
	void disconnect(Connection &from, int a_slot);

	Connection *bit(size_t n);
	std::vector<Connection *> databits();
	void set_nbits(int nbits);
	void set_name(const std::string &a_name);
	size_t nbits() const;
	unsigned long long get() const;
};



