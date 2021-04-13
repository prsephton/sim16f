#ifndef __sram_h__
#define __sram_h__

#include <set>
#include <iostream>
#include "constants.h"


class SRAM {
	BYTE m_bank[RAM_BANKS][BANK_SIZE];

	bool calc_bank_ofs(const WORD a_idx, BYTE &bank, BYTE &ofs, bool indirect) const;


  public:
	static const WORD INDF    = 0x00;
	static const WORD TMR0    = 0x01;
	static const WORD PCL     = 0x02;
	static const WORD STATUS  = 0x03;
	static const WORD FSR     = 0x04;
	static const WORD PORTA   = 0x05;
	static const WORD PORTB   = 0x06;

	static const WORD PCLATH  = 0x0a;
	static const WORD INTCON  = 0x0b;
	static const WORD PIR1    = 0x0c;

	static const WORD TMR1L   = 0x0e;
	static const WORD TMR1H   = 0x0f;
	static const WORD T1CON   = 0x10;
	static const WORD TMR2    = 0x11;
	static const WORD T2CON   = 0x12;

	static const WORD CCPR1L  = 0x15;
	static const WORD CCPR1H  = 0x16;
	static const WORD CCP1CON = 0x17;
	static const WORD RCSTA   = 0x18;
	static const WORD TXREG   = 0x19;

	static const WORD RCREG   = 0x1a;

	static const WORD CMCON   = 0x1f;

	static const WORD OPTION  = 0x81;

	static const WORD TRISA   = 0x85;
	static const WORD TRISB   = 0x86;

	static const WORD PIE1    = 0x8c;

	static const WORD PCON    = 0x8e;
	static const WORD PR2     = 0x92;


	static const WORD TXSTA   = 0x98;
	static const WORD SPBRG   = 0x99;

	static const WORD EEDATA  = 0x9a;
	static const WORD EEADR   = 0x9b;
	static const WORD EECON1  = 0x9c;
	static const WORD EECON2  = 0x9d;

	static const WORD VRCON   = 0x9f;


	std::set<WORD> ALL_BANK;
	std::set<WORD> EVEN_BANK;
	std::set<WORD> ODD_BANK;
	std::set<WORD> BANK_0;
	std::set<WORD> BANK_1;


	SRAM() {
		WORD all_banks[] = {INDF, PCL, STATUS, FSR, PCLATH, INTCON};
		WORD even_banks[] = {TMR0, PORTB};
		WORD odd_banks[] = {OPTION, TRISB};
		WORD bank_0[] = {PORTA, PIR1, TMR1L, TMR1H, T1CON, TMR2, T2CON, CCPR1L, CCPR1H, CCP1CON, RCSTA, TXREG, RCREG, CMCON};
		WORD bank_1[] = {TRISA, PIE1, PCON, PR2, TXSTA, SPBRG, EEDATA, EEADR, EECON1, EECON2, VRCON};
		ALL_BANK = std::set<WORD>(all_banks, all_banks+6);
		EVEN_BANK = std::set<WORD>(even_banks, even_banks+2);
		ODD_BANK = std::set<WORD>(odd_banks, odd_banks+2);
		BANK_0 = std::set<WORD>(bank_0, bank_0+14);
		BANK_1 = std::set<WORD>(bank_1, bank_1+11);
	}

	BYTE fsr() const {
		return (m_bank[0][FSR]);
	}

	BYTE &fsr() {
		return (m_bank[0][FSR]);
	}

	BYTE status() const {
		return (m_bank[0][STATUS]);
	}

	BYTE &status() {
		return (m_bank[0][STATUS]);
	}

	const BYTE bank() const {
		return (status() & 0x60) >> 5;
	}

	void bank(BYTE n) {
		BYTE &sts = status();
		sts = (sts & ~0x60) | (n << 5);
	}

	BYTE option() const {
		return (m_bank[1][OPTION % BANK_SIZE]);
	}

	BYTE &option() {
		return (m_bank[1][OPTION % BANK_SIZE]);
	}

	WORD get_PC() const {
		WORD hi = m_bank[0][PCLATH] & 0x1f;
		WORD lo = m_bank[0][PCL];
		WORD PC =  hi << 8 | lo;
		return PC;
	}

	void set_PC(WORD PC) {
		m_bank[0][PCLATH] = (BYTE)(PC >> 8) & 0x1f;
		m_bank[0][PCL] = (BYTE)(PC & 0xff);
	}

	void write(const WORD a_idx, const BYTE value, bool indirect=false);

	const BYTE read(const WORD a_idx, bool indirect=false) const;
};

#endif
