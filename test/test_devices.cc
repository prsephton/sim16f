#include <cassert>
#include <iomanip>
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
	assert(w.rd() == 3);
	std::cout << "A wire voltage is determined by the least of deterministic inputs" << std::endl;
	assert(c[2].rd() == 3);
	std::cout << "... and any impeded connections (outputs) are determined by wire.V" << std::endl;
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

	Input in, i2, gate;

	FET nFET(in, gate, true);         // positive signal at gate allows current through device
	FET pFET(i2, gate, false);        // negative signal at gate allows current through device

	assert(nFET.rd().rd() == in.Vss);
	assert(pFET.rd().rd() == i2.Vss);
	std::cout << "Similar to a voltage controlled switch, a FET requires an input voltage" << std::endl;

	in.set_value(in.Vdd);
	i2.set_value(i2.Vdd);
	assert(nFET.rd().rd() == in.Vss);
	std::cout << "An nFET conducts with a positive gate signal" << std::endl;
	assert(pFET.rd().rd() == i2.Vdd);
	std::cout << " and a pFET conducts with a negative gate signal" << std::endl;

	gate.set_value(gate.Vdd);
	assert(nFET.rd().rd() == in.Vdd);
	assert(pFET.rd().rd() == i2.Vss);
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

int main(int a_argc, char *a_argv[]) {
	prsep(); test_connection();
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
	return 0;
};

#endif
