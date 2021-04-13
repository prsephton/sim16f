#include "cpu_data.h"
#include "smart_ptr.cc"

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


CPU_DATA::CPU_DATA(): SP(0), W(0) {
	Registers["INDF"]   = new INDF();
	Registers["TMR0"]   = new Register(SRAM::TMR0, "Timer 0");  // bank 0 and 2
	Registers["PCL"]    = new Register(SRAM::PCL, "Program Counters Low  Byte");  // all banks
	Registers["STATUS"] = new Register(SRAM::STATUS, "IRP RP1 RP0 TO PD Z DC C");  // all banks
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
