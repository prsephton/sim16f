#include <cassert>
#include <vector>
#include "test_clockcycler.h"
#include "../src/devices/clock.h"
#include "../src/devices/simulated_ports.h"


namespace Tests {

	// Defines the devices we will need to test the comparator
	class MiniMachine {
	  public:
		SRAM sram;
		Comparator cmp0;
		ClockCycler clock;
		std::vector<Terminal>  pin;
		SinglePortA_Analog     ra0;
		SinglePortA_Analog     ra1;
		SinglePortA_Analog_RA2 ra2;
		SinglePortA_Analog_RA3 ra3;
		SinglePortA_Analog_RA4 ra4;

		ClockedRegister CMCON;
		BYTE            cmcon;
		float           vref;

		void on_connection_change(Connection *c, const std::string &name) {
			if (c->name() == "VREF") {
				vref = c->rd();
	//			std::cout << "VREF = " << vref << std::endl;
			}
		}

		void on_comparator_change(Comparator *c, const std::string &name, const std::vector<BYTE> &data) {
	//		std::cout << "Comparator update" << std::endl;
			cmcon = data[Comparator::DVALUE::NEW];
			CMCON.write(sram, cmcon);
		}

		MiniMachine():
			pin({Terminal("Pin0"), Terminal("Pin1"), Terminal("Pin2"), Terminal("Pin3"), Terminal("Pin4")}),
			ra0(pin[0], "RA0"), ra1(pin[1], "RA1"), ra2(pin[2], "RA2"), ra3(pin[3], "RA3"), ra4(pin[4], "RA4"),
			CMCON(SRAM::CMCON, "CMCON"), cmcon(0), vref(0)
		{
			DeviceEvent<Comparator>::subscribe<MiniMachine>(this, &MiniMachine::on_comparator_change);
			DeviceEvent<Connection>::subscribe<MiniMachine>(this, &MiniMachine::on_connection_change);
		}
		~MiniMachine() {
			DeviceEvent<Comparator>::unsubscribe<MiniMachine>(this, &MiniMachine::on_comparator_change);
			DeviceEvent<Connection>::unsubscribe<MiniMachine>(this, &MiniMachine::on_connection_change);
		}

	};

	void test_comparator() {

		MiniMachine m;
		//  Check logic for CMCON modes
		// =================================
		// mode 0 is comparator reset.  C1 compares RA0 > RA3, C2 compares RA2 > RA1
		// mode 1 is 3 inputs.  C1.vin = RA2, C1.ref = RA3 if CIS else RA0.  C1 tests vin > ref.  C2 tests RA2 > RA1
		// mode 2 is C1,C2 multiplexed depending on CIS=0: C1=VREF>RA0, C2=VREF>RA1, CIS=1: C1=VREF>RA3, C2=VREF>RA2
		// mode 3 is common reference;  C1=RA2>RA0, C2=RA2>RA1
		// mode 4 is two independent comparators;  C1=RA3>RA0, C2=RA2>RA1
		// mode 5 is one independent comparator: C1=0, C2=RA2>RA1
		// mode 6 is two common reference comparators with outputs: C1=RA2>AN0, C2=RA2>RA1
		// mode 7 is comparator off; C1=0, C2=0

		// Test mode 0  (reset)

		ClockedRegister PORTA(SRAM::CMCON, "PORTA");
		ClockedRegister TRISA(SRAM::TRISA, "TRISA");
		ClockedRegister VRCON(SRAM::VRCON, "VRCON");
		ClockedRegister CMCON(SRAM::CMCON, "CMCON");

		std::cout << "_______________________________________________________________________________________________" << std::endl;
		std::cout << "Testing Comparator Mode 0 (reset)" << std::endl;
		std::cout << "=================================" << std::endl;

		CMCON.write(m.sram, 0);    // mode 0 = reset
		// set ra0..ra3 as input
		TRISA.write(m.sram, (Flags::TRISA::TRISA0 | Flags::TRISA::TRISA1 | Flags::TRISA::TRISA2 | Flags::TRISA::TRISA3));

		CMCON.read(m.sram);

		assert((m.sram.read(SRAM::CMCON) & Flags::CMCON::C1OUT) == 0);
		// ensure RA0 > RA3 and RA2 > RA1.  If comparator is on, we expect to see it [incorrectly] read 1
		m.pin[0].set_value(3, false);     m.pin[3].set_value(2, false);
		m.pin[2].set_value(3.2, false);   m.pin[1].set_value(2.8, false);

		assert((CMCON.read(m.sram) & Flags::CMCON::C1OUT) == 0);
		assert((CMCON.read(m.sram) & Flags::CMCON::C2OUT) == 0);

		std::cout << "_______________________________________________________________________________________________" << std::endl;
		std::cout << "Testing Comparator Mode 7 (off)" << std::endl;
		std::cout << "===============================" << std::endl;
		// Test mode 7   ( comparator off )
		CMCON.write(m.sram, 7);
		CMCON.read(m.sram);

		assert((m.cmcon & Flags::CMCON::C1OUT) == 0);
		assert((m.cmcon & Flags::CMCON::C2OUT) == 0);

		std::cout << "_______________________________________________________________________________________________" << std::endl;
		std::cout << "Testing Comparator Mode 1 ( 3 way multiplex )" << std::endl;
		std::cout << "=============================================" << std::endl;
		// Test mode 1   ( 3 way multiplex )
		CMCON.write(m.sram, 1);

		assert((bool)(m.cmcon & Flags::CMCON::C1OUT) == (m.pin[2].rd() > m.pin[0].rd()));
		assert((bool)(m.cmcon & Flags::CMCON::C2OUT) == (m.pin[2].rd() > m.pin[1].rd()));

		CMCON.write(m.sram, Flags::CMCON::CIS | 1);

		assert((bool)(m.cmcon & Flags::CMCON::C1OUT) == (m.pin[2].rd() > m.pin[3].rd()));
		assert((bool)(m.cmcon & Flags::CMCON::C2OUT) == (m.pin[2].rd() > m.pin[1].rd()));

		std::cout << "_______________________________________________________________________________________________" << std::endl;
		std::cout << "Testing Comparator Mode 2 ( 4 inputs multiplexed, and vin=VREF.vin )" << std::endl;
		std::cout << "====================================================================" << std::endl;
		// Test mode 2   ( 4 inputs multiplexed, and vin=VREF.vin )
		VRCON.write(m.sram,  Flags::VRCON::VREN | Flags::VRCON::VRR | 12); // VROE is off
		assert(m.vref == 2.5);

		CMCON.write(m.sram, 2);
		assert((m.cmcon & 7) == 2);  // assert mode 2 selected

		assert((bool)(m.cmcon & Flags::CMCON::C1OUT) == (2.5 > m.pin[0].rd()));
		assert((bool)(m.cmcon & Flags::CMCON::C2OUT) == (2.5 > m.pin[1].rd()));

		CMCON.write(m.sram, Flags::CMCON::CIS | 2);

		assert((bool)(m.cmcon & Flags::CMCON::C1OUT) == (2.5 > m.pin[3].rd()));
		assert((bool)(m.cmcon & Flags::CMCON::C2OUT) == (2.5 > m.pin[2].rd()));

	//	CMCON.debug(true);
	//	m.cmp0.debug(true);

		std::cout << "_______________________________________________________________________________________________" << std::endl;
		std::cout << "Testing Comparator Mode 3 (test mode 3: common reference;  C1=RA2>RA0, C2=RA2>RA1)" << std::endl;
		std::cout << "==================================================================================" << std::endl;
		// test mode 3: common reference;  C1=RA2>RA0, C2=RA2>RA1

	//	m.cmp0.debug(true);
		CMCON.write(m.sram, 3);

		assert((m.cmcon & 7) == 3);  // assert mode 3 selected

	//	for (int n = 0; n < 4; ++n) {
	//		std::cout << "pin #"<<n<<" = "<<m.pin[n].rd(false)<<";  ";
	//	} std::cout << std::endl;
	//
	//	std::cout << "cmcon; C1OUT=" << (int)(m.cmcon & Flags::CMCON::C1OUT) << ";  C2OUT=" << (int)(m.cmcon & Flags::CMCON::C2OUT) << std::endl;

		assert((bool)(m.cmcon & Flags::CMCON::C1OUT) == (m.pin[2].rd() > m.pin[0].rd()));
		assert((bool)(m.cmcon & Flags::CMCON::C2OUT) == (m.pin[2].rd() > m.pin[1].rd()));

		// test inverse function
		CMCON.write(m.sram, 3 | Flags::CMCON::C1INV | Flags::CMCON::C2INV);
		assert((bool)(m.cmcon & Flags::CMCON::C1OUT) == (m.pin[2].rd() < m.pin[0].rd()));
		assert((bool)(m.cmcon & Flags::CMCON::C2OUT) == (m.pin[2].rd() < m.pin[1].rd()));

		std::cout << "_______________________________________________________________________________________________" << std::endl;
		std::cout << "Testing Comparator Mode 4 (two independent comparators;  C1=RA3>RA0, C2=RA2>RA1)" << std::endl;
		std::cout << "================================================================================" << std::endl;
		// test mode 4: two independent comparators;  C1=RA3>RA0, C2=RA2>RA1
		CMCON.write(m.sram, 4);
		assert((m.cmcon & 7) == 4);  // assert mode 4 selected

		assert((bool)(m.cmcon & Flags::CMCON::C1OUT) == (m.pin[3].rd() > m.pin[0].rd()));
		assert((bool)(m.cmcon & Flags::CMCON::C2OUT) == (m.pin[2].rd() > m.pin[1].rd()));

		std::cout << "_______________________________________________________________________________________________" << std::endl;
		std::cout << "Testing Comparator Mode 5 (one independent comparator: C1=0, C2=RA2>RA1)" << std::endl;
		std::cout << "========================================================================" << std::endl;
		// test mode 5: one independent comparator: C1=0, C2=RA2>RA1
		CMCON.write(m.sram, 5);
		assert((m.cmcon & 7) == 5);  // assert mode 5 selected

		assert((bool)(m.cmcon & Flags::CMCON::C1OUT) == 0);
		assert((bool)(m.cmcon & Flags::CMCON::C2OUT) == (m.pin[2].rd() > m.pin[1].rd()));

		std::cout << "_______________________________________________________________________________________________" << std::endl;
		std::cout << "Testing Comparator Mode 6 (two common ref comparators with outputs: C1=RA2>AN0, C2=RA2>RA1)" << std::endl;
		std::cout << "===========================================================================================" << std::endl;
		// test mode 6: two common reference comparators with outputs: C1=RA2>AN0, C2=RA2>RA1
		// set PORTA::RA3 and PORTA::RA4 as outputs

		auto c_p3 = m.ra3.components();
		auto c_p4 = m.ra4.components();
	//	Wire &w_p4 = dynamic_cast<Wire &>(*c_p4["Pin Wire"]);
	//	OrGate &nor1_p4 = dynamic_cast<OrGate &>(*c_p4["NOR Gate"]);
	//	FET &fet1_p4 = dynamic_cast<FET &>(*c_p4["FET1"]);
	//	w_p4.debug(true);
	//	nor1_p4.debug(true);
	//	fet1_p4.debug(true);

		m.pin[4].set_value(m.pin[4].Vdd, false);  // an input on pin4 (drain)
		TRISA.write(m.sram, (Flags::TRISA::TRISA0 | Flags::TRISA::TRISA1 | Flags::TRISA::TRISA2));

		CMCON.write(m.sram, 6);
		assert((m.cmcon & 7) == 6);  // assert mode 6 selected

		assert((bool)(m.cmcon & Flags::CMCON::C1OUT) == (m.pin[2].rd() > m.pin[0].rd()));
		assert((bool)(m.cmcon & Flags::CMCON::C2OUT) == (m.pin[2].rd() > m.pin[1].rd()));

		Mux &mux_p3 = dynamic_cast<Mux &>(*c_p3["Mux"]);
		Mux &mux_p4 = dynamic_cast<Mux &>(*c_p4["Mux"]);
		assert(mux_p3.rd().signal() == (m.pin[2].rd() > m.pin[0].rd()));
		assert(mux_p4.rd().signal() == (m.pin[2].rd() > m.pin[1].rd()));

		Latch &TrisLatch_p3 = dynamic_cast<Latch &>(*c_p3["Tris Latch"]);
		Latch &TrisLatch_p4 = dynamic_cast<Latch &>(*c_p4["Tris Latch"]);

		assert(TrisLatch_p3.Qc().signal());   // Port::pin3 set as output
		assert(TrisLatch_p4.Qc().signal());   // Port::pin4 set as output

		Tristate &ts1_p3 = dynamic_cast<Tristate &>(*c_p3["Tristate1"]);
		assert(!ts1_p3.gate().signal());
		assert(ts1_p3.rd().signal() == mux_p3.rd().signal());

		assert(m.pin[3].signal() == (m.pin[2].rd() > m.pin[0].rd()));
		assert(m.pin[4].signal() == (m.pin[2].rd() > m.pin[1].rd()));

		std::cout << "_______________________________________________________________________________________________" << std::endl;
		std::cout << "Comparator modes passed all defined tests" << std::endl;
	}


	void test_comparator_module() {
		test_comparator();
	}

}
