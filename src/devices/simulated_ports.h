#include "device_base.h"
#include "register.h"
#include "sram.h"
#include "flags.h"
#include "clock.h"


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
	bool       porta_select;  // false is portb
	BYTE 	   port_mask;
	DeviceEventQueue eq;
	bool debug = false;

	virtual void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data);

public:
	std::map<std::string, SmartPtr<Device> > &components();

	void process_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data);

	BasicPort(Connection &a_Pin, const std::string &a_name, int port_no, int port_bit_ofs);
	Wire &bus_line();
	Connection &data();
	Connection &pin();
	Connection &read_port();
	Connection &read_tris();
};


//___________________________________________________________________________________
// Some components added specific to port A
class BasicPortA: public BasicPort {
protected:
	Connection S1;
	Connection S1_en;

public:
	BasicPortA(Connection &a_Pin, const std::string &a_name, int port_bit_ofs);
};


//___________________________________________________________________________________
//  A model for a most ports, which have a Tristate connected to the DataLatch
// and TrisLatch, and clamps the port range.
class SinglePortA: public BasicPortA {

  public:
	SinglePortA(Connection &a_Pin, const std::string &a_name, int port_bit_ofs);
};

//___________________________________________________________________8________________
//  A model for a single port for pins RA0/AN0, RA1/AN1
//  These are standard ports, but with a comparator output
class SinglePortA_Analog: public SinglePortA {

  protected:
	Connection Comparator;

	void set_comparator(bool on);
	void set_comparators_for_an0_and_an1(BYTE cmcon);
	virtual void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data);

  public:
	SinglePortA_Analog(Connection &a_Pin, const std::string &a_name);
	Connection &comparator();
};

//___________________________________________________________________________________
//  A model for a single port for pin AN2.  This looks like AN0/AN1 except that
//  it also has a voltage reference.
class SinglePortA_Analog_RA2: public  SinglePortA_Analog {
	Connection m_vref_sw;
	Connection m_vref_in;

	virtual void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data);

  public:
	SinglePortA_Analog_RA2(Connection &a_Pin, const std::string &a_name);
	Relay &VRef();
};

//___________________________________________________________________________________
//  A model for a single port for pin AN3.
//
// It differs from a standard port by the introduction of a Mux between the data latch
// and the tristate ouput.
//
// The mux selects a comparator output if the CMCON register has a comparator
// mode of 0b110, otherwise the Q output of the data latch is selected

class SinglePortA_Analog_RA3: public  BasicPortA {
	Connection &m_comparator_out;
	Connection m_cmp_mode_sw;
	Connection Comparator;

	void set_comparator(bool on);
	virtual void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data);

  public:
	SinglePortA_Analog_RA3(Connection &a_Pin, Connection &comparator_out, const std::string &a_name);
	Connection &comparator();
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

class SinglePortA_Analog_RA4: public BasicPortA {
	Connection &m_comparator_out;
	Connection m_cmp_mode_sw;

	void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data);


  public:
	SinglePortA_Analog_RA4(Connection &a_Pin, Connection &comparator_out, const std::string &a_name);
	Connection &TMR0();

};


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

class SinglePortA_MCLR_RA5: public Device {
	std::map<std::string, SmartPtr<Device> > m_components;
protected:
	Connection &Pin;
	Connection Data;    // This is the data bus value

	Connection rdPort;  // read port
	Connection rdTris;  // read tris
	Connection S1;      // Schmitt trigger 1 input
	Connection S2;      // Schmitt trigger 2 input
	Connection S2_en;   // Schmitt trigger 2 enable (active low)
	Connection cVss;    // We need a signal ground here
	Connection MCLRE;   // MCLR enable

	DeviceEventQueue eq;

	void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data);

  public:
	SinglePortA_MCLR_RA5(Connection &a_Pin, const std::string &a_name);
	Wire &bus_line();
	Connection &data();
	Connection &pin();
	std::map<std::string, SmartPtr<Device> > &components();
	Connection &read_port();
	Connection &read_tris();
};

//___________________________________________________________________________________
//  A model for the pin RA6/OSC2/CLKOUT
//  This time, there are the expected data and tris latches, and the read mechanism
// is comfortably familiar.
//  However, the internal oscillator circuit does potentially output the clock
// signal on this pin (FOSC = 011, 100, 110 with RA6=CLKOUT).   If RA6 is
// configured for I/O, CLKOUT can also be output with FOSC=101,111.
//  We should be able to derive this port from BasicPort.

class SinglePortA_RA6_CLKOUT: public  BasicPortA {
	Connection m_CLKOUT;
	Connection m_OSC;
	Connection m_Fosc1;
	Connection m_Fosc2;  // also s1_en
	BYTE m_fosc;

	virtual void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data);
	void on_clock_change(Clock *c, const std::string &name, const std::vector<BYTE> &data);

public:
	SinglePortA_RA6_CLKOUT(Connection &a_Pin, const std::string &a_name);
	Connection &fosc1();
	Connection &fosc2();
	Connection &osc();
};

//___________________________________________________________________8________________
// Port RA7/Osc1/CLKIN shares most features with a basic port.
// This means that it has the normal data and Tris latches and the output pin is
// clamped between Vss and Vdd.  It varies for Fosc modes 0b100 and 0b101, in that
// an internal oscillator determines whether RA7 can be used as an input/output pin.
// For anything other than these internal oscillator modes, the pin leads straight
// to the clock circuits.
class PortA_RA7: public BasicPortA {
	Connection m_Fosc;

  protected:
	virtual void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data);

  public:
	PortA_RA7(Connection &a_Pin, const std::string &a_name);
	Connection &Fosc();
};

//___________________________________________________________________________________
//  A model for a most PortB ports, which have a Tristate connected to the DataLatch
//  and TrisLatch, and clamps the port range.  PortB ports also have an optional
//  weak pull-up installed on the pin wire, such that we can drive an external
//  signal either as an active high, or as an active low.
class BasicPortB: public BasicPort {
	Connection m_RBPU;
	Connection m_PinOut;

protected:
	virtual void on_register_change(Register *r, const std::string &name, const std::vector<BYTE> &data);

  public:
	BasicPortB(Connection &a_Pin, const std::string &a_name, int port_bit_ofs);
	Connection &RBPU() { return m_RBPU; }
	Connection &PinOut() { return m_PinOut; }
};

//___________________________________________________________________________________
// RB0 is a basic I/O port, which can also drive an interrupt signal.
class PortB_RB0: public BasicPortB {
	Connection m_INT;

  public:
	PortB_RB0(Connection &a_Pin, const std::string &a_name);
	Connection &INT() { return m_INT; };
};

