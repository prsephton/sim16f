#include <cassert>
#include <cmath>
#include <iomanip>
#include "run_tests.h"
#include "../src/devices/device_base.h"


void test_connection() {
	std::cout << "Testing Connections" << std::endl;
	std::cout << "===================" << std::endl;

	Connection c0;
	assert(c0.impeded());           // A newly created connection is impeded
	assert(!c0.determinate());      // Unless specified, the value set is indeterminate
	std::cout << "Created a new default connection[C0], impeded and indeterminate" << std::endl;

	c0.set_value(5, true);     // set_value(V, impeded)
	assert(c0.rd() == 5);           // This connection is at 5v
	assert(c0.impeded());           // This connection is impeded
	assert(c0.determinate());       // The value is now determined
	std::cout << "[C0] now has a determined value of 5v, and is impeded" << std::endl;
	c0.impeded(false);
	assert(!c0.impeded());          // This connection is no longer impeded
	std::cout << "[C0] no longer has any resistence, and may be used as an output" << std::endl;

	Connection c1(5);               // Create a connection at 5v
	assert(c1.impeded());           // A newly created connection is impeded
	assert(c1.determinate());       // A new connection with a value set is determinate
	assert(c1.rd() == 5);           // This connection is at 5v
	std::cout << "Created a new connection[C1], impeded and determinate, at 5v" << std::endl;
	assert(c1.signal());            // c1 is emitting a signal
	std::cout << "[C1] at 5v emits a positive signal" << std::endl;
	c1.set_value(c1.Vss, false);     // set_value(V, impeded)
	assert(!c1.signal());            // c1 no longer emits a signal
	assert(c1.determinate());        // c1 still has a known value
	assert(!c1.impeded());           // c1 has zero resistence
	std::cout << "[C1] at 0v no longer emits a signal" << std::endl;
}


void test_terminals() {
	std::cout << "Testing Connections" << std::endl;
	std::cout << "===================" << std::endl;

	std::cout << "Terminals work just like connections excepting when there are other connections to the terminal" << std::endl;
	Terminal t0;
	assert(t0.impeded());            // A newly created connection is impeded
	assert(!t0.determinate());       // Unless specified, the value set is indeterminate
	std::cout << "Created a new default connection[T0], impeded and indeterminate" << std::endl;

	t0.set_value(5, true);           // set_value(V, impeded)
	assert(t0.rd() == 5);            // This connection is at 5v
	assert(t0.impeded());            // This connection is impeded
	assert(t0.determinate());        // The value is now determined
	std::cout << "[T0] now has a determined value of 5v, and is impeded" << std::endl;
	t0.impeded(false);
	assert(!t0.impeded());           // This connection is no longer impeded
	std::cout << "[T0] no longer has any resistence, and may be used as an output" << std::endl;

	Terminal t1(5);                  // Create a connection at 5v
	assert(t1.impeded());            // A newly created connection is impeded
	assert(t1.determinate());        // A new connection with a value set is determinate
	assert(t1.rd() == 5);            // This connection is at 5v
	std::cout << "Created a new connection[T1], impeded and determinate, at 5v" << std::endl;
	assert(t1.signal());             // c1 is emitting a signal
	std::cout << "[T1] at 5v emits a positive signal" << std::endl;
	t1.set_value(t1.Vss, false);     // set_value(V, impeded)
	assert(!t1.signal());            // c1 no longer emits a signal
	assert(t1.determinate());        // c1 still has a known value
	assert(!t1.impeded());           // c1 has zero resistence
	std::cout << "[T1] at 0v no longer emits a signal" << std::endl;

	/*
	 *  Now some more interesting behaviour;
	 *  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	 *  When a terminal has connections, the terminal manages those connections in the same way
	 *    a Device::Wire does, but represents the sum of those connections as a single connection.
	 *  A wire, or another device connecting to the terminal will then see an output connection
	 *    if all connections in the terminal pool are outputs.  If one of the pool is an input,
	 *    the terminal will represent that input connection.
	 *  If more than one input happens to be in the pool, the terminal  pretends to be a wire to
	 *    the pool, and appears to be a single connection on the other (connecting) side.
	 */

	t1.set_value(t1.Vdd, true);     // set_value(V, impeded)
//	t1.debug(true);
	t1.set_vdrop(3);                 // tell t1 that there is a voltage drop from 5v to 3v over t1.R
	std::cout << "[T1] can be told that there is a voltage drop over its internal resistance." << std::endl;
	std::cout << "   For example, we have just told T1 that it has 3v at its output instead of 5v." << std::endl;
	assert(t1.vDrop() == -2);
	std::cout << "   The voltage difference is 3v - 5v = -2v." << std::endl;
	assert(t1.rd() == 3);
	std::cout << "   When we read T1, we see a value of 3v." << std::endl;
	t1.R(100);                       // pretend we have a 100 ohm internal resistance
	std::cout << "   If we tell T1 that it's internal resistance is 100 Ohm," << std::endl;
	assert(fabs(t1.I() + 0.02) < 1e-6);    // sounds right: 2v / 100 Ohm = 20 mA
	std::cout << "   then we can query it's current.  I = V/R = -2/100 = 0.02A, or 20mA." << std::endl;
	assert(t1.signal());
	std::cout << "   Since the terminal output is 3V, it emits a positive signal." << std::endl;
	t1.set_vdrop(2);                 // tell t1 that there is a voltage drop from 5v to 2v over t1.R
	std::cout << "   If we set the output voltage at 2v instead of 3v," << std::endl;

	assert(not t1.signal());         // no longer emit a signal below the limit
	std::cout << "   T1 no longer emits a signal." << std::endl;
	assert(t1.rd() == 2);            // our external voltage is 2v
	assert(t1.rd(false) == 5);       // our internal voltage is still 5v
	std::cout << "   T1 reads an output of 2v when queried, but T1 still retains it's internal voltage at 5v." << std::endl;
	assert(t1.impeded());
	std::cout << "   All of this behaviour works whether or not T1 is set as an input or output." << std::endl;

	Connection c0("Out1");
	Connection c1("4v");
	Connection c2("5v");

	c0.set_value(4, true);           // an impeded connection
	c1.set_value(4, false);          // an voltage source at 4v
	c1.R(100);                       // with an internal resistance of 100 Ohm
	c2.set_value(5, false);          // an voltage source at 5v
	c2.R(50);                        // with an internal resistance of 100 Ohm

	std::cout << "Let us create three new connections:" << std::endl;
	std::cout << "          c0[output] is 4v." << std::endl;
	std::cout << "          c1[input] is 4v, with a resistance of 100 Ohm" << std::endl;
	std::cout << "          c2[input] is 5v, with a resistance of 50 Ohm" << std::endl;

	t1.connect(c0);
	assert(t1.impeded());
	std::cout << "  If we connect just C1, T1 remains impeded." << std::endl;


	t1.connect(c1);                  // Connect them into the terminal
	t1.connect(c2);
	assert(not t1.impeded());
	std::cout << "  ... but as soon as we add the two inputs c1 & c2 to the termninal pool," << std::endl;
	std::cout << "      t1 immediately shows itself to be unimpeded (IOW, an input too)." << std::endl;

	/*
	 *  I1 + I2 = 0
	 *  (V1 - Vi)/R1 + (V2 - Vi)/R2 = 0          :    I = V/R, Vi is the common terminal voltage
	 *  4v/100 - Vi/100 + 5v/50 - Vi/50 = 0      :    substitute, simplify
	 *  -Vi/100 - Vi/50 = -4v/100 - 5v/50
	 *  -Vi/100 - 2Vi/100 = -4v/100 - 10v/100    : common denominator
	 *  Vi + 2Vi = 4v + 10v                      : multiply by -100
	 *  3Vi = 14v
	 *  Vi = 14/3 = 4.66667v
	 */
	assert(t1.rd() - 4.66667 < 1e-6);
	assert(c0.rd() - 4.66667 < 1e-6); // check the output too
	assert(t1.R() - 33.3333 < 1.0e-4);

	std::cout << "  T1 does much more than that.  It calculates the voltage output from T1 as 4.6667v," << std::endl;
	std::cout << "    and updates the output c0 to reflect that voltage as well." << std::endl;
	std::cout << "  Furthermore, T1 calculates the resistance across the inputs to be 33.3333 Ohm." << std::endl;
	std::cout << "    and updates itself to reflect those parameters as internal voltage and resistance." << std::endl;

	// t1 now looks like a voltage source of 4.66667v behind an internal resistance of 1/(1/R1 + 1/R2)= 33.3333 Ohms
	//     this simplifies things a whole lot going forward.
	t1.set_vdrop(3);                  // tell t1 that there is a voltage drop from 4.66667 to 3v.
	std::cout << t1.vDrop() << std::endl;
	assert(t1.vDrop() + 1.666667 < 1e-4);
	assert(t1.I() + 0.05 < 1e-3);    //  A vDrop of  3 - 4.667 = -1.667v over R=33.33 gives I = V/R = 1.667/33.33 = 0.05A, or 50mA

	std::cout << "  If we give T1 an output voltage of 3V now, it calculate sthe voltage drop as " << std::endl;
	std::cout << "       vDrop =  3v - 4.6667v" << std::endl;
	std::cout << "             =  -1.6667v" << std::endl;
	std::cout << "  We can now also query the current " << std::endl;
	std::cout << "            I = V/R " << std::endl;
	std::cout << "              = -1.6667v/33.3333 Ohm" << std::endl;
	std::cout << "              = 0.05A (or 50 mA)" << std::endl;

	c1.impeded(true);   // turn c1 into an output
	c2.impeded(true);   // turn c2 into an output too

	assert(t1.impeded());  // a pool of impeded-only connections reverts a terminal to its original setting

	std::cout << "  If we change the two input connections in the terminal pool (c1 & c2) to outputs," << std::endl;
	std::cout << "    then T1 reflects that change by itself showing as a high impedance output." << std::endl;
}

void test_rails() {
	std::cout << "Testing Rails" << std::endl;
	std::cout << "=============" << std::endl;

	Voltage Vcc(5);
	Ground  GND;

	assert(Vcc.rd() == 5);
	assert(GND.rd() == 0);
	std::cout << "Created Vcc at 5V and GND at 0V" << std::endl;

	Vcc.set_value(10, true);
	GND.set_value(10, true);

	assert(Vcc.rd() == 5);
	assert(GND.rd() == 0);

	assert(!Vcc.impeded());
	assert(!GND.impeded());

	std::cout << "After changing value, Vcc is still at 5V and GND is still at 0V and unimpeded" << std::endl;
}

void test_wires() {
	std::cout << "Testing Wires" << std::endl;
	std::cout << "=============" << std::endl;
	Connection c[3] = {Connection("c0"),Connection("c1"),Connection("c2") };
	Wire w("wire");
//	c[0].debug(true);
//	w.debug(true);
	for (int n=0; n<3; ++n)
		w.connect(c[n]);
	assert(!w.determinate());
	std::cout << "A wire with three indeterminate connections is indeterminate" << std::endl;
	for (int n=0; n<3; ++n)
		c[n].set_value(0, true);
	assert(!w.determinate());
	std::cout << "A wire with three impeded connections is indeterminate" << std::endl;
	for (int n=0; n < 3; ++n)
		assert(!c[n].determinate());
	std::cout << "... and the impeded connections are also indeterminate" << std::endl;

	c[0].set_value(0, false);
	assert(w.determinate());
	assert(w.rd() == c[0].rd());
	std::cout << "A wire with two impeded connections and one input has V=input.V" << std::endl;
	for (int n=1; n < 3; ++n) {
		assert(c[n].determinate());
		assert(c[n].rd() == c[0].rd());
	}
	std::cout << "... and all impeded connections are determined by input.V" << std::endl;

	c[0].set_value(5, false);
	c[1].set_value(3, false);
	assert(w.rd() == 4);
	std::cout << "A wire voltage is calculated based on resistance of deterministic inputs" << std::endl;
	assert(c[2].rd() == 4);
	std::cout << "... and any impeded connections (outputs) are determined by wire.V" << std::endl;

//	Terminal t("TRM");
//	c[0].set_value(1, false);
//	t.set_value(5, false);
//	t.R(10000);
//
//	t.debug(true);
//	w.connect(t);
//	std::cout << t.rd() << std::endl;
//	std::cout << t.vDrop() << std::endl;
//	std::cout << w.rd() << std::endl;
//
//
//	exit(0);
}

void test_inverse() {
	std::cout << "Testing Inverse" << std::endl;
	std::cout << "===============" << std::endl;
	Connection c;
	Inverse i(c);

	c.set_value(c.Vdd, false);
	assert(i.rd() == i.Vss);
	std::cout << "Inverse(Connection(Vdd)) = Vss" << std::endl;

	c.set_value(c.Vss, false);
	assert(i.rd() == i.Vdd);
	std::cout << "Inverse(Connection(Vss)) = Vdd" << std::endl;

	i.set_value(c.Vdd, false);
	assert(i.rd() == i.Vss);
	std::cout << "Inverse(Inverse(Vdd)) = Vss" << std::endl;

	i.set_value(c.Vss, false);
	assert(i.rd() == i.Vdd);
	std::cout << "Inverse(Inverse(Vss)) = Vdd" << std::endl;
}

void test_input_output() {
	std::cout << "Testing Input" << std::endl;
	std::cout << "===============" << std::endl;
	Connection c;
	Input i(c);
	Output o(c);

	c.set_value(c.Vdd, false);
	assert(i.rd() == i.Vdd);
	assert(i.impeded() == true);
	std::cout << "A connection mimics an impeded input" << std::endl;

	assert(o.rd() == i.Vdd);
	assert(o.impeded() == false);
	std::cout << "... while simultaneously mimicing an unimpeded output" << std::endl;

	Output o2(5);
	assert(o2.rd() == 5);

	Input i2(5);
	assert(i2.rd() == 5);

	std::cout << "Inputs & Outputs can mirror existing connections, or be declared independently" << std::endl;
}


void test_abuffer() {
	std::cout << "Testing ABuffer" << std::endl;
	std::cout << "===============" << std::endl;

	Connection c;
	ABuffer b(c);

	c.set_value(5, true);
	assert(!b.rd().impeded());
	assert(b.rd().rd() == 5);
	std::cout << "A buffer reads an input value, and produces the same value as output" << std::endl;

	c.set_value(3, false);
	assert(!b.rd().impeded());
	assert(b.rd().rd() == 3);
	std::cout << "... or reads and reproduces an output signal, as a separate output connection" << std::endl;

}

void test_inverter() {
	std::cout << "Testing Inverter" << std::endl;
	std::cout << "================" << std::endl;

	Connection c;
	Inverter i(c);

	c.set_value(c.Vdd, true);
	assert(!i.rd().impeded());
	assert(!i.rd().signal());
	std::cout << "An inverter acts like a buffer, except it returns Vss given Vdd, " << std::endl;

	c.set_value(c.Vss, true);
	assert(!i.rd().impeded());
	assert(i.rd().signal());
	std::cout << "... or returns Vdd given Vss, " << std::endl;

}

void test_and_gate() {
	std::cout << "Testing the AND Gate" << std::endl;
	std::cout << "====================" << std::endl;

	Input c1;
	Input c2;

	AndGate and_gate({&c1, &c2}, false);
	AndGate nand_gate({&c1, &c2}, true);

	std::cout << "c1" << "\t" << "c2" << "\tand" << "\tnand" << std::endl;
	std::cout << "_____________________________" << std::endl;
	c1.set_value(c1.Vss, true); c2.set_value(c1.Vss, true);
	assert(and_gate.rd().signal() == false);
	assert(nand_gate.rd().signal());
	std::cout << " " << c1.signal() << "\t " << c2.signal() << "\t " << and_gate.rd().signal() << "\t " << nand_gate.rd().signal() << std::endl;

	c1.set_value(c1.Vdd, true); c2.set_value(c1.Vss, true);
	assert(and_gate.rd().signal() == false);
	assert(nand_gate.rd().signal());
	std::cout << " " << c1.signal() << "\t " << c2.signal() << "\t " << and_gate.rd().signal() << "\t " << nand_gate.rd().signal() << std::endl;

	c1.set_value(c1.Vss, true); c2.set_value(c1.Vdd, true);
	assert(and_gate.rd().signal() == false);
	assert(nand_gate.rd().signal());
	std::cout << " " << c1.signal() << "\t " << c2.signal() << "\t " << and_gate.rd().signal() << "\t " << nand_gate.rd().signal() << std::endl;

	c1.set_value(c1.Vdd, true); c2.set_value(c1.Vdd, true);
	assert(and_gate.rd().signal());
	assert(nand_gate.rd().signal() == false);
	std::cout << " " << c1.signal() << "\t " << c2.signal() << "\t " << and_gate.rd().signal() << "\t " << nand_gate.rd().signal() << std::endl;
}


void test_or_gate() {
	std::cout << "Testing the OR Gate" << std::endl;
	std::cout << "===================" << std::endl;

	Input c1;
	Input c2;

	OrGate or_gate({&c1, &c2}, false);
	OrGate nor_gate({&c1, &c2}, true);

	std::cout << "c1" << "\t" << "c2" << "\tor" << "\tnor" << std::endl;
	std::cout << "_____________________________" << std::endl;
	c1.set_value(c1.Vss, true); c2.set_value(c1.Vss, true);
	assert(or_gate.rd().signal() == false);
	assert(nor_gate.rd().signal());
	std::cout << " " << c1.signal() << "\t " << c2.signal() << "\t " << or_gate.rd().signal() << "\t " << nor_gate.rd().signal() << std::endl;

	c1.set_value(c1.Vdd, true); c2.set_value(c1.Vss, true);
	assert(or_gate.rd().signal());
	assert(!nor_gate.rd().signal());
	std::cout << " " << c1.signal() << "\t " << c2.signal() << "\t " << or_gate.rd().signal() << "\t " << nor_gate.rd().signal() << std::endl;

	c1.set_value(c1.Vss, true); c2.set_value(c1.Vdd, true);
	assert(or_gate.rd().signal());
	assert(!nor_gate.rd().signal());
	std::cout << " " << c1.signal() << "\t " << c2.signal() << "\t " << or_gate.rd().signal() << "\t " << nor_gate.rd().signal() << std::endl;

	c1.set_value(c1.Vdd, true); c2.set_value(c1.Vdd, true);
	assert(or_gate.rd().signal());
	assert(!nor_gate.rd().signal());
	std::cout << " " << c1.signal() << "\t " << c2.signal() << "\t " << or_gate.rd().signal() << "\t " << nor_gate.rd().signal() << std::endl;
}


void test_xor_gate() {
	std::cout << "Testing the XOR Gate" << std::endl;
	std::cout << "====================" << std::endl;

	Input c1;
	Input c2;

	XOrGate xor_gate({&c1, &c2}, false);
	XOrGate nxor_gate({&c1, &c2}, true);

	std::cout << "c1" << "\t" << "c2" << "\txor" << "\tnxor" << std::endl;
	std::cout << "_____________________________" << std::endl;
	c1.set_value(c1.Vss, true); c2.set_value(c1.Vss, true);
	assert(xor_gate.rd().signal() == false);
	assert(nxor_gate.rd().signal());
	std::cout << " " << c1.signal() << "\t " << c2.signal() << "\t " << xor_gate.rd().signal() << "\t " << nxor_gate.rd().signal() << std::endl;

	c1.set_value(c1.Vdd, true); c2.set_value(c1.Vss, true);
	assert(xor_gate.rd().signal());
	assert(!nxor_gate.rd().signal());
	std::cout << " " << c1.signal() << "\t " << c2.signal() << "\t " << xor_gate.rd().signal() << "\t " << nxor_gate.rd().signal() << std::endl;

	c1.set_value(c1.Vss, true); c2.set_value(c1.Vdd, true);
	assert(xor_gate.rd().signal());
	assert(!nxor_gate.rd().signal());
	std::cout << " " << c1.signal() << "\t " << c2.signal() << "\t " << xor_gate.rd().signal() << "\t " << nxor_gate.rd().signal() << std::endl;

	c1.set_value(c1.Vdd, true); c2.set_value(c1.Vdd, true);
	assert(!xor_gate.rd().signal());
	assert(nxor_gate.rd().signal());
	std::cout << " " << c1.signal() << "\t " << c2.signal() << "\t " << xor_gate.rd().signal() << "\t " << nxor_gate.rd().signal() << std::endl;
}

void test_tristate() {
	std::cout << "Testing the TriState Buffer" << std::endl;
	std::cout << "===========================" << std::endl;

	Input c1;
	Input en;

	Tristate ts(c1, en);

	c1.set_value(c1.Vdd);
	en.set_value(c1.Vss);
	assert(ts.rd().rd() == ts.Vss);
	std::cout << "Tristate with a disabled gate output is at ground" << std::endl;
	assert(ts.rd().impeded());
	std::cout << "Tristate with a disabled gate output is at high impedence" << std::endl;

	en.set_value(c1.Vdd);
	assert(!ts.rd().impeded());
	assert(ts.rd().signal());
	c1.set_value(c1.Vss);
	assert(!ts.rd().signal());
	std::cout << "Tristate with an enabled gate output reflects input signal" << std::endl;

	ts.inverted(true);
	assert(ts.rd().signal());
	c1.set_value(c1.Vdd);
	assert(!ts.rd().signal());
	std::cout << "Tristate with an inverted output reflects an inverted input" << std::endl;

	ts.gate_invert(true);
	assert(ts.rd().impeded());
	assert(!ts.rd().signal());
	std::cout << "Tristate inverted gate[high] disables output" << std::endl;
	en.set_value(c1.Vss);
	c1.set_value(c1.Vss);
	assert(!ts.rd().impeded());
	assert(ts.rd().signal());
	std::cout << "Tristate inverted gate[low] and inverted output reflects inverted input" << std::endl;

}

void test_relay() {
	std::cout << "Testing the Relay" << std::endl;
	std::cout << "=================" << std::endl;

	Connection c1;
	Input sw;
	c1.name("relay-c1");
	sw.name("relay-sw");

	Relay r(c1, sw);

	sw.set_value(sw.Vss);
	c1.set_value(sw.Vdd, false);

	assert(r.rd().impeded());
	std::cout << "A relay output is an open circuit for an open switch" << std::endl;

	sw.set_value(sw.Vdd);
	assert(r.rd().rd() == sw.Vdd);
	assert(!r.rd().impeded());
	c1.set_value(c1.Vss, false);
	assert(r.rd().rd() == c1.Vss);
	std::cout << "A relay output follows input with a closed switch" << std::endl;

}

void test_clamp() {
	std::cout << "Testing a Clamp" << std::endl;
	std::cout << "===============" << std::endl;

	Connection c1;
	Clamp c(c1);

	c1.set_value(3, true);
	assert(c1.rd() == 3);
	std::cout << "A clamp allows voltages between limits to be set on a connection" << std::endl;

	c1.set_value(-1, true);
	assert(c1.rd()==0);
	std::cout << "... but any value below the minimum will set the connection to minimum" << std::endl;

	c1.set_value(6, true);
	assert(c1.rd() == 5);
	std::cout << "... and any value above the maximum will set the connection to maximum" << std::endl;
}

void test_DFF() {
	std::cout << "Testing a Latch- D flip flop" << std::endl;
	std::cout << "============================" << std::endl;
	Input data, en;

	Latch l(data, en);  // default transparent latch with trigger on enable low (clock signal)

	assert(!l.Q().signal());   // Latch starts up with Q low
	assert(l.Qc().signal());   // and Qc high
	std::cout << "A default D flip-flop latch starts up with Q low and Qc high" << std::endl;

	data.set_value(data.Vdd);
	assert(l.Qc().signal());   // setting data high does not alter things
	std::cout << "Changing the data signal from low to high does not change things" << std::endl;

	data.set_value(data.Vss);
	assert(l.Qc().signal());   // nor does setting data low
	std::cout << "... even if you toggle data back low" << std::endl;

	data.set_value(data.Vdd);  // setting data high,
	en.set_value(en.Vdd);      // and adding an enable signal
	assert(l.Qc().signal());   // still does not change things
	std::cout << "If you take data high, and add an enable signal, Q remains low" << std::endl;

	en.set_value(en.Vss);      // but taking enable from high to low
	assert(l.Q().signal());    // latches the data value into place
	std::cout << "  but if you change enable from high to low, data is latched into Q" << std::endl;

	data.set_value(data.Vss);  // setting data low again
	assert(l.Q().signal());    // does not change the latched value
	std::cout << "Removing the data signal does not alter the Q output" << std::endl;

	en.set_value(en.Vdd);      // but toggling enable signal high
	assert(l.Q().signal());    // ...
	en.set_value(en.Vss);      // and then low again
	assert(l.Qc().signal());   // latches a new low signal from data
	std::cout << "  but clocking the enable input high, then low, again latches Q=data" << std::endl;
}

void test_latch() {
	std::cout << "Testing a Latch, SR mode" << std::endl;
	std::cout << "========================" << std::endl;
	Input data, en;
	Latch sr(data, en, true, false);  // the other mode where we latch on enable

	assert(!sr.Q().signal());   // SR Latch starts up with Q low
	assert(sr.Qc().signal());   // and Qc high
	std::cout << "An SR latch starts up with Q low and Qc high" << std::endl;

	data.set_value(data.Vdd);
	assert(sr.Qc().signal());   // setting data high does not alter things
	std::cout << "Changing the data signal from low to high does not change things" << std::endl;

	data.set_value(data.Vss);
	assert(sr.Qc().signal());   // nor does setting data low
	std::cout << "... even if you toggle data back low" << std::endl;

	en.set_value(en.Vdd);      // the enable signal is needed
	data.set_value(data.Vdd);  // to change the latch output
	assert(sr.Q().signal());
	std::cout << "If we change enable to high, and change data input signal," << std::endl;
	data.set_value(data.Vss);  // and output follows input while enable is high
	assert(sr.Qc().signal());
	std::cout << "... then Q reflects data input signal while enable (reset) is high." << std::endl;

	en.set_value(en.Vss);      // disable latch
	data.set_value(data.Vdd);  // to change the latch output
	assert(sr.Qc().signal());
	std::cout << "If we disable the latch, then set data high, this does not reflect in latch.Q" << std::endl;

	en.set_value(en.Vdd);      // enable latch
	en.set_value(en.Vss);      // disable latch
	assert(sr.Q().signal());
	std::cout << "... but if we then enable and immediately disable the latch," << std::endl << "... then latch.Q is updated to reflect the data signal." << std::endl;
}

void test_mux() {
	std::cout << "Testing a MUX" << std::endl;
	std::cout << "=============" << std::endl;

	Input c1, c2;
	Input s;

	Mux mux({&c1, &c2}, {&s});  // connection list, select mask

	assert(mux.rd().signal() == c1.signal());
	s.set_value(s.Vdd);
	assert(mux.rd().signal() == c2.signal());
	std::cout << "sel\tdata0\tdata1\tdout" << std::endl;
	std::cout << "________________________________" << std::endl;
	for (int q=0; q < 2; ++q) {
		s.set_value(q * s.Vdd);
		for (int r=0; r < 4; ++r) {
			std::cout << " " << q << "\t ";
			c1.set_value((r % 2) * c1.Vdd) ;
			c2.set_value((r / 2) * c2.Vdd) ;
			std::cout << c1.signal() << "\t ";
			std::cout << c2.signal() << "\t ";
			std::cout << mux.rd().signal();
			assert(mux.rd().signal() == (q?c2.signal():c1.signal()));
			std::cout << std::endl;
		}
	}
}

void test_FET() {
	std::cout << "Testing a nFET & pFET" << std::endl;
	std::cout << "=====================" << std::endl;

	Connection in, i2, gate;
	Ground gnd;

	FET nFET(in, gate, true);         // positive signal at gate allows current through device
	FET pFET(i2, gate, false);        // negative signal at gate allows current through device

	gnd.connect(nFET.rd());
	gnd.connect(pFET.rd());

	assert(nFET.rd().rd(false) == in.Vss);
	assert(pFET.rd().rd(false) == i2.Vss);
	std::cout << "Similar to a voltage controlled switch, a FET requires an input voltage" << std::endl;

	in.set_value(in.Vdd, true);
	i2.set_value(i2.Vdd, true);
	assert(nFET.rd().rd(false) == in.Vss);

	std::cout << "An nFET conducts with a positive gate signal" << std::endl;
	assert(pFET.rd().rd(false) == i2.Vdd);
	std::cout << " and a pFET conducts with a negative gate signal" << std::endl;

	gate.set_value(gate.Vdd, true);
	assert(nFET.rd().rd(false) == in.Vdd);
	assert(pFET.rd().rd(false) == i2.Vss);
	std::cout << "So switching gate voltage with respect to source voltage" << std::endl;
	std::cout << "  lets you control current between drain (Anode) and source (Cathode)" << std::endl;
}

void test_schmitt() {
	std::cout << "Testing the Schmitt trigger" << std::endl;
	std::cout << "===========================" << std::endl;

	Connection c1;
	Connection en;
	c1.name("schmitt-c1");
	en.name("schmitt-en");

	// Schmitt(connection, enable, out_impeded, gate_invert, out_invert
	Schmitt S1(c1, en, false, false, false);
	c1.set_value(5, true);    // set_value(voltage, impeded)

	en.set_value(0, true);       // dinable s.gate
	assert( S1.rd().rd() == 0 );
	std::cout << "Schmitt(Vcc, en[false], igate=false, iout=false) -> 0V" << std::endl;
	assert( S1.rd().impeded() );  // disabling sets the output to high impedence

	en.set_value(5, true);    // enable s.gate, expect 5v on output.
	assert( S1.rd().rd() == 5 );
	std::cout << "Schmitt(Vcc, en[true], igate=false, iout=false) -> Vcc" << std::endl;
	assert( !S1.rd().impeded() );  // enabling sets the output to unimpeded (output)

	S1.gate_invert(true);    // invert the gate signal.  This should disable the signal, so we should see 0V.
	assert( S1.rd().rd() == 0 );
	std::cout << "Schmitt(Vcc, en[true], igate=true, iout=false) -> 0V" << std::endl;

	S1.out_invert(true);     // invert the output.  Since we are disabled, we should still see 0V
	assert( S1.rd().rd() == 0 );
	std::cout << "Schmitt(Vcc, en[true], igate=true, iout=true) -> 0V" << std::endl;

	en.set_value(0, true);    // A 0V on an inverted gate enables.  We expect an inverted output, or 0V
	assert( S1.rd().rd() == 0 );
	std::cout << "Schmitt(Vcc, en[true], igate=true, iout=true) -> 0V" << std::endl;

	c1.set_value(0, true);    // For an input at ground, am inverted value should be 5V
	assert( S1.rd().rd() == 5 );
	std::cout << "Schmitt(GND, en[true], igate=true, iout=true) -> Vcc" << std::endl;

	S1.rd().impeded(true);    // Changing the output to impeded should leave it at 5V, as it is not connected to a wire
	assert( S1.rd().rd() == 5 );
	std::cout << "Schmitt(GND, en[true], imp[out]=true, igate=true, iout=true) -> Vcc" << std::endl;

	S1.out_invert(false);     // For the next part, remove the inverter on output.  We will test hysteresis
	std::cout << std::setprecision(5);
	const int samples = 18;   // 0 .. samples
	for (int n = 0; n <= samples; ++n) {     // samples+1 steps
		c1.set_value(5.0*n/samples, true);
		std::cout << std::setw(4) << S1.rd().signal();
		if (n == samples / 2)    // mid point
			assert(S1.rd().signal() == false);
	}
	std::cout << std::endl;
	for (int n = samples; n >= 0; --n) {     // samples+1 steps
		c1.set_value(5.0*n/samples, true);
		std::cout << std::setw(4) << S1.rd().signal();
		if (n == samples / 2)    // mid point
			assert(S1.rd().signal() == true);
	}
	std::cout << std::endl;
};

void prsep() {
	DeviceEventQueue eq;
	assert(!eq.size());
	std::cout << "______________________________________________________________________________" << std::endl;
}

#ifdef TESTING

void test_devices() {
	prsep(); test_connection();
	prsep(); test_terminals();
	prsep(); test_rails();
	prsep(); test_inverse();
	prsep(); test_input_output();
	prsep(); test_abuffer();
	prsep(); test_inverter();
	prsep(); test_and_gate();
	prsep(); test_or_gate();
	prsep(); test_xor_gate();
	prsep(); test_wires();
	prsep(); test_tristate();
	prsep(); test_relay();
	prsep(); test_clamp();
	prsep(); test_DFF();
	prsep(); test_latch();
	prsep(); test_mux();
	prsep(); test_FET();
	prsep(); test_schmitt();
	prsep();
};

#endif
