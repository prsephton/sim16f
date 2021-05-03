#include "cpu_data.h"

#include "utils/smart_ptr.cc"


template <class Register> class
	DeviceEvent<Register>::registry  DeviceEvent<Register>::subscribers;


//___________________________________________________________________________________
// Many registers interact directly with hardware, or are used to control features
// directly.

class INDF: public Register {
  public:
	INDF() : Register(SRAM::INDF,  "Addressing this location uses contents of FSR to address data memory") {}

	WORD indirect_address(const SRAM &a_sram) {
		BYTE fsr = a_sram.fsr();
		BYTE sts = a_sram.status();
		return((WORD)((sts & Flags::STATUS::IRP) << 1) + fsr);
	}

	virtual const BYTE read(const SRAM &a_sram) {
		return(a_sram.read(indirect_address(a_sram)), true);
	}

	virtual void write(SRAM &a_sram, const char value) {
		a_sram.write(indirect_address(a_sram), value, true);
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
	STATUS() : Register(SRAM::STATUS,  "IRP RP1 RP0 TO PD Z DC C") {}

	virtual void write(SRAM &a_sram, const BYTE value) {  // cannot overwrite some fields,
		BYTE sts = a_sram.status();
		BYTE mask = Flags::STATUS::TO | Flags::STATUS::PD;  // read only bits
		BYTE nvalue = (value & (!mask)) | (sts & mask);

		BYTE changed = sts ^ nvalue; // all changing bits.
		if (changed)
			eq.queue_event(new DeviceEvent<Register>(*this, "OPTION", {sts, changed, nvalue}));

		a_sram.write(index(), value);
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
	OPTION() : Register(SRAM::OPTION,  "RBPU INTEDG T0CS T0SE PSA PS2 PS1 PS0") {}

	virtual void write(SRAM &a_sram, const BYTE value) {
		BYTE options = a_sram.read(index());
		BYTE changed = options ^ value; // all changing bits.
		if (changed)
			eq.queue_event(new DeviceEvent<Register>(*this, "OPTION", {options, changed, value}));

		a_sram.write(index(), value);
	}
};


CPU_DATA::CPU_DATA(): execPC(0), SP(0), W(0), Config(0) {
	Registers["INDF"]   = new INDF();
	Registers["TMR0"]   = new Register(SRAM::TMR0, "Timer 0");  // bank 0 and 2
	Registers["PCL"]    = new Register(SRAM::PCL, "Program Counters Low  Byte");  // all banks
	Registers["STATUS"] = new STATUS();
	Registers["FSR"]    = new Register(SRAM::FSR, "Indirect Data Memory Address Pointer");  // all banks
	Registers["PORTA"]  = new Register(SRAM::PORTA, "RA7 RA6 RA5 RA4 RA3 RA2 RA1 RA0");  // bank 0 and 2
	Registers["PORTB"]  = new Register(SRAM::PORTB, "RB7 RB6 RB5 RB4 RB3 RB2 RB1 RB0");  // bank 0 and 2

	Registers["PCLATH"] = new Register(SRAM::PCLATH, "— — — Write Buffer for upper 5 bits of Program Counter");  // all banks
	Registers["INTCON"] = new Register(SRAM::INTCON, "GIE PEIE T0IE INTE RBIE T0IF INTF RBIF");  // all banks
	Registers["PIR1"]   = new Register(SRAM::PIR1, "EEIF CMIF RCIF TXIF — CCP1IF TMR2IF TMR1IF 0");

	Registers["TMR1L"]  = new Register(SRAM::TMR1L, "Holding Register for the Least Significant Byte of the 16-bit TMR1 Register");
	Registers["TMR1H"]  = new Register(SRAM::TMR1H, "Holding Register for the Most Significant Byte of the 16-bit TMR1 Register");
	Registers["T1CON"]  = new Register(SRAM::T1CON, "— — T1CKPS1 T1CKPS0 T1OSCEN T1SYNC TMR1CS TMR1ON");
	Registers["TMR2"]   = new Register(SRAM::TMR2, "TMR2 Module’s Register");
	Registers["T2CON"]  = new Register(SRAM::T2CON, "— TOUTPS3 TOUTPS2 TOUTPS1 TOUTPS0 TMR2ON T2CKPS1 T2CKPS0");

	Registers["CCPR1L"] = new Register(SRAM::CCPR1L, "Capture/Compare/PWM Register (LSB)");
	Registers["CCPR1H"] = new Register(SRAM::CCPR1H, "Capture/Compare/PWM Register (MSB)");
	Registers["CCP1CON"]= new Register(SRAM::CCP1CON, "— — CCP1X CCP1Y CCP1M3 CCP1M2 CCP1M1 CCP1M0");
	Registers["RCSTA"]  = new Register(SRAM::RCSTA, " SPEN RX9 SREN CREN ADEN FERR OERR RX9D");
	Registers["TXREG"]  = new Register(SRAM::TXREG, "USART Transmit Data Register");
	Registers["RCREG"]  = new Register(SRAM::RCREG, "USART Receive Data Register");
	Registers["CMCON"]  = new Register(SRAM::CMCON, "C2OUT C1OUT C2INV C1INV CIS CM2 CM1 CM0");

	Registers["OPTION"] = new Register(SRAM::OPTION, "RBPU INTEDG T0CS T0SE PSA PS2 PS1 PS0");

	Registers["TRISA"]  = new Register(SRAM::TRISA, "TRISA7 TRISA6 TRISA5 TRISA4 TRISA3 TRISA2 TRISA1 TRISA0");
	Registers["TRISB"]  = new Register(SRAM::TRISB, "TRISB7 TRISB6 TRISB5 TRISB4 TRISB3 TRISB2 TRISB1 TRISB0");

	Registers["PIE1"]   = new Register(SRAM::PIE1, "EEIE CMIE RCIE TXIE — CCP1IE TMR2IE TMR1IE");

	Registers["PCON"]   = new Register(SRAM::PCON, "— — — — OSCF — POR BOR");

	Registers["PR2"]    = new Register(SRAM::PR2, "Timer2 Period Register");

	Registers["TXSTA"]  = new Register(SRAM::TXSTA, "CSRC TX9 TXEN SYNC — BRGH TRMT TX9D");
	Registers["SPBRG"]  = new Register(SRAM::SPBRG, "Baud Rate Generator Register");

	Registers["EEDATA"] = new Register(SRAM::EEDATA, "EEPROM Data Register");
	Registers["EEADR"]  = new Register(SRAM::EEADR, "EEPROM Address Register");
	Registers["EECON1"] = new Register(SRAM::EECON1, "— — — — WRERR WREN WR RD");
	Registers["EECON2"] = new Register(SRAM::EECON2, "EEPROM Control Register 2 (not a physical register)");

	Registers["VRCON"]  = new Register(SRAM::VRCON, "VREN VROE VRR — VR3 VR2 VR1 VR0");

	typedef std::map<std::string, SmartPtr<Register> >::iterator register_foreach;
	for (register_foreach r=Registers.begin(); r != Registers.end(); ++r) {
		RegisterNames[r->second->index()] = r->first;
	}
}


//void CPU_DATA::process_option(const SRAM::Event &e) {
//	if (e.changed & Flags::OPTION::RBPU) {
//		portb.recalc_pullups(pins, e.new_value & Flags::OPTION::RBPU);
//	}
//	if (e.changed & Flags::OPTION::INTEDG) {
//		portb.rising_rb0_interrupt(pins, e.new_value & Flags::OPTION::INTEDG);
//	}
//	if (e.changed & Flags::OPTION::T0CS) {
//		tmr0.clock_source_select(e.new_value & Flags::OPTION::T0CS);
//	}
//	if (e.changed & Flags::OPTION::T0SE) {
//		tmr0.clock_transition(e.new_value & Flags::OPTION::T0SE);
//	}
//	if (e.changed & Flags::OPTION::PSA) {
//		tmr0.prescaler(e.new_value & Flags::OPTION::PSA);
//	}
//	if (e.changed & (Flags::OPTION::PS0 | Flags::OPTION::PS1 | Flags::OPTION::PS2)) {
//		tmr0.prescaler_rate_select(e.new_value & 0x7);
//	}
//}
//
//void CPU_DATA::process_sram_event(const SRAM::Event &e) {
//	if (e.name == "OPTION") process_option(e);
//}

// static definition
CpuEvent::registry CpuEvent::subscribers;

