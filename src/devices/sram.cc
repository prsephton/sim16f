#include "sram.h"


bool SRAM::calc_bank_ofs(WORD a_idx, BYTE &bank, BYTE &ofs, bool indirect) const {
	if (a_idx < BANK_SIZE) {
		bank = this->bank();
		a_idx += (int)bank * BANK_SIZE;
	}
	else
		bank = a_idx / BANK_SIZE;

	ofs  = a_idx % BANK_SIZE;
	if (ofs > 0x6f) bank = 0;      // shared access across banks
	if (!indirect && bank == 0) {
		bank = (status() & 0b01100000) >> 5;
	}
//	std::cout << "Calculate bank & offset [" << std::hex << (int)a_idx << "] ; ";
	if (ofs > 0x1f) {
		if ((ofs - 0x20) + bank * 80 > MAX_MEMORY)
			return false;
	} else if (ALL_BANK.find((int)ofs)==ALL_BANK.end()) {
		if (EVEN_BANK.find(a_idx) != EVEN_BANK.end()) {
			if (bank % 2 == 1) {
				std::cout << "expected even bank, but found odd\n";
				return false;
			}
			bank = 0;
		} else if (ODD_BANK.find(a_idx) != ODD_BANK.end()) {
			if (bank % 2 == 0) {
				std::cout << "expected odd bank, but found even\n";
				return false;
			}
			bank = 1;
		} else if (BANK_0.find(a_idx) != BANK_0.end()) {
			if (bank != 0) {
				std::cout << "expected bank 0, but found bank" << (int)bank << "\n";
				return false;
			}
		} else if (BANK_1.find(a_idx) != BANK_1.end()) {
			if (bank != 1) {
				std::cout << "expected bank 1, but found bank" << (int)bank << "\n";
				return false;
			}
		} else {
//			std::cout << "unknown register [" << std::hex << (int)a_idx << "]\n";
			return false;
		}
	} else {
		bank = 0;
	}
//	std::cout << std::hex <<  "bank " << (int)bank << ", offset " << (int)ofs << "\n";
	return(true);
}

void SRAM::write(const WORD a_idx, const BYTE value, bool indirect) {
	BYTE bank, ofs;
	if (calc_bank_ofs(a_idx, bank, ofs, indirect)) { // Cannot write invalid addresses
//		eq.queue_event(new DeviceEvent<SRAM>(*this, "SRAM", {bank, ofs, value}));
		m_bank[bank][ofs] = value;
	}
}

const BYTE SRAM::read(const WORD a_idx, bool indirect) const {
	BYTE bank, ofs;

	if (!calc_bank_ofs(a_idx, bank, ofs, indirect)) return 0;
	return(m_bank[bank][ofs]);    // Reading from undefined address returns 0
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


