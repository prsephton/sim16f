//__________________________________________________________________________________________________
// Definitions for flags in various registers
//
//  This class implements everything as static members, so it all gets initialised at startup,
// and the class does not need to be instantiated before being used.

#ifndef __flags_h__
#define __flags_h__
#include <vector>
#include <string>
#include <map>

class Flags {
  public:
	struct CONFIG {
		static const WORD CP    = (WORD)1 << 13;  // bit 13
		static const WORD CPD   = (WORD)1 << 8;   // bit 8
		static const WORD LVP   = (WORD)1 << 7;
		static const WORD BOREN = (WORD)1 << 6;
		static const WORD MCLRE = (WORD)1 << 5;
		static const WORD FOSC2 = (WORD)1 << 4;
		static const WORD PWRTE = (WORD)1 << 3;   // ...
		static const WORD WDTE  = (WORD)1 << 2;
		static const WORD FOSC1 = (WORD)1 << 1;
		static const WORD FOSC0 = (WORD)1;        // bit 0
		static const std::vector<std::string> bits;
	};
	struct STATUS {
		static const BYTE IRP = 0b10000000;
		static const BYTE RP1 = 0b01000000;
		static const BYTE RP0 = 0b00100000;
		static const BYTE TO  = 0b00010000;
		static const BYTE PD  = 0b00001000;
		static const BYTE Z   = 0b00000100;
		static const BYTE DC  = 0b00000010;
		static const BYTE C   = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct OPTION {
		static const BYTE RBPU   = 0b10000000;
		static const BYTE INTEDG = 0b01000000;
		static const BYTE T0CS   = 0b00100000;
		static const BYTE T0SE   = 0b00010000;
		static const BYTE PSA    = 0b00001000;
		static const BYTE PS2    = 0b00000100;
		static const BYTE PS1    = 0b00000010;
		static const BYTE PS0    = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct TRISA {
		static const BYTE TRISA7 = 0b10000000;
		static const BYTE TRISA6 = 0b01000000;
		static const BYTE TRISA5 = 0b00100000;
		static const BYTE TRISA4 = 0b00010000;
		static const BYTE TRISA3 = 0b00001000;
		static const BYTE TRISA2 = 0b00000100;
		static const BYTE TRISA1 = 0b00000010;
		static const BYTE TRISA0 = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct TRISB {
		static const BYTE TRISB7 = 0b10000000;
		static const BYTE TRISB6 = 0b01000000;
		static const BYTE TRISB5 = 0b00100000;
		static const BYTE TRISB4 = 0b00010000;
		static const BYTE TRISB3 = 0b00001000;
		static const BYTE TRISB2 = 0b00000100;
		static const BYTE TRISB1 = 0b00000010;
		static const BYTE TRISB0 = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct INTCON {
		static const BYTE GIE  = 0b10000000;
		static const BYTE PEIE = 0b01000000;
		static const BYTE T0IE = 0b00100000;
		static const BYTE INTE = 0b00010000;
		static const BYTE RBIE = 0b00001000;
		static const BYTE T0IF = 0b00000100;
		static const BYTE INTF = 0b00000010;
		static const BYTE RBIF = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct PIE1 {
		static const BYTE EEIE   = 0b10000000;
		static const BYTE CMIE   = 0b01000000;
		static const BYTE RCIE   = 0b00100000;
		static const BYTE TXIE   = 0b00010000;
		static const BYTE na0    = 0b00001000;
		static const BYTE CCP1IE = 0b00000100;
		static const BYTE TMR2IE = 0b00000010;
		static const BYTE TMR1IE = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct PCON {
		static const BYTE na0  = 0b10000000;
		static const BYTE na1  = 0b01000000;
		static const BYTE na2  = 0b00100000;
		static const BYTE na3  = 0b00010000;
		static const BYTE OSCF = 0b00001000;
		static const BYTE na4  = 0b00000100;
		static const BYTE POR  = 0b00000010;
		static const BYTE BOR  = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct RCSTA {
		static const BYTE SPEN = 0b10000000;
		static const BYTE RX9  = 0b01000000;
		static const BYTE SREN = 0b00100000;
		static const BYTE CREN = 0b00010000;
		static const BYTE ADEN = 0b00001000;
		static const BYTE FERR = 0b00000100;
		static const BYTE OERR = 0b00000010;
		static const BYTE RX9D = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct TXSTA {
		static const BYTE CSRC = 0b10000000;
		static const BYTE TX9  = 0b01000000;
		static const BYTE TXEN = 0b00100000;
		static const BYTE SYNC = 0b00010000;
		static const BYTE na0  = 0b00001000;
		static const BYTE BRGH = 0b00000100;
		static const BYTE TRMT = 0b00000010;
		static const BYTE TX9D = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct EECON1 {
		static const BYTE na0   = 0b10000000;
		static const BYTE na1   = 0b01000000;
		static const BYTE na2   = 0b00100000;
		static const BYTE na3   = 0b00010000;
		static const BYTE WRERR = 0b00001000;
		static const BYTE WREN  = 0b00000100;
		static const BYTE WR    = 0b00000010;
		static const BYTE RD    = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct CMCON {
		static const BYTE C2OUT = 0b10000000;
		static const BYTE C1OUT = 0b01000000;
		static const BYTE C2INV = 0b00100000;
		static const BYTE C1INV = 0b00010000;
		static const BYTE CIS   = 0b00001000;
		static const BYTE CM2   = 0b00000100;
		static const BYTE CM1   = 0b00000010;
		static const BYTE CM0   = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct VRCON {
		static const BYTE VREN = 0b10000000;
		static const BYTE VROE = 0b01000000;
		static const BYTE VRR  = 0b00100000;
		static const BYTE na0  = 0b00010000;
		static const BYTE VR3  = 0b00001000;
		static const BYTE VR2  = 0b00000100;
		static const BYTE VR1  = 0b00000010;
		static const BYTE VR0  = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct T1CON {
		static const BYTE na2     = 0b10000000;
		static const BYTE na1     = 0b01000000;
		static const BYTE T1CKPS1 = 0b00100000;
		static const BYTE T1CKPS0 = 0b00010000;
		static const BYTE T1OSCEN = 0b00001000;
		static const BYTE T1SYNC  = 0b00000100;
		static const BYTE TMR1CS  = 0b00000010;
		static const BYTE TMR1ON  = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct T2CON {
		static const BYTE na2     = 0b10000000;
		static const BYTE TOUTPS3 = 0b01000000;
		static const BYTE TOUTPS2 = 0b00100000;
		static const BYTE TOUTPS1 = 0b00010000;
		static const BYTE TOUTPS0 = 0b00001000;
		static const BYTE TMR2ON  = 0b00000100;
		static const BYTE T2CKPS1 = 0b00000010;
		static const BYTE T2CKPS0 = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct PORTA {
		static const BYTE RA7 = 0b10000000;
		static const BYTE RA6 = 0b01000000;
		static const BYTE RA5 = 0b00100000;
		static const BYTE RA4 = 0b00010000;
		static const BYTE RA3 = 0b00001000;
		static const BYTE RA2 = 0b00000100;
		static const BYTE RA1 = 0b00000010;
		static const BYTE RA0 = 0b00000001;
		static const std::vector<std::string> bits;
	};
	struct PORTB {
		static const BYTE RB7 = 0b10000000;
		static const BYTE RB6 = 0b01000000;
		static const BYTE RB5 = 0b00100000;
		static const BYTE RB4 = 0b00010000;
		static const BYTE RB3 = 0b00001000;
		static const BYTE RB2 = 0b00000100;
		static const BYTE RB1 = 0b00000010;
		static const BYTE RB0 = 0b00000001;
		static const std::vector<std::string> bits;
	};
	static const std::map<const WORD, const std::vector<std::string> > register_bits;
	static bool bit_number_for_bitname(WORD register_index, const std::string &bit_name, BYTE &bit_number);           // false if not found
	static const std::string bit_name_for_register_bit(WORD register_index, BYTE bit_number);  // empty string if not found
};

#endif
