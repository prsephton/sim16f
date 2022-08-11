#include "sram.h"
#include "flags.h"

const WORD SRAM::calc_index(const BYTE a_idx, bool indirect) const {
	BYTE sts = status();
	WORD index = a_idx;
	if (indirect) index = index | (sts & Flags::STATUS::IRP) << 1;
	else index = index | ((sts & (Flags::STATUS::RP0 |  Flags::STATUS::RP1)) << 2);

	if (ALL_BANK.find(index % BANK_SIZE) != ALL_BANK.end()) { index = index % BANK_SIZE; }
	else if (EVEN_BANK.find(index % (2*BANK_SIZE)) != EVEN_BANK.end()) { index = index % (2*BANK_SIZE); }
	else if (ODD_BANK.find(index % (2*BANK_SIZE)) != ODD_BANK.end()) { index =  index % (2*BANK_SIZE); }

	return index;
}


void SRAM::write(const WORD a_idx, const BYTE value, bool indirect) {
	WORD index = calc_index(a_idx, indirect);
	m_bank[index / BANK_SIZE][index % BANK_SIZE] = value;
}

const BYTE SRAM::read(const WORD a_idx, bool indirect) const {
	WORD index = calc_index(a_idx, indirect);
	return m_bank[index / BANK_SIZE][index % BANK_SIZE];
}

void SRAM::reset() {
	set_PC(0);
	 for (int bank = 0; bank < RAM_BANKS; ++bank)
		 for (int ofs = 0; ofs < BANK_SIZE; ++ofs)
			 m_bank[bank][ofs] = 0;
}


SRAM::SRAM() {
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


