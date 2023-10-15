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
#include <cassert>
#include <functional>
#include <mutex>
#include <chrono>
#include <thread>
#include "../utils/smart_ptr.h"
#include "../utils/utility.h"
#include "constants.h"
#include <cmath>

//__________________________________________________________________________________________________
// A small function to return a current time stamp in microseconds.
using time_stamp = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;

inline time_stamp current_time_us() {
	return std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
}

inline void sleep_for_us(unsigned long us) {
	std::this_thread::sleep_for(std::chrono::duration<unsigned long, std::micro>(us));
}

//__________________________________________________________________________________________________
//   A node represents a connection point between two or more electrical components
class Node {

  public:
	virtual Node *get_parent() = 0;
	virtual ~Node() {}
	virtual void process_model() = 0;
};

#define min_R (1.0e-12)
#define max_R (1.0e+12)


//__________________________________________________________________________________________________
// Devices have this minimal interface
struct IDevice {};

//__________________________________________________________________________________________________
// An interface for a slot.  A slot houses a connection between a source and destination device.
struct ISlot { // @suppress("Class has a virtual method and non-virtual destructor")
	virtual void recalculate() = 0;
	virtual void unslot() = 0;
};

//__________________________________________________________________________________________________
// A device which may be connected to target devices looks like this.
template <typename T> struct IConnectable {
	virtual ISlot *slot(T *to, T *from) = 0;
	virtual bool add_connection_slots(std::set<ISlot *> &slots) = 0;
	virtual bool unslot(T *dev) = 0;
	virtual void unslot_all_targets() = 0;
	virtual std::vector<T *> targets() = 0;
	virtual std::vector<ISlot *> slots() = 0;
	virtual ~IConnectable() {};
};

template <typename T>class PossibleSource: public IDevice {
	IConnectable<T> *m_targets = NULL;
  public:
	IConnectable<T> *connectable() const { return m_targets; }
	void connectable(IConnectable<T> *c) { if (m_targets) delete m_targets; m_targets = c; }

	virtual int slot_id(int a_id) { return a_id; }
	virtual ISlot *slot(T *dev) { if (m_targets) return m_targets->slot(dev, dynamic_cast<T *>(this)); return NULL; }
	virtual bool unslot(T *dev) { if (m_targets) return m_targets->unslot(dev); return true; }
	virtual void unslot_all_targets() {if (m_targets) m_targets->unslot_all_targets(); }
	virtual bool add_connection_slots(std::set<ISlot *> &slots) {
		if (m_targets) return m_targets->add_connection_slots(slots);
		return false;
	}
	virtual std::vector<T *> connected_targets() {
		if (m_targets) return m_targets->targets();
		return {};
	}
	virtual ~PossibleSource(){ connectable(NULL); }
};

//__________________________________________________________________________________________________
// An ITargetable device may accept connections from IConnectable devices.  Depending on the device,
// we can have one or more incoming slots.
template<typename T> struct ITargetable {
	virtual bool add_slots(std::set<ISlot *> &slots) = 0;
	virtual std::vector<T *> sources() = 0;
	virtual bool connect(T &from, T &to) = 0;
	virtual void disconnect(T &from) = 0;

	virtual ~ITargetable() {};
};

template<typename T> class PossibleTarget: public IDevice {
	ITargetable<T> *m_sources = NULL;

  public:
	ITargetable<T> *targetable() const { return m_sources; }
	void targetable(ITargetable<T> *a_sources) { if (m_sources) delete m_sources; m_sources = a_sources; }

	bool add_slots(std::set<ISlot *> &slots) {
		if (m_sources) return m_sources->add_slots(slots);
		return false;
	}
	virtual std::vector<T *> connected_sources() {
		if (m_sources) return m_sources->sources();
		return {};
	}
	virtual bool connect(T &c) {
		if (m_sources)
			return m_sources->connect(c, *dynamic_cast<T *>(this));
		return false;
	}
	virtual void disconnect(T &c) {
		if (m_sources) m_sources->disconnect(c);
	}
	virtual ~PossibleTarget() {
		targetable(NULL);
	}
};

//__________________________________________________________________________________________________
// Device definitions.  A basic device may be named and have a debug flag.
class Device: public PossibleSource<Device> {
	std::string m_name;
	bool m_debug;
	double amps = 0;

  public:
	static constexpr double Vss = 0.0;
	static constexpr double Vdd = 5.0;

	Device(): PossibleSource(), m_name(""), m_debug(false) {}
	Device(const Device &d): PossibleSource(), m_debug(false) {
		name(d.name());
	}
	Device(const std::string name): PossibleSource(), m_name(name), m_debug(false) {}
	virtual ~Device() {}

	Device &operator=(const Device &d) {
		name(d.name());
		return *this;
	}
	virtual void source_changed(Device *D, const std::string &name, const std::vector<BYTE> &data) {}
	virtual std::vector<Device *> sources() { return {}; }
	virtual std::vector<Device *> targets() { return connected_targets(); }

	virtual void query_voltage(int debug=0) {};
	virtual void update_voltage(double v) {};
	virtual void refresh() {};
	virtual double I() const { return amps; }
	virtual double R() const { return max_R; }
	virtual double conductance() const {  return min_R; }
	virtual double rd(bool include_vdrop=true) const { return 0; }
	virtual double vDrop() const { return I() * R(); }
	virtual bool impeded() const { return true; }

	virtual std::string info() { return "Name: " + name(); }
	virtual void I(double a_amps) { amps = a_amps; }
	void debug(bool flag) { m_debug = flag; }
	bool debug() const { return m_debug; }
	virtual const std::string &name() const { return m_name; }
	virtual void name(const std::string &a_name) { m_name = a_name; }

};

