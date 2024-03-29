#include "constants.h"
#include "flags.h"
#include "sram.h"


const std::vector<std::string> Flags::CONFIG::bits({"FOSC0","FOSC1","WDTE","PWRTE","FOSC2","MCLRE","BOREN","LVP","CPD","","","","CP"});
const std::vector<std::string> Flags::STATUS::bits({"C","DC","Z","PD","TO","RP0","RP1","IRP"});
const std::vector<std::string> Flags::OPTION::bits({"PS0","PS1","PS2","PSA","T0SE","T0CS","INTEDG","RBPU"});
const std::vector<std::string> Flags::TRISA::bits({"TRISA0","TRISA1","TRISA2","TRISA3","TRISA4","TRISA5","TRISA6","TRISA7"});
const std::vector<std::string> Flags::TRISB::bits({"TRISB0","TRISB1","TRISB2","TRISB3","TRISB4","TRISB5","TRISB6","TRISB7"});
const std::vector<std::string> Flags::INTCON::bits({"RBIF","INTF","T0IF","RBIE","INTE","T0IE","PEIE","GIE"});
const std::vector<std::string> Flags::PIE1::bits({"TMR1IE","TMR2IE","CCP1IE","","TXIE","RCIE","CMIE","EEIE"});
const std::vector<std::string> Flags::PIR1::bits({"TMR1IF","TMR2IF","CCP1IF","","TXIF","RCIF","CMIF","EEIF"});
const std::vector<std::string> Flags::PCON::bits({"BOR","POR","","OSCF","","","",""});
const std::vector<std::string> Flags::RCSTA::bits({"RX9D","OERR","FERR","ADEN","CREN","SREN","RX9","SPEN"});
const std::vector<std::string> Flags::TXSTA::bits({"TX9D","TRMT","BRGH","","SYNC","TXEN","TX9","CSRC"});
const std::vector<std::string> Flags::EECON1::bits({"RD","WR","WREN","WRERR","","","",""});
const std::vector<std::string> Flags::CMCON::bits({"CM0","CM1","CM2","CIS","C1INV","C2INV","C1OUT","C2OUT"});
const std::vector<std::string> Flags::VRCON::bits({"VR0","VR1","VR2","VR3","","VRR","VROE","VREN"});
const std::vector<std::string> Flags::T1CON::bits({"TMR1ON","TMR1CS","T1SYNC","T1OSCEN","T1CKPS0","T1CKPS1","",""});
const std::vector<std::string> Flags::T2CON::bits({"T2CKPS0","T2CKPS1","TMR2ON","TOUTPS0","TOUTPS1","TOUTPS2","TOUTPS3",""});
const std::vector<std::string> Flags::PORTA::bits({"RA0","RA1","RA2","RA3","RA4","RA5","RA6","RA7"});
const std::vector<std::string> Flags::PORTB::bits({"RB0","RB1","RB2","RB3","RB4","RB5","RB6","RB7"});

const std::map<const WORD, const std::vector<std::string> > Flags::register_bits = {
		{ (WORD)SRAM::STATUS, Flags::STATUS::bits },
		{ (WORD)SRAM::OPTION, Flags::OPTION::bits },
		{ (WORD)SRAM::TRISA,  Flags::TRISA::bits },
		{ (WORD)SRAM::TRISB,  Flags::TRISB::bits },
		{ (WORD)SRAM::INTCON, Flags::INTCON::bits },
		{ (WORD)SRAM::PIE1,   Flags::PIE1::bits },
		{ (WORD)SRAM::PIR1,   Flags::PIR1::bits },
		{ (WORD)SRAM::PCON,   Flags::PCON::bits },
		{ (WORD)SRAM::RCSTA,  Flags::RCSTA::bits },
		{ (WORD)SRAM::TXSTA,  Flags::TXSTA::bits },
		{ (WORD)SRAM::EECON1, Flags::EECON1::bits },
		{ (WORD)SRAM::CMCON,  Flags::CMCON::bits },
		{ (WORD)SRAM::VRCON,  Flags::VRCON::bits },
		{ (WORD)SRAM::T1CON,  Flags::T1CON::bits },
		{ (WORD)SRAM::T2CON,  Flags::T2CON::bits },
		{ (WORD)SRAM::PORTA,  Flags::PORTA::bits },
		{ (WORD)SRAM::PORTB,  Flags::PORTB::bits }
};

const std::vector<std::string> bitnames_for_register(WORD index) {
	std::vector<std::string> empty({});

	std::map<const WORD, const std::vector<std::string> >::const_iterator all_register_bits = Flags::register_bits.find(index);
	if (all_register_bits == Flags::register_bits.end())
		return empty;
	return all_register_bits->second;
}

bool Flags::bit_number_for_bitname(WORD register_index, const std::string & bit_name, BYTE &bit_number) {          // false if not found
	const std::vector<std::string> &bit_names = bitnames_for_register(register_index);
	for (WORD n=0; n < bit_names.size(); ++n)
		if (bit_names[n] == bit_name) {
			bit_number = n; return true;
		}
	return false;
}

const std::string Flags::bit_name_for_register_bit(WORD register_index, BYTE bit_number) {  // empty string if not found
	const std::vector<std::string> &bit_names = bitnames_for_register(register_index);
	if (bit_names.size() > bit_number) {
		return bit_names[bit_number];
	}
	return "";
}
