#include <cassert>
#include <vector>
#include <bitset>
#include "test_clockcycler.h"
#include "../src/devices/simulated_ports.h"

#ifdef TESTING
namespace Tests {

	class Machine : public Device {
		SRAM &sram;

		void register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data) {
			if (debug()) std::cout << "Register::" << name << " <- " << std::bitset<8>(data[Register::DVALUE::NEW]) << std::endl;
			sram.write(r->index(), data[Register::DVALUE::NEW]);
		}

	  public:
		Machine(SRAM &a_sram): Device(), sram(a_sram) {
			sram.init_params(4, 0x80);
			sram.write(SRAM::STATUS, 0);
			sram.write(SRAM::OPTION, 0);
			DeviceEvent<Register>::subscribe<Machine>(this, &Machine::register_changed);
		}
		~Machine() {
			DeviceEvent<Register>::unsubscribe<Machine>(this, &Machine::register_changed);
		}
	};


	void test_port_pin_ra0() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		SinglePortA_Analog ra0(pin, "RA0");
		Comparator cmp0;

		auto &c = ra0.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Tristate &ts1 = dynamic_cast<Tristate &>(*c["Tristate1"]);
		Schmitt &trigger = dynamic_cast<Schmitt &>(*c["Schmitt Trigger"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);

	//	PinWire.debug(true);
		// set it up as input, set pin to ground, read and check zero
		ClockedRegister PORTA(SRAM::PORTA, "PORTA");
		ClockedRegister TRISA(SRAM::TRISA, "TRISA");
		ClockedRegister CMCON(SRAM::CMCON, "CMCON");

	//	PORTA.debug(true);
	//	TRISA.debug(true);

		CMCON.write(sram, 0xff);  // no comparators active

		PORTA.write(sram, PORTA.read(sram) &  (~Flags::PORTA::RA0));     // PortA[RA0] flag should be 0
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA0) == 0);

		TRISA.write(sram, TRISA.read(sram) &  (~Flags::TRISA::TRISA0));  // TrisA[RA0] flag should now be zero for output
		assert(TrisLatch.Qc().signal());
		assert((sram.read(SRAM::TRISA) & Flags::TRISA::TRISA0) == 0);

		// Check for an output signal
		assert(pin.signal() == false);     // we expect Vss on output

		// raise a signal on PORTA and check the pin
		PORTA.write(sram, PORTA.read(sram) | Flags::PORTA::RA0);      // PortA[RA0] should now be Vdd

		assert(ra0.data().signal());

		assert(DataLatch.Q().signal());
		assert(!ts1.gate().signal());  // an inverted gate input
		assert(ts1.signal());
		assert(pin.signal());         // we expect Vdd on output now
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA0));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISA.write(sram, TRISA.read(sram) | Flags::TRISA::TRISA0);  // TrisA[RA0] flag should now be 1 for input
		assert((sram.read(SRAM::TRISA) & Flags::TRISA::TRISA0));

		// Set the pin to Vss, read the register and check that the port reflects the pin value
		pin.set_value(pin.Vss, false);
		PORTA.read(sram);   // read all the pin values

		assert(!pin.signal());
		assert(!pin.impeded());
		assert(!PinWire.signal());
		assert(!trigger.rd().signal()); // expect Schmitt output low
		assert(!ra0.comparator().signal());
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA0) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value

		//	ra0.debug(true);
		//	PinWire.debug(true);

		pin.set_value(pin.Vdd, false);
		PORTA.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert(ra0.comparator().signal());
		assert(trigger.rd().signal()); // expect Schmitt output high
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA0));    // Check SRAM has correct value

		pin.set_value(3.0, false);
		assert(ra0.comparator().rd()==3.0);

		std::cout << "PORTA::RA0: all tests concluded successfully" << std::endl;
	}

	void test_port_pin_ra1() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		SinglePortA_Analog ra1(pin, "RA1");
		Comparator cmp0;

		auto &c = ra1.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Tristate &ts1 = dynamic_cast<Tristate &>(*c["Tristate1"]);
		Schmitt &trigger = dynamic_cast<Schmitt &>(*c["Schmitt Trigger"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);

		// set it up as input, set pin to ground, read and check zero
		ClockedRegister PORTA(SRAM::PORTA, "PORTA");
		ClockedRegister TRISA(SRAM::TRISA, "TRISA");
		ClockedRegister CMCON(SRAM::CMCON, "CMCON");

		CMCON.write(sram, 0xff);  // no comparators active

		PORTA.write(sram, PORTA.read(sram) &  (~Flags::PORTA::RA1));     // PortA[RA0] flag should be 0
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA1) == 0);

		TRISA.write(sram, TRISA.read(sram) &  (~Flags::TRISA::TRISA1));  // TrisA[RA0] flag should now be zero for output
		assert(TrisLatch.Qc().signal());
		assert((sram.read(SRAM::TRISA) & Flags::TRISA::TRISA1) == 0);

		// Check for an output signal
		assert(pin.signal() == false);     // we expect Vss on output

		// raise a signal on PORTA and check the pin
		PORTA.write(sram, PORTA.read(sram) | Flags::PORTA::RA1);      // PortA[RA0] should now be Vdd

		assert(ra1.data().signal());

		assert(DataLatch.Q().signal());
		assert(!ts1.gate().signal());  // an inverted gate input
		assert(ts1.signal());
		assert(pin.signal());         // we expect Vdd on output now
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA1));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISA.write(sram, TRISA.read(sram) | Flags::TRISA::TRISA1);  // TrisA[RA0] flag should now be 1 for input
		assert((sram.read(SRAM::TRISA) & Flags::TRISA::TRISA1));

		// Set the pin to Vss, read the register and check that the port reflects the pin value

		pin.set_value(pin.Vss, false);
		PORTA.read(sram);   // read all the pin values
		assert(!pin.signal());
		assert(!pin.impeded());
		assert(!PinWire.signal());
		assert(!trigger.rd().signal()); // expect Schmitt output high
		assert(!ra1.comparator().signal());
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA1) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value

		pin.set_value(pin.Vdd, false);
		PORTA.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert(trigger.rd().signal()); // expect Schmitt output high
		assert(ra1.comparator().signal());
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA1));    // Check SRAM has correct value

		pin.set_value(3.0, false);
		assert(ra1.comparator().rd()==3.0);

		std::cout << "PORTA::RA1: all tests concluded successfully" << std::endl;
	}

	void test_port_pin_ra2() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		SinglePortA_Analog_RA2 ra2(pin, "RA2");
		Comparator cmp0;

		auto &c = ra2.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Tristate &ts1 = dynamic_cast<Tristate &>(*c["Tristate1"]);
		Schmitt &trigger = dynamic_cast<Schmitt &>(*c["Schmitt Trigger"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);

		// set it up as input, set pin to ground, read and check zero
		ClockedRegister PORTA(SRAM::PORTA, "PORTA");
		ClockedRegister TRISA(SRAM::TRISA, "TRISA");
		ClockedRegister CMCON(SRAM::CMCON, "CMCON");
		ClockedRegister VRCON(SRAM::VRCON, "VRCON");

	//	PORTA.debug(true);
	//	TRISA.debug(true);
	//	ra0.debug(true);

		CMCON.write(sram, 0xff);  // no comparators active

		PORTA.write(sram, PORTA.read(sram) &  (~Flags::PORTA::RA2));     // PortA[RA0] flag should be 0
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA2) == 0);

		TRISA.write(sram, TRISA.read(sram) &  (~Flags::TRISA::TRISA2));  // TrisA[RA0] flag should now be zero for output
		assert(TrisLatch.Qc().signal());
		assert((sram.read(SRAM::TRISA) & Flags::TRISA::TRISA2) == 0);

		// Check for an output signal
		assert(pin.signal() == false);     // we expect Vss on output

		// raise a signal on PORTA and check the pin
		PORTA.write(sram, PORTA.read(sram) | Flags::PORTA::RA2);      // PortA[RA1] should now be Vdd

		assert(ra2.data().signal());

		assert(DataLatch.Q().signal());
		assert(!ts1.gate().signal());  // an inverted gate input
		assert(ts1.signal());
		assert(pin.signal());         // we expect Vdd on output now
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA2));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISA.write(sram, TRISA.read(sram) | Flags::TRISA::TRISA2);  // TrisA[RA0] flag should now be 1 for input
		assert((sram.read(SRAM::TRISA) & Flags::TRISA::TRISA2));

		// Set the pin to Vss, read the register and check that the port reflects the pin value
		pin.set_value(pin.Vss, false);
		PORTA.read(sram);   // read all the pin values
		assert(!pin.signal());
		assert(!pin.impeded());
		assert(!PinWire.signal());
		assert(!trigger.rd().signal()); // expect Schmitt output high
		assert(!ra2.comparator().signal());
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA2) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value

		pin.set_value(pin.Vdd, false);
		PORTA.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert(trigger.rd().signal()); // expect Schmitt output high
		assert(ra2.comparator().signal());
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA2));    // Check SRAM has correct value

		pin.set_value(3.0, false);
		assert(ra2.comparator().rd()==3.0);

		pin.set_value(pin.Vss, false);
		assert(pin.rd() == pin.Vss);
		pin.set_value(pin.Vss, true);

		// test the VREF function
		VRCON.write(sram, Flags::VRCON::VROE);
		assert(pin.rd() == pin.Vss);
		// ((vrcon & 0b111) / 32.0) * Vdd + Vdd/4;
		for (BYTE n = 0; n < 16; ++n) {
			VRCON.write(sram, Flags::VRCON::VROE | Flags::VRCON::VREN | n);
			assert(pin.rd() == pin.Vdd * n / 32 + pin.Vdd / 4);
		}
		for (BYTE n = 0; n < 16; ++n) {
			VRCON.write(sram, Flags::VRCON::VROE | Flags::VRCON::VREN | Flags::VRCON::VRR | n);
			assert(pin.rd() == pin.Vdd * n / 24);
		}
		std::cout << "PORTA::RA2: all tests concluded successfully" << std::endl;

	}


	void test_port_pin_ra3() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		SinglePortA_Analog_RA3 ra3(pin, "RA3");
		Comparator cmp0;

		auto &c = ra3.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Tristate &ts1 = dynamic_cast<Tristate &>(*c["Tristate1"]);
		Schmitt &trigger = dynamic_cast<Schmitt &>(*c["Schmitt Trigger"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);

		// set it up as input, set pin to ground, read and check zero
		ClockedRegister PORTA(SRAM::PORTA, "PORTA");
		ClockedRegister TRISA(SRAM::TRISA, "TRISA");
		ClockedRegister CMCON(SRAM::CMCON, "CMCON");
		ClockedRegister VRCON(SRAM::VRCON, "VRCON");

		CMCON.write(sram, 0);  // no comparators active

		PORTA.write(sram, PORTA.read(sram) &  (~Flags::PORTA::RA3));     // PortA[RA0] flag should be 0
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA3) == 0);

		TRISA.write(sram, TRISA.read(sram) &  (~Flags::TRISA::TRISA3));  // TrisA[RA0] flag should now be zero for output
		assert(TrisLatch.Qc().signal());
		assert((sram.read(SRAM::TRISA) & Flags::TRISA::TRISA3) == 0);

		// Check for an output signal
		assert(pin.signal() == false);     // we expect Vss on output

		// raise a signal on PORTA and check the pin
		PORTA.write(sram, PORTA.read(sram) | Flags::PORTA::RA3);      // PortA[RA1] should now be Vdd

		assert(ra3.data().signal());

		assert(DataLatch.Q().signal());
		assert(!ts1.gate().signal());  // an inverted gate input
		assert(ts1.signal());
		assert(pin.signal());         // we expect Vdd on output now
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA3));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISA.write(sram, TRISA.read(sram) | Flags::TRISA::TRISA3);  // TrisA[RA0] flag should now be 1 for input
		assert((sram.read(SRAM::TRISA) & Flags::TRISA::TRISA3));

		// Set the pin to Vss, read the register and check that the port reflects the pin value
		pin.set_value(pin.Vss, false);
		PORTA.read(sram);   // read all the pin values
		assert(!pin.signal());
		assert(!pin.impeded());
		assert(!PinWire.signal());
		assert(!trigger.rd().signal()); // expect Schmitt output high
		assert(!ra3.comparator().signal());
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA3) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value

		pin.set_value(pin.Vdd, false);
		PORTA.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert(trigger.rd().signal()); // expect Schmitt output high
		assert(ra3.comparator().signal());
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA3));    // Check SRAM has correct value

		pin.set_value(3.0, false);
		assert(ra3.comparator().rd()==3.0);

		pin.set_value(pin.Vss, false);
		assert(pin.rd() == pin.Vss);
		pin.set_value(pin.Vss, true);

		std::cout << "PORTA::RA3: all tests concluded successfully" << std::endl;

	}

	void test_port_pin_ra4() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		SinglePortA_Analog_RA4 ra4(pin, "RA4");
		Comparator cmp0;

		Connection external(Connection::Vdd, "EXT");
		external.impeded(false);
		external.R(100000);
		auto &c = ra4.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		OrGate &nor1 = dynamic_cast<OrGate &>(*c["NOR Gate"]);
		Schmitt &trigger = dynamic_cast<Schmitt &>(*c["Schmitt Trigger"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
		Mux &mux1 = dynamic_cast<Mux &>(*c["Mux"]);
		FET &fet1 = dynamic_cast<FET &>(*c["FET1"]);

		// set it up as input, set pin to ground, read and check zero
		ClockedRegister PORTA(SRAM::PORTA, "PORTA");
		ClockedRegister TRISA(SRAM::TRISA, "TRISA");
		ClockedRegister CMCON(SRAM::CMCON, "CMCON");
		ClockedRegister VRCON(SRAM::VRCON, "VRCON");

	//	PinWire.debug(true);
	//	fet1.debug(true);
	//	pin.debug(true);
	//	nor1.debug(true);
	//	mux1.debug(true);

		pin.connect(external);

		CMCON.write(sram, 0);  // no comparators active

		PORTA.write(sram, PORTA.read(sram) &  (~Flags::PORTA::RA4));     // PortA[RA0] flag should be 0

		TRISA.write(sram, TRISA.read(sram) &  (~Flags::TRISA::TRISA4));  // TrisA[RA0] flag should now be zero for output

		assert(DataLatch.Qc().signal());             // outputting a zero
		assert(TrisLatch.Qc().signal());

		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA4) == 0);
		assert((sram.read(SRAM::TRISA) & Flags::TRISA::TRISA4) == 0);

		// Check MUX Output low
		assert(not mux1.rd().signal());

		// Check NOR gate output high
		assert(nor1.rd().signal());
		assert(fet1.gate().signal());
		assert(not PinWire.signal());     // PinWire is drained

		// Check for an output signal
		assert(not pin.signal());     // we expect Vss on output

		// raise a signal on PORTA and check the pin
		PORTA.write(sram, PORTA.read(sram) | Flags::PORTA::RA4);      // PortA[RA1] should now be Vdd

		assert(ra4.data().signal());

		assert(DataLatch.Q().signal());
		assert(not nor1.rd().signal());

		assert(not fet1.gate().signal());
		assert(PinWire.signal());     // PinWire is positive
		assert(pin.signal());         // we expect Vdd on output now

		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA4));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISA.write(sram, TRISA.read(sram) | Flags::TRISA::TRISA4);  // TrisA[RA0] flag should now be 1 for input
		assert((sram.read(SRAM::TRISA) & Flags::TRISA::TRISA4));

	// Set the pin to Vss, read the register and check that the port reflects the pin value

		external.set_value(pin.Vss, false);
		PORTA.read(sram);   // read all the pin values
		assert(not pin.signal());
		assert(not pin.impeded());
		assert(not PinWire.signal());
		assert(not trigger.rd().signal()); // expect Schmitt output high
	//	assert(!ra4.comparator().signal());
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA4) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value
		external.set_value(pin.Vdd, false);
		PORTA.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert(trigger.rd().signal()); // expect Schmitt output high

	//	assert(ra4.().signal());
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA4));    // Check SRAM has correct value

		external.set_value(pin.Vss, false);
		assert(pin.rd() == pin.Vss);

		std::cout << "PORTA::RA4: all tests concluded successfully" << std::endl;

	}

	//___________________________________________________________________________________
	class CONFIG: public Register {
	  public:
		CONFIG(const std::string &a_name): Register(0, a_name) {}
		virtual const BYTE read(SRAM &a_sram) { return get_value(); }
		virtual void write(SRAM &a_sram, const unsigned char value)	{ set_value(value, get_value()); }
	};


	void test_port_pin_ra5() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		ClockCycler clock;
		SinglePortA_MCLR_RA5 ra5(pin, "RA5");

		CONFIG cfg1("CONFIG1");  // These are "pretend" registers, but we use this mechanism
		CONFIG cfg2("CONFIG2");  // to communicate CPU configuration to machine parts.

		auto &c = ra5.components();

	//	Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
	//
		Schmitt &trigger1 = dynamic_cast<Schmitt &>(*c["Schmitt1"]);
		Schmitt &trigger2 = dynamic_cast<Schmitt &>(*c["Schmitt2"]);
	//	Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
		Wire &MCLREWire = dynamic_cast<Wire &>(*c["MCLRE Wire"]);
		AndGate &nand1 = dynamic_cast<AndGate &>(*c["And1"]);

		// Port RA5 is controlled by the MCLRE configuration bit.  If this bit is not set, it
		// enables input on RA5 pin.  However, the TRIS register has no impact, and the
		// port cannot be used as output (TRISA but 5 always reads 1).
		// When the MCLRE configuration bit is set, it enables the MCLR circuit and acts as
		// an active low reset.
		// This pin is also used as a programming voltage input, and enters programming mode
		// when RA voltage exceeds Vdd.

		ClockedRegister PORTA(SRAM::PORTA, "PORTA");
	//	MCLREWire.debug(true);
		cfg1.write(sram, Flags::CONFIG::MCLRE); // set the MCLRE bit
		clock.cycle();
		pin.set_value(pin.Vdd, false);
		PORTA.read(sram);   // read all the pin values

	// With MCLRE high, we shut off trigger2, so we should see no PORTA input
		assert(pin.signal());
		assert(not trigger2.rd().signal());
		assert(MCLREWire.signal());
		assert(not (sram.read(SRAM::PORTA) & Flags::PORTA::RA5));
	// trigger1 should show the inverse of what is on the pin
		assert(not trigger1.rd().signal());

	// Active low on MCLR input from RA5::Pin
		assert(nand1.rd().signal()); // not(and(MCLRE, not(PinValue)))
		pin.set_value(pin.Vss, false);
		assert(not ra5.mclr().signal()); // not(and(MCLRE, not(PinValue)))

		pin.set_value(pin.Vdd, false);
		cfg1.write(sram, 0); // clear the MCLRE bit
		clock.cycle();
		ra5.debug(true);
		PORTA.read(sram);   // read all the pin values

	// With MCLRE low, we enable trigger2, and we read RA5::Pin into PORTA
		assert(not MCLREWire.signal());
		assert(pin.signal());
		assert(trigger2.rd().signal());
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA5));
	// trigger1 should show the inverse of what is on the pin
		assert(not trigger1.rd().signal());
	// Active low on MCLR input from RA5::Pin, but port configured as input
		assert(ra5.mclr().signal());   // not(and(MCLRE, not(PinValue)))

		pin.set_value(pin.Vss, false);
		PORTA.read(sram);   // read all the pin values

	// RA5 configured as input should respond to RA5::Pin changes
		assert(not pin.signal());
		assert(not trigger2.rd().signal());
		assert(not (sram.read(SRAM::PORTA) & Flags::PORTA::RA5));
	// trigger1 should show the inverse of what is on the pin
		assert(trigger1.rd().signal());

	// If the pin voltage is raised above Vdd, we set the programming mode
		assert(not ra5.pgm().signal());
		pin.set_value(pin.Vdd, false);
		assert(not ra5.pgm().signal());
		pin.set_value(pin.Vdd*2, false);
		assert(ra5.pgm().signal());

		std::cout << "PORTA::RA5: all tests concluded successfully" << std::endl;
	}


	void test_port_pin_ra6() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		SinglePortA_RA6_CLKOUT ra6(pin, "RA6");
		ClockCycler clock;
		CONFIG cfg1("CONFIG1");  // These are "pretend" registers, but we use this mechanism
		CONFIG cfg2("CONFIG2");  // to communicate CPU configuration to machine parts.

		auto &c = ra6.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		OrGate &nor1 = dynamic_cast<OrGate &>(*c["Nor1"]);
		Schmitt &trigger = dynamic_cast<Schmitt &>(*c["Schmitt Trigger"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
		Mux &mux1 = dynamic_cast<Mux &>(*c["Mux"]);
		Tristate &ts1 = dynamic_cast<Tristate&>(*c["Tristate1"]);

		// for ra6 to be used for i/o, we need to set FOSC to one of 011, 100 or 110
		cfg1.write(sram, Flags::CONFIG::FOSC0 | Flags::CONFIG::FOSC1);  // mode 011
		clock.cycle();
		// set it up as input, set pin to ground, read and check zero
		ClockedRegister PORTA(SRAM::PORTA, "PORTA");
		ClockedRegister TRISA(SRAM::TRISA, "TRISA");

		PORTA.write(sram, PORTA.read(sram) &  (~Flags::PORTA::RA6));     // PortA[RA0] flag should be 0
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA6) == 0);
		assert(mux1.rd().signal() == DataLatch.Q().signal());

		TRISA.write(sram, TRISA.read(sram) &  (~Flags::TRISA::TRISA6));  // TrisA[RA0] flag should now be zero for output
		assert(TrisLatch.Qc().signal());
		assert(mux1.rd().signal() == DataLatch.Q().signal());
		assert((sram.read(SRAM::TRISA) & Flags::TRISA::TRISA6) == 0);

		// Check for an output signal
		assert(pin.signal() == false);     // we expect Vss on output

		// raise a signal on PORTA and check the pin
		PORTA.write(sram, PORTA.read(sram) | Flags::PORTA::RA6);      // PortA[RA1] should now be Vdd

		assert(ra6.data().signal());
		assert(not ra6.fosc1().signal());
		assert(ra6.fosc2().signal());
		assert(DataLatch.Q().signal());
		assert(mux1.rd().signal() == DataLatch.Q().signal());
		assert(not nor1.rd().signal());
		assert(ts1.rd().signal());
		assert(pin.signal());         // we expect Vdd on output now
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA6));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISA.write(sram, TRISA.read(sram) | Flags::TRISA::TRISA6);  // TrisA[RA0] flag should now be 1 for input
		assert((sram.read(SRAM::TRISA) & Flags::TRISA::TRISA6));

		// Set the pin to Vss, read the register and check that the port reflects the pin value
		pin.set_value(pin.Vss, false);
		PORTA.read(sram);   // read all the pin values
		assert(!pin.signal());
		assert(!pin.impeded());
		assert(!PinWire.signal());
		assert(!trigger.rd().signal()); // expect Schmitt output low
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA6) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value

		pin.set_value(pin.Vdd, false);
		PORTA.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert(trigger.rd().signal()); // expect Schmitt output high
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA6));    // Check SRAM has correct value

	// I/O tests done

		cfg1.write(sram, Flags::CONFIG::FOSC0 | Flags::CONFIG::FOSC1 | Flags::CONFIG::FOSC2);  // mode 111
		clock.cycle();
		TRISA.write(sram, TRISA.read(sram) &  (~Flags::TRISA::TRISA6));  // TrisA[RA0] flag should now be zero for output

	// test clock out
		assert(!pin.signal());
		clock.q();
		clock.q();
		assert(pin.signal());
		clock.q();
		clock.q();
		assert(!pin.signal());

	// Can't really test external oscillator yet
		std::cout << "PORTA::RA6: all tests concluded successfully" << std::endl;

	}


	void test_port_pin_ra7() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		PortA_RA7 ra7(pin, "RA7");
		ClockCycler clock;
		CONFIG cfg1("CONFIG1");  // These are "pretend" registers, but we use this mechanism
		CONFIG cfg2("CONFIG2");  // to communicate CPU configuration to machine parts.

		auto &c = ra7.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Schmitt &trigger = dynamic_cast<Schmitt &>(*c["Schmitt Trigger"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
		Tristate &ts1 = dynamic_cast<Tristate&>(*c["Tristate1"]);

		// for ra7 to be used for i/o, we need to set FOSC to either 100 or 101
		cfg1.write(sram, Flags::CONFIG::FOSC2 | Flags::CONFIG::FOSC0);  // mode 101
		clock.cycle();
		assert(ra7.Fosc().signal());

		// set it up as input, set pin to ground, read and check zero
		ClockedRegister PORTA(SRAM::PORTA, "PORTA");
		ClockedRegister TRISA(SRAM::TRISA, "TRISA");

		PORTA.write(sram, PORTA.read(sram) &  (~Flags::PORTA::RA7));     // PortA[RA0] flag should be 0
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA7) == 0);

		TRISA.write(sram, TRISA.read(sram) &  (~Flags::TRISA::TRISA7));  // TrisA[RA0] flag should now be zero for output
		assert(TrisLatch.Qc().signal());
		assert((sram.read(SRAM::TRISA) & Flags::TRISA::TRISA7) == 0);

		// Check for an output signal
		assert(pin.signal() == false);     // we expect Vss on output

		// raise a signal on PORTA and check the pin
		PORTA.write(sram, PORTA.read(sram) | Flags::PORTA::RA7);      // PortA[RA1] should now be Vdd

		assert(ra7.data().signal());
		assert(DataLatch.Q().signal());
		assert(ts1.rd().signal());
		assert(pin.signal());         // we expect Vdd on output now
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA7));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISA.write(sram, TRISA.read(sram) | Flags::TRISA::TRISA7);  // TrisA[RA0] flag should now be 1 for input
		assert((sram.read(SRAM::TRISA) & Flags::TRISA::TRISA7));

		// Set the pin to Vss, read the register and check that the port reflects the pin value
		pin.set_value(pin.Vss, false);
		PORTA.read(sram);   // read all the pin values
		assert(!pin.signal());
		assert(!pin.impeded());
		assert(!PinWire.signal());
		assert(!trigger.rd().signal()); // expect Schmitt output low
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA7) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value

		pin.set_value(pin.Vdd, false);
		PORTA.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert(trigger.rd().signal()); // expect Schmitt output high
		assert((sram.read(SRAM::PORTA) & Flags::PORTA::RA7));    // Check SRAM has correct value

	// I/O tests done

		std::cout << "PORTA::RA7: all tests concluded successfully" << std::endl;

	}


	void test_port_pin_rb0() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		PortB_RB0 rb0(pin, "RB0");

		auto &c = rb0.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Schmitt &trigger = dynamic_cast<Schmitt &>(*c["INT_TRIGGER"]);  // external interrupt
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
		Tristate &ts1 = dynamic_cast<Tristate&>(*c["Tristate1"]);
		Latch &SR1 = dynamic_cast<Latch &>(*c["SR1"]);


		ClockedRegister PORTB(SRAM::PORTB, "PORTB");
		ClockedRegister TRISB(SRAM::TRISB, "TRISB");
		ClockedRegister OPTION(SRAM::OPTION, "OPTION");

		OPTION.write(sram, sram.read(SRAM::OPTION) | Flags::OPTION::RBPU);  //set pull-up resistor option

		PORTB.write(sram, PORTB.read(sram) & (~Flags::PORTB::RB0));        // PortB[RB0] flag should be 0
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB0) == 0);

		TRISB.write(sram, TRISB.read(sram) &  (~Flags::TRISB::TRISB0));    // TrisA[RB0] flag should now be zero for output
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB0) == 0);

		assert(TrisLatch.Qc().signal());

		// Check for an output signal
		assert(not pin.signal());     // we expect Vss on output

		// raise a signal on PORTB and check the pin
		PORTB.write(sram, PORTB.read(sram) | Flags::PORTB::RB0);           // PortB[RB0] should now be Vdd

		assert(DataLatch.Q().signal());
		assert(ts1.rd().signal());
		assert(pin.signal());         // we expect Vdd on output now
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB0));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISB.write(sram, TRISB.read(sram) | Flags::TRISB::TRISB0);  // TrisA[RB0] flag should now be 1 for input
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB0));

		pin.set_value(pin.Vss, true);                                 // assume pin has high resistance
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB0));         // pull-up active
		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB0) == 0);    // Terminal pin overrides Pull-Up
		pin.set_value(pin.Vss, false);                                // assume pin is an input
		OPTION.write(sram, sram.read(SRAM::OPTION) & ~Flags::OPTION::RBPU);  //set pull-up resistor option off
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB0) == 0);  // no pull-up

		// Set the pin to Vss, read the register and check that the port reflects the pin value
		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert(!pin.signal());
		assert(!pin.impeded());
		assert(!PinWire.signal());
		assert(!trigger.rd().signal()); // expect Schmitt output low
		assert(SR1.Qc().signal());
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB0) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value

		pin.set_value(pin.Vdd, false);
		PORTB.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert(trigger.rd().signal()); // expect Schmitt output high
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB0));    // Check SRAM has correct value

	// I/O tests done
		std::cout << "PORTB::RB0: all tests concluded successfully" << std::endl;

	}

	void test_port_pin_rb1() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		PortB_RB1 rb1(pin, "RB1");

		auto &c = rb1.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Schmitt &trigger = dynamic_cast<Schmitt &>(*c["USART_TRIGGER"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
		Tristate &ts1 = dynamic_cast<Tristate&>(*c["Tristate1"]);
		Latch &SR1 = dynamic_cast<Latch &>(*c["SR1"]);

		ClockedRegister PORTB(SRAM::PORTB, "PORTB");
		ClockedRegister TRISB(SRAM::TRISB, "TRISB");
		ClockedRegister OPTION(SRAM::OPTION, "OPTION");

		OPTION.write(sram, sram.read(SRAM::OPTION) | Flags::OPTION::RBPU);  //set pull-up resistor option

		PORTB.write(sram, PORTB.read(sram) & (~Flags::PORTB::RB1));        // PortB[RB1] flag should be 0
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB1) == 0);

		TRISB.write(sram, TRISB.read(sram) &  (~Flags::TRISB::TRISB1));    // TrisA[RB1] flag should now be zero for output
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB1) == 0);

		assert(TrisLatch.Qc().signal());

		// Check for an output signal
		assert(not pin.signal());     // we expect Vss on output

		// raise a signal on PORTB and check the pin
		PORTB.write(sram, PORTB.read(sram) | Flags::PORTB::RB1);           // PortB[RB1] should now be Vdd

		assert(rb1.data().signal());
		assert(DataLatch.Q().signal());
		assert(ts1.rd().signal());
		assert(pin.signal());         // we expect Vdd on output now
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB1));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISB.write(sram, TRISB.read(sram) | Flags::TRISB::TRISB1);  // TrisA[RB1] flag should now be 1 for input
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB1));

		pin.set_value(pin.Vss, true);                                 // assume pin has high resistance
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB1));         // pull-up active
		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB1) == 0);    // Terminal pin overrides Pull-Up
		pin.set_value(pin.Vss, false);                                // assume pin is an input
		OPTION.write(sram, sram.read(SRAM::OPTION) & ~Flags::OPTION::RBPU);  //set pull-up resistor option off
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB1) == 0);  // no pull-up

		// Set the pin to Vss, read the register and check that the port reflects the pin value
		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert(!pin.signal());
		assert(!pin.impeded());
		assert(!PinWire.signal());
		assert(!trigger.rd().signal()); // expect Schmitt output low
		assert(SR1.Qc().signal());
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB1) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value

		pin.set_value(pin.Vdd, false);
		PORTB.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert(trigger.rd().signal()); // expect Schmitt output high
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB1));    // Check SRAM has correct value

	// I/O tests done
		std::cout << "PORTB::RB1: all tests concluded successfully" << std::endl;

	}

	void test_port_pin_rb2() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		PortB_RB2 rb2(pin, "RB2");

		auto &c = rb2.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Schmitt &trigger = dynamic_cast<Schmitt &>(*c["USART_TRIGGER"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
		Tristate &ts1 = dynamic_cast<Tristate&>(*c["Tristate1"]);
		Latch &SR1 = dynamic_cast<Latch &>(*c["SR1"]);

		ClockedRegister PORTB(SRAM::PORTB, "PORTB");
		ClockedRegister TRISB(SRAM::TRISB, "TRISB");
		ClockedRegister OPTION(SRAM::OPTION, "OPTION");

		OPTION.write(sram, sram.read(SRAM::OPTION) | Flags::OPTION::RBPU);  //set pull-up resistor option

		PORTB.write(sram, PORTB.read(sram) & (~Flags::PORTB::RB2));        // PortB[RB2] flag should be 0
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB2) == 0);

		TRISB.write(sram, TRISB.read(sram) &  (~Flags::TRISB::TRISB2));    // TrisA[RB2] flag should now be zero for output
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB2) == 0);

		assert(TrisLatch.Qc().signal());

		// Check for an output signal
		assert(not pin.signal());     // we expect Vss on output

		// raise a signal on PORTB and check the pin
		PORTB.write(sram, PORTB.read(sram) | Flags::PORTB::RB2);           // PortB[RB2] should now be Vdd

		assert(rb2.data().signal());
		assert(DataLatch.Q().signal());
		assert(ts1.rd().signal());
		assert(pin.signal());         // we expect Vdd on output now
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB2));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISB.write(sram, TRISB.read(sram) | Flags::TRISB::TRISB2);  // TrisA[RB2] flag should now be 1 for input
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB2));

		pin.set_value(pin.Vss, true);                                 // assume pin has high resistance
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB2));         // pull-up active
		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB2) == 0);    // Terminal pin overrides Pull-Up
		pin.set_value(pin.Vss, false);                                // assume pin is an input
		OPTION.write(sram, sram.read(SRAM::OPTION) & ~Flags::OPTION::RBPU);  //set pull-up resistor option off
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB2) == 0);  // no pull-up

		// Set the pin to Vss, read the register and check that the port reflects the pin value
		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert(!pin.signal());
		assert(!pin.impeded());
		assert(!PinWire.signal());
		assert(!trigger.rd().signal()); // expect Schmitt output low
		assert(SR1.Qc().signal());
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB2) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value

		pin.set_value(pin.Vdd, false);
		PORTB.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert(trigger.rd().signal()); // expect Schmitt output high
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB2));    // Check SRAM has correct value

	// I/O tests done
		std::cout << "PORTB::RB2: all tests concluded successfully" << std::endl;

	}

	void test_port_pin_rb3() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		PortB_RB3 rb3(pin, "RB3");

		auto &c = rb3.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Schmitt &trigger = dynamic_cast<Schmitt &>(*c["TRIGGER"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
		Tristate &ts1 = dynamic_cast<Tristate&>(*c["Tristate1"]);
		Latch &SR1 = dynamic_cast<Latch &>(*c["SR1"]);
	//	FET &FET1 = dynamic_cast<FET &>(*c["RBPU_FET"]);


		ClockedRegister PORTB(SRAM::PORTB, "PORTB");
		ClockedRegister TRISB(SRAM::TRISB, "TRISB");
		ClockedRegister OPTION(SRAM::OPTION, "OPTION");
	//	trigger.debug(true);
	//	FET1.rd().debug(true);

		OPTION.write(sram, sram.read(SRAM::OPTION) | Flags::OPTION::RBPU);  //set pull-up resistor option

		PORTB.write(sram, PORTB.read(sram) & (~Flags::PORTB::RB3));        // PortB[RB3] flag should be 0
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB3) == 0);

		TRISB.write(sram, TRISB.read(sram) &  (~Flags::TRISB::TRISB3));    // TrisA[RB3] flag should now be zero for output
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB3) == 0);

		assert(TrisLatch.Qc().signal());

		// Check for an output signal
		assert(not pin.signal());     // we expect Vss on output

		// raise a signal on PORTB and check the pin
		PORTB.write(sram, PORTB.read(sram) | Flags::PORTB::RB3);           // PortB[RB3] should now be Vdd

		assert(rb3.data().signal());
		assert(DataLatch.Q().signal());
		assert(ts1.rd().signal());
		assert(pin.signal());         // we expect Vdd on output now
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB3));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISB.write(sram, TRISB.read(sram) | Flags::TRISB::TRISB3);  // TrisA[RB3] flag should now be 1 for input
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB3));

		pin.set_value(pin.Vss, true);                                 // assume pin has high resistance
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB3));         // pull-up active
		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB3) == 0);    // Terminal pin overrides Pull-Up
		pin.set_value(pin.Vss, false);                                // assume pin is an input
		OPTION.write(sram, sram.read(SRAM::OPTION) & ~Flags::OPTION::RBPU);  //set pull-up resistor option off
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB3) == 0);  // no pull-up

		// Set the pin to Vss, read the register and check that the port reflects the pin value
		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert(!pin.signal());
		assert(!pin.impeded());
		assert(!PinWire.signal());
		assert(!trigger.rd().signal()); // expect Schmitt output low
		assert(SR1.Qc().signal());
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB3) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value

		pin.set_value(pin.Vdd, false);
		PORTB.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert(trigger.rd().signal()); // expect Schmitt output high
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB3));    // Check SRAM has correct value

	// I/O tests done
		std::cout << "PORTB::RB3: all tests concluded successfully" << std::endl;

	}


	void test_port_pin_rb4() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		PortB_RB4 rb4(pin, "RB4");

		auto &c = rb4.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Schmitt &trigger = dynamic_cast<Schmitt &>(*c["TRIGGER"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
		Tristate &ts1 = dynamic_cast<Tristate&>(*c["Tristate1"]);
		Latch &SR1 = dynamic_cast<Latch &>(*c["SR1"]);
	//	FET &FET1 = dynamic_cast<FET &>(*c["RBPU_FET"]);


		ClockedRegister PORTB(SRAM::PORTB, "PORTB");
		ClockedRegister TRISB(SRAM::TRISB, "TRISB");
		ClockedRegister OPTION(SRAM::OPTION, "OPTION");
	//	trigger.debug(true);
	//	FET1.rd().debug(true);

		OPTION.write(sram, sram.read(SRAM::OPTION) | Flags::OPTION::RBPU);  //set pull-up resistor option

		PORTB.write(sram, PORTB.read(sram) & (~Flags::PORTB::RB4));        // PortB[RB4] flag should be 0
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB4) == 0);

		TRISB.write(sram, TRISB.read(sram) &  (~Flags::TRISB::TRISB4));    // TrisA[RB4] flag should now be zero for output
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB4) == 0);

		assert(TrisLatch.Qc().signal());

		// Check for an output signal
		assert(not pin.signal());     // we expect Vss on output

		// raise a signal on PORTB and check the pin
		PORTB.write(sram, PORTB.read(sram) | Flags::PORTB::RB4);           // PortB[RB4] should now be Vdd

		assert(rb4.data().signal());
		assert(DataLatch.Q().signal());
		assert(ts1.rd().signal());
		assert(pin.signal());         // we expect Vdd on output now
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB4));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISB.write(sram, TRISB.read(sram) | Flags::TRISB::TRISB4);  // TrisA[RB4] flag should now be 1 for input
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB4));

		pin.set_value(pin.Vss, true);                                 // assume pin has high resistance
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB4));         // pull-up active


	//	rb4.debug(true);
	//	PinWire.debug(true);
		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB4) == 0);    // Terminal pin overrides Pull-Up
		pin.set_value(pin.Vss, false);                                // assume pin is an input
		OPTION.write(sram, sram.read(SRAM::OPTION) & ~Flags::OPTION::RBPU);  //set pull-up resistor option off
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB4) == 0);  // no pull-up

		// Set the pin to Vss, read the register and check that the port reflects the pin value
		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert(!pin.signal());
		assert(!pin.impeded());
		assert(!PinWire.signal());
		assert(!trigger.rd().signal()); // expect Schmitt output low
		assert(SR1.Qc().signal());
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB4) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value

		pin.set_value(pin.Vdd, false);
		PORTB.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert(trigger.rd().signal()); // expect Schmitt output high
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB4));    // Check SRAM has correct value

	// I/O tests done
		std::cout << "PORTB::RB4: all tests concluded successfully" << std::endl;

	}


	void test_port_pin_rb5() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		PortB_RB5 rb5(pin, "RB5");

		auto &c = rb5.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
		Tristate &ts1 = dynamic_cast<Tristate&>(*c["Tristate1"]);
		Latch &SR1 = dynamic_cast<Latch &>(*c["SR1"]);

		ClockedRegister PORTB(SRAM::PORTB, "PORTB");
		ClockedRegister TRISB(SRAM::TRISB, "TRISB");
		ClockedRegister OPTION(SRAM::OPTION, "OPTION");

		OPTION.write(sram, sram.read(SRAM::OPTION) | Flags::OPTION::RBPU);  //set pull-up resistor option

		PORTB.write(sram, PORTB.read(sram) & (~Flags::PORTB::RB5));        // PortB[RB5] flag should be 0
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB5) == 0);

		TRISB.write(sram, TRISB.read(sram) &  (~Flags::TRISB::TRISB5));    // TrisA[RB5] flag should now be zero for output
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB5) == 0);

		assert(TrisLatch.Qc().signal());

		// Check for an output signal
		assert(not pin.signal());     // we expect Vss on output

		// raise a signal on PORTB and check the pin
		PORTB.write(sram, PORTB.read(sram) | Flags::PORTB::RB5);           // PortB[RB5] should now be Vdd

		assert(rb5.data().signal());
		assert(DataLatch.Q().signal());
		assert(ts1.rd().signal());
		assert(pin.signal());         // we expect Vdd on output now
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB5));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISB.write(sram, TRISB.read(sram) | Flags::TRISB::TRISB5);  // TrisA[RB5] flag should now be 1 for input
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB5));

		pin.set_value(pin.Vss, true);                                 // assume pin has high resistance
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB5));         // pull-up active


		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB5) == 0);    // Terminal pin overrides Pull-Up
		pin.set_value(pin.Vss, false);                                // assume pin is an input
		OPTION.write(sram, sram.read(SRAM::OPTION) & ~Flags::OPTION::RBPU);  //set pull-up resistor option off
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB5) == 0);  // no pull-up

		// Set the pin to Vss, read the register and check that the port reflects the pin value
		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert(!pin.signal());
		assert(!pin.impeded());
		assert(!PinWire.signal());
		assert(SR1.Qc().signal());
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB5) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value

		pin.set_value(pin.Vdd, false);
		PORTB.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB5));    // Check SRAM has correct value

	// I/O tests done
		std::cout << "PORTB::RB5: all tests concluded successfully" << std::endl;

	}

	void test_port_pin_rb6() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		PortB_RB6 rb6(pin, "RB6");

		auto &c = rb6.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
		Tristate &ts1 = dynamic_cast<Tristate&>(*c["Tristate1"]);
		Latch &SR1 = dynamic_cast<Latch &>(*c["SR1"]);
		Schmitt &trigger = dynamic_cast<Schmitt &>(*c["TRIGGER"]);

		ClockedRegister PORTB(SRAM::PORTB, "PORTB");
		ClockedRegister TRISB(SRAM::TRISB, "TRISB");
		ClockedRegister OPTION(SRAM::OPTION, "OPTION");

		OPTION.write(sram, sram.read(SRAM::OPTION) | Flags::OPTION::RBPU);  //set pull-up resistor option

		PORTB.write(sram, PORTB.read(sram) & (~Flags::PORTB::RB6));        // PortB[RB6] flag should be 0
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB6) == 0);

		TRISB.write(sram, TRISB.read(sram) &  (~Flags::TRISB::TRISB6));    // TrisA[RB6] flag should now be zero for output
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB6) == 0);

		assert(TrisLatch.Qc().signal());

		// Check for an output signal
		assert(not pin.signal());     // we expect Vss on output

		// raise a signal on PORTB and check the pin
		PORTB.write(sram, PORTB.read(sram) | Flags::PORTB::RB6);           // PortB[RB6] should now be Vdd

		assert(rb6.data().signal());
		assert(DataLatch.Q().signal());
		assert(ts1.rd().signal());
		assert(pin.signal());         // we expect Vdd on output now
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB6));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISB.write(sram, TRISB.read(sram) | Flags::TRISB::TRISB6);  // TrisA[RB6] flag should now be 1 for input
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB6));

		pin.set_value(pin.Vss, true);                                 // assume pin has high resistance
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB6));         // pull-up active


		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB6) == 0);    // Terminal pin overrides Pull-Up
		pin.set_value(pin.Vss, false);                                // assume pin is an input
		OPTION.write(sram, sram.read(SRAM::OPTION) & ~Flags::OPTION::RBPU);  //set pull-up resistor option off
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB6) == 0);  // no pull-up

		// Set the pin to Vss, read the register and check that the port reflects the pin value
		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert(!pin.signal());
		assert(!pin.impeded());
		assert(!PinWire.signal());
		assert(!trigger.rd().signal()); // expect Schmitt output low
		assert(SR1.Qc().signal());
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB6) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value

		pin.set_value(pin.Vdd, false);
		PORTB.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert(trigger.rd().signal()); // expect Schmitt output high
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB6));    // Check SRAM has correct value

	// I/O tests done
		std::cout << "PORTB::RB6: all tests concluded successfully" << std::endl;

	}

	void test_port_pin_rb7() {
		SRAM sram;
		Machine machine(sram);
		Terminal pin;
		PortB_RB7 rb7(pin, "RB7");

		auto &c = rb7.components();

		Latch &DataLatch = dynamic_cast<Latch &>(*c["Data Latch"]);
		Latch &TrisLatch = dynamic_cast<Latch &>(*c["Tris Latch"]);
		Wire &PinWire = dynamic_cast<Wire &>(*c["Pin Wire"]);
		Tristate &ts1 = dynamic_cast<Tristate&>(*c["Tristate1"]);
		Latch &SR1 = dynamic_cast<Latch &>(*c["SR1"]);
		OrGate &Out_en = dynamic_cast<OrGate &>(*c["OR(TrisLatch.Q, T1OSCEN)"]);

		ClockedRegister PORTB(SRAM::PORTB, "PORTB");
		ClockedRegister TRISB(SRAM::TRISB, "TRISB");
		ClockedRegister OPTION(SRAM::OPTION, "OPTION");

		OPTION.write(sram, sram.read(SRAM::OPTION) | Flags::OPTION::RBPU);  //set pull-up resistor option

		PORTB.write(sram, PORTB.read(sram) & (~Flags::PORTB::RB7));        // PortB[RB7] flag should be 0
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB7) == 0);

		TRISB.write(sram, TRISB.read(sram) &  (~Flags::TRISB::TRISB7));    // TrisA[RB7] flag should now be zero for output
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB7) == 0);

		assert(TrisLatch.Qc().signal());

		// Check for an output signal
		assert(not pin.signal());     // we expect Vss on output

		// raise a signal on PORTB and check the pin
		PORTB.write(sram, PORTB.read(sram) | Flags::PORTB::RB7);           // PortB[RB7] should now be Vdd

		assert(rb7.data().signal());
		assert(DataLatch.Q().signal());
		assert(not Out_en.rd().signal());
		assert(ts1.rd().signal());
		assert(pin.signal());         // we expect Vdd on output now
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB7));    // Check SRAM has correct value

		// Now we set the port up as an input, by writing 1 into the TRIS register
		TRISB.write(sram, TRISB.read(sram) | Flags::TRISB::TRISB7);  // TrisA[RB7] flag should now be 1 for input
		assert((sram.read(SRAM::TRISB) & Flags::TRISB::TRISB7));

		pin.set_value(pin.Vss, true);                                 // assume pin has high resistance
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB7));         // pull-up active


		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB7) == 0);    // Terminal pin overrides Pull-Up
		pin.set_value(pin.Vss, false);                                // assume pin is an input
		OPTION.write(sram, sram.read(SRAM::OPTION) & ~Flags::OPTION::RBPU);  //set pull-up resistor option off
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB7) == 0);  // no pull-up

		// Set the pin to Vss, read the register and check that the port reflects the pin value
		pin.set_value(pin.Vss, false);
		PORTB.read(sram);   // read all the pin values
		assert(!pin.signal());
		assert(!pin.impeded());
		assert(!PinWire.signal());
		assert(SR1.Qc().signal());
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB7) == 0);

		// Set the pin to Vdd, read the register and check that the port reflects the pin value

		pin.set_value(pin.Vdd, false);
		PORTB.read(sram);   // read all the pin values

		assert(pin.signal());
		assert(!pin.impeded());
		assert(PinWire.signal());
		assert((sram.read(SRAM::PORTB) & Flags::PORTB::RB7));    // Check SRAM has correct value

	// I/O tests done
		std::cout << "PORTB::RB7: all tests concluded successfully" << std::endl;

	}

	void test_ports() {

		std::cout << "Testing PORT A pins" << std::endl;
		std::cout << "===================" << std::endl;
		test_port_pin_ra0();
		test_port_pin_ra1();
		test_port_pin_ra2();
		test_port_pin_ra3();
		test_port_pin_ra4();
		test_port_pin_ra5();
		test_port_pin_ra6();
		test_port_pin_ra7();

		std::cout << std::endl;
		std::cout << "Testing PORT B pins" << std::endl;
		std::cout << "===================" << std::endl;
		test_port_pin_rb0();
		test_port_pin_rb1();
		test_port_pin_rb2();
		test_port_pin_rb3();
		test_port_pin_rb4();
		test_port_pin_rb5();
		test_port_pin_rb6();
		test_port_pin_rb7();
	}

}
#endif
