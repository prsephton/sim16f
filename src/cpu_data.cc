#include "cpu_data.h"

#include "utils/smart_ptr.cc"


template <class Register> class
	DeviceEvent<Register>::registry  DeviceEvent<Register>::subscribers;

CpuEvent::registry CpuEvent::subscribers;

//___________________________________________________________________________________
// Many registers interact directly with hardware, or are used to control features
// directly.

class INDF: public Register {
  public:
	INDF() : Register(SRAM::INDF, "INDF", "Addressing this location uses contents of FSR to address data memory") {}

	WORD indirect_address(const SRAM &a_sram) {
		BYTE fsr = a_sram.fsr();
		BYTE sts = a_sram.status();
		return((WORD)((sts & Flags::STATUS::IRP) << 1) + fsr);
	}

	virtual const BYTE read(const SRAM &a_sram) {
		WORD addr = indirect_address(a_sram);
		BYTE data = a_sram.read(addr, true);
		set_value(data, data);
//		std::cout << "Read address [" << std::hex << (int)addr << "] = " << (int)data << std::endl;
		return data;
	}

	virtual void write(SRAM &a_sram, const BYTE value) {
		WORD addr = indirect_address(a_sram);
//		std::cout << "Write address [" << std::hex << (int)addr << "] = " << (int)value << std::endl;
		set_value(value);
		a_sram.write(addr, value, true);
	}
};

class STATUS: public Register {
	// IRP- Register Bank Select bit (used for indirect addressing)
	// RP<1:0>: Register Bank Select bits (used for direct addressing)
	// TO - Timeout Bit.  1 = After power-up, CLRWDT instruction or SLEEP instruction.  0 = A WDT time out occurred
	// PD - Power down bit. 1 = After power-up or by the CLRWDT instruction. 0 = By execution of the SLEEP instruction.
	// Z: Zero bit
	// DC: Digit Carry/Borrow bit
	// C: Carry/Borrow bit
	// Instructions which change Z,DC,C cannot change these bits on write to the register.
	//

  public:
	STATUS() : Register(SRAM::STATUS, "STATUS", "IRP RP1 RP0 TO PD Z DC C") {}

	virtual void write(SRAM &a_sram, const BYTE value) {  // cannot overwrite some fields,
		BYTE sts = a_sram.status();
		BYTE mask = Flags::STATUS::TO | Flags::STATUS::PD;  // read only bits
		BYTE nvalue = (value & (~mask)) | (sts & mask);
		set_value(nvalue, sts);
	}
};


class OPTION: public Register {
	// RBPU: PORTB Pull-up Enable bit.  1 = PORTB pull-ups are disabled. 0 = PORTB pull-ups are enabled by individual port latch values.
	// INTEDG: Interrupt Edge Select bit. 1 = Interrupt on rising edge of RB0/INT pin. 0 = Interrupt on falling edge of RB0/INT pin.
	// T0CS: TMR0 Clock Source Select bit. 1 = Transition on RA4/T0CKI/CMP2 pin. 0 = Internal instruction cycle clock (CLKOUT).
	// T0SE: TMR0 Source Edge Select bit. 1 = Increment on high-to-low transition on RA4/T0CKI/CMP2 pin. 0 = Increment on low-to-high transition.
	// PSA: Prescaler Assignment bit. 1 = Prescaler is assigned to the WDT. 0 = Prescaler is assigned to the Timer0 module.
	// PS<2:0>: Prescaler Rate Select bits.

  public:
	OPTION() : Register(SRAM::OPTION, "OPTION", "RBPU INTEDG T0CS T0SE PSA PS2 PS1 PS0") {}

	virtual void write(SRAM &a_sram, const BYTE value) {
		BYTE options = a_sram.read(index());
		set_value(value, options);
	}
};


CPU_DATA::CPU_DATA():
		execPC(0), SP(0), W(0), Config(0), porta(pins), portb(pins), cfg1("CONFIG1"), cfg2("CONFIG2") {
	Registers["INDF"]   = new INDF();
	Registers["TMR0"]   = new Register(SRAM::TMR0, "TMR0", "Timer 0");  // bank 0 and 2
	Registers["PCL"]    = new Register(SRAM::PCL, "PCL", "Program Counters Low  Byte");  // all banks
	Registers["STATUS"] = new STATUS();
	Registers["FSR"]    = new Register(SRAM::FSR, "FSR", "Indirect Data Memory Address Pointer");  // all banks
	Registers["PORTA"]  = new Register(SRAM::PORTA, "PORTA", "RA7 RA6 RA5 RA4 RA3 RA2 RA1 RA0");  // bank 0 and 2
	Registers["PORTB"]  = new Register(SRAM::PORTB, "PORTB", "RB7 RB6 RB5 RB4 RB3 RB2 RB1 RB0");  // bank 0 and 2

	Registers["PCLATH"] = new Register(SRAM::PCLATH, "PCLATH", "— — — Write Buffer for upper 5 bits of Program Counter");  // all banks
	Registers["INTCON"] = new Register(SRAM::INTCON, "INTCON", "GIE PEIE T0IE INTE RBIE T0IF INTF RBIF");  // all banks
	Registers["PIR1"]   = new Register(SRAM::PIR1, "PIR1", "EEIF CMIF RCIF TXIF — CCP1IF TMR2IF TMR1IF 0");

	Registers["TMR1L"]  = new Register(SRAM::TMR1L, "TMR1L", "Holding Register for the Least Significant Byte of the 16-bit TMR1 Register");
	Registers["TMR1H"]  = new Register(SRAM::TMR1H, "TMR1H", "Holding Register for the Most Significant Byte of the 16-bit TMR1 Register");
	Registers["T1CON"]  = new Register(SRAM::T1CON, "T1CON", "— — T1CKPS1 T1CKPS0 T1OSCEN T1SYNC TMR1CS TMR1ON");
	Registers["TMR2"]   = new Register(SRAM::TMR2, "TMR2", "TMR2 Module’s Register");
	Registers["T2CON"]  = new Register(SRAM::T2CON, "T2CON", "— TOUTPS3 TOUTPS2 TOUTPS1 TOUTPS0 TMR2ON T2CKPS1 T2CKPS0");

	Registers["CCPR1L"] = new Register(SRAM::CCPR1L, "CCPR1L", "Capture/Compare/PWM Register (LSB)");
	Registers["CCPR1H"] = new Register(SRAM::CCPR1H, "CCPR1H", "Capture/Compare/PWM Register (MSB)");
	Registers["CCP1CON"]= new Register(SRAM::CCP1CON, "CCPCON", "— — CCP1X CCP1Y CCP1M3 CCP1M2 CCP1M1 CCP1M0");
	Registers["RCSTA"]  = new Register(SRAM::RCSTA, "RCSTA", "SPEN RX9 SREN CREN ADEN FERR OERR RX9D");
	Registers["TXREG"]  = new Register(SRAM::TXREG, "TCREG", "USART Transmit Data Register");
	Registers["RCREG"]  = new Register(SRAM::RCREG, "RCREG", "USART Receive Data Register");
	Registers["CMCON"]  = new Register(SRAM::CMCON, "CMCON", "C2OUT C1OUT C2INV C1INV CIS CM2 CM1 CM0");

	Registers["OPTION"] = new Register(SRAM::OPTION, "OPTION", "RBPU INTEDG T0CS T0SE PSA PS2 PS1 PS0");

	Registers["TRISA"]  = new Register(SRAM::TRISA, "TRISA", "TRISA7 TRISA6 TRISA5 TRISA4 TRISA3 TRISA2 TRISA1 TRISA0");
	Registers["TRISB"]  = new Register(SRAM::TRISB, "TRISB", "TRISB7 TRISB6 TRISB5 TRISB4 TRISB3 TRISB2 TRISB1 TRISB0");

	Registers["PIE1"]   = new Register(SRAM::PIE1, "PIE1", "EEIE CMIE RCIE TXIE — CCP1IE TMR2IE TMR1IE");

	Registers["PCON"]   = new Register(SRAM::PCON, "PCON", "— — — — OSCF — POR BOR");

	Registers["PR2"]    = new Register(SRAM::PR2, "PR2", "Timer2 Period Register");

	Registers["TXSTA"]  = new Register(SRAM::TXSTA, "TXSTA", "CSRC TX9 TXEN SYNC — BRGH TRMT TX9D");
	Registers["SPBRG"]  = new Register(SRAM::SPBRG, "SPBRG", "Baud Rate Generator Register");

	Registers["EEDATA"] = new Register(SRAM::EEDATA, "EEDATA", "EEPROM Data Register");
	Registers["EEADR"]  = new Register(SRAM::EEADR, "EEADR", "EEPROM Address Register");
	Registers["EECON1"] = new Register(SRAM::EECON1, "EECON1", "— — — — WRERR WREN WR RD");
	Registers["EECON2"] = new Register(SRAM::EECON2, "EECON2", "EEPROM Control Register 2 (not a physical register)");

	Registers["VRCON"]  = new Register(SRAM::VRCON, "VRCON", "VREN VROE VRR — VR3 VR2 VR1 VR0");

	typedef std::map<std::string, SmartPtr<Register> >::iterator register_foreach;
	for (register_foreach r=Registers.begin(); r != Registers.end(); ++r) {
		RegisterNames[r->second->index()] = r->first;
	}

	DeviceEvent<Register>::subscribe<CPU_DATA>(this, &CPU_DATA::register_changed);
	DeviceEvent<Comparator>::subscribe<CPU_DATA>(this, &CPU_DATA::comparator_changed);
	DeviceEvent<Timer0>::subscribe<CPU_DATA>(this, &CPU_DATA::timer0_changed);
	DeviceEvent<Timer1>::subscribe<CPU_DATA>(this, &CPU_DATA::timer1_changed);
	DeviceEvent<PORTB>::subscribe<CPU_DATA>(this, &CPU_DATA::portB_changed);
}

CPU_DATA::~CPU_DATA() {
	DeviceEvent<Register>::unsubscribe<CPU_DATA>(this, &CPU_DATA::register_changed);
	DeviceEvent<Comparator>::unsubscribe<CPU_DATA>(this, &CPU_DATA::comparator_changed);
	DeviceEvent<Timer0>::unsubscribe<CPU_DATA>(this, &CPU_DATA::timer0_changed);
	DeviceEvent<Timer1>::unsubscribe<CPU_DATA>(this, &CPU_DATA::timer1_changed);
	DeviceEvent<PORTB>::unsubscribe<CPU_DATA>(this, &CPU_DATA::portB_changed);
}

void CPU_DATA::register_changed(Register *r, const std::string &name, const std::vector<BYTE> &data) {
	static const std::set<std::string> ignore({"CONFIG1", "CONFIG2", "CONFIG1.read", "CONFIG2.read", "PORTA.read", "PORTB.read", "TRISA.read", "TRISB.read"});

	if (ignore.find(name) == ignore.end()) {
		if (name.find(".read") == name.npos) {      // a write operation
			if (r->index())
				sram.write(r->index(), data[Register::DVALUE::NEW], false);
			else {
				BYTE address = sram.read(SRAM::FSR, false);    // indirect
				sram.write(address, data[Register::DVALUE::NEW], true);
			}
		} else {         // a read operation
			BYTE sdata;
			if (r->index()) {
				sdata = sram.read(r->index(), false);
			} else {
				BYTE address = sram.read(SRAM::FSR, false);    // indirect
				sdata = sram.read(address, true);
			}
			r->set_value(sdata, r->get_value());
			r->busy(false);  // indicate data is ready in the register
		}
	}
//	std::cout << name << " = " << std::hex <<  (int)data[1] << std::endl;
}

void CPU_DATA::portB_changed(PORTB *p, const std::string &name, const std::vector<BYTE> &data) {
	if (name == "PORTB::INTF") {
//		std::cout << "INTCON::INTF" << std::endl;
		auto INTCON = Registers["INTCON"];
		BYTE idata = INTCON->get_value();
		INTCON->write(sram, idata | Flags::INTCON::INTF);
	}
}


void CPU_DATA::timer0_changed(Timer0 *t, const std::string &name, const std::vector<BYTE> &data) {
	if (name == "Overflow") {
		auto INTCON = Registers["INTCON"];
		BYTE idata = INTCON->get_value();
		INTCON->write(sram, idata | Flags::INTCON::T0IF);
	} else if (name == "Value") {
		auto TMR0 = Registers["TMR0"];
		TMR0->set_value(data[0], data[0]);   // update in memory, but don't trigger a change.
		sram.write(TMR0->index(), data[0]);  // update the SRAM value separately.
	}
}

void CPU_DATA::timer1_changed(Timer1 *t, const std::string &name, const std::vector<BYTE> &data) {
	if (name == "Overflow") {
		auto PIR1 = Registers["PIR1"];
		BYTE idata = PIR1->get_value();
		PIR1->write(sram, idata | Flags::PIR1::TMR1IF);
	} else if (name == "Value") {
		auto TMR1L = Registers["TMR1L"];
		auto TMR1H = Registers["TMR1H"];
		TMR1L->set_value(data[0], data[0]);
		TMR1H->set_value(data[1], data[1]);   // update in memory, but don't trigger a change.
		sram.write(TMR1L->index(), data[0]);
		sram.write(TMR1H->index(), data[1]);  // update the SRAM value separately.
	}
}


void CPU_DATA::comparator_changed(Comparator *c, const std::string &name, const std::vector<BYTE> &data) {
	auto r = Registers.find("CMCON");
	if (r != Registers.end())    // update CMCON register from comparator
		r->second->write(sram, data[Comparator::DVALUE::NEW]);   // this signal event from comparator module
}




