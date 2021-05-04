#pragma once

#include <set>
#include <iostream>
#include <queue>
#include "constants.h"
#include "device_base.h"

class SRAM: public Device{
	BYTE m_bank[RAM_BANKS][BANK_SIZE];

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

	DeviceEventQueue eq;

	SRAM();


	bool calc_bank_ofs(WORD a_idx, BYTE &bank, BYTE &ofs, bool indirect) const;

	BYTE fsr() const {
		return (m_bank[0][FSR]);
	}

	BYTE &fsr() {
		return (m_bank[0][FSR]);
	}

	BYTE status() const {
		return (m_bank[0][STATUS]);
	}

	BYTE &status() {     // only way to set special bits is by this reference
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

	void reset();

	void write(const WORD a_idx, const BYTE value, bool indirect=false);

	const BYTE read(const WORD a_idx, bool indirect=false) const;
};
