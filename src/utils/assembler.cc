#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <queue>

#include "../cpu_data.h"
#include "assembler.h"
#include "hex.h"
#include "smart_ptr.cc"
#include "utility.h"

bool parse_args(const char *in, std::queue<std::string> &args) {
	char *pt = (char *)in, *sep;

	for (; *pt && *pt <= 32; ++pt);  // find first non ws
	while (*pt) {                                // We still have input, not ws
		if (*pt == '"') {
			for (sep = pt+1; *sep && *sep != '"'; ++sep);
			if (*sep != '"') return false;
			args.push(std::string(pt+1, sep-pt-1));
//			std::cout << "text: [" << std::string(pt+1, sep-pt-1) << "]" << std::endl;
			pt = sep+1;
		} else if (*pt == ',') {
			for (sep = pt+1; *sep > 32 && strchr("\",", *sep)==NULL; ++sep);  // find first ws or separator
			if (sep - pt > 1) {
				args.push(to_upper(std::string(pt+1, sep-pt-1)));
//				std::cout << "comma: [" << std::string(pt+1, sep-pt-1) << "]" << std::endl;
			}
			pt = sep;
		} else {   // white space; last arg
			for (sep = pt+1; *sep > 32 && strchr("\",", *sep)==NULL; ++sep);  // find first ws or separator
			args.push(to_upper(std::string(pt, sep-pt)));
//			std::cout << "non-separator: [" << std::string(pt, sep-pt) << "]" << std::endl;
			pt = sep;
		}
		for (; *pt && *pt <= 32; ++pt);  // find first non ws
	}
	return true;
}


/*
 *   translate=[whitespace]<input_line><newline>
 *   input_line = <comment> | <instruction_line>
 *   comment = <;>[ignored text]
 *   newline = <chr(10)> | <chr(13)> | <chr(10)><chr(13)>
 *   instruction_line = [label_part][whitespace]<instruction>[whitespace][comment]
 *   label_part = <label><:>
 *   instruction = <mnemonic><whitespace><address_part>
 *   address_part = <address>[,<arg>]
 *
 */
bool translate(const char *in,
		std::string &label,  std::string &mnemonic, std::string &address, std::queue<std::string> &args) {
	char *pt = (char *)in, *sep;

	while (*pt && *pt <= 32) pt++;             // white space skip
	if (*pt) {                                 // found something
		if (*pt==';') return false;            // just a comment line

	} else {
		return false;                          // blank line
	}

	if ((sep = strchr(pt, ':')) != NULL) {
		label = to_upper(std::string(pt, sep-pt)); pt = sep+1;
	}
	while (*pt && *pt <= 32) pt++;             // white space skip
	if (*pt) {
		if (*pt==';') return true;             // comment and label
	} else {
		return true;                           // just a label on a line.
	}
	for (sep=pt; *sep && *sep > 32; ++sep);    // look for next whitespace
	if (sep > pt) {                            // either ws or zero terminator
		mnemonic = std::string(pt, sep-pt);
		pt = sep+1;

		while (*pt && *pt <= 32) pt++;         // white space skip
		if (*pt) {
			if (*pt == ';') return true;       // a comment
			for (sep=pt; *sep && *sep != ';'; ++sep);  // find a comment or end of line

			if (!parse_args(std::string(pt, sep-pt).c_str(), args)) return false;
		}
		if (args.size()) {
			address = args.front(); args.pop();
		}
		return true;
	}
	return false;
}


bool assemble(const std::string &a_filename, CPU_DATA &cpu, InstructionSet &instructions) {   // assembly file read into flash
	// expected format:
	//     [label:] mnemonic [args] [; comments] \n
	//
	// <label> may be preceded by whitespace, and may not be a number or hex
	// number.  If it is, it will be ignored.
	// An instruction may reference a label; for example, GOTO mylabel
	// <mnemonic> may be preceded by whitespace.
	//
	// The W or Flag register destination argument is separated by a comma and
	// is the small letter 'w' or 'f'.  For example:  "ADDWF  0x33,f".
	// Instead of ,w or ,f, ,0 or ,1 are also accepted.
	//
	// It is valid to use register names instead of values.  Eg. BTS TRISA,2
	// anything on a line after a semicolon is considered to be a comment
	//
	// Case is not important. XORWF STATUS,w is equivalent to xorwf StaTus, W
	//
	// Compiler directives ORG, CONFIG, EQU, DATA, EEORG, DE, DT,
	// and RADIX are implemented.
	// ORG  sets the PC to the specified location, and assembly continues from
	//      that location.  Subsequent DATA directives will use and increment
	//      PC.
	// DATA assembles a [12/14/16] bit word at that point, effectively hard coding
	//       the flash.  this may not be particularly useful.
	// DT   directives assemble a byte of data encoded inside a RETLW instruction
	// EEORG sets the current location for writing eeprom data
	// DE writes bytes directly into eeprom, starting from EEORG, and incrementing
	//    an internal counter for each byte written.
	// EQU  allows definition of a variable. eg:
	//        VARNAME: EQU Value
	//      In this case, VARNAME will not be a label, but instead be a reference
	//      which may be used to define a named reference to be used elsewhere.
	// CONFIG may be used to set the configuration word.
	// RADIX may be used to change the current radix for numbers. For example,
	//         RADIX hex
	//         RADIX dec
	//         RADIX 10
	//         RADIX 16

	FILE *f = fopen(a_filename.c_str(), "r");
	char buf[256];
	std::map<std::string, WORD> labels;
	std::map<std::string, std::string> variables;
	WORD PC = 0;
	WORD EEC = 0;
	int radix = 16;
	bool directive_skip = false;

	for (int pass=0; pass<2; ++pass) {
		PC = 0;
		fseek(f, 0, SEEK_SET);
		if (pass)  // clear flash on second pass
			memset(cpu.flash.data, 0, sizeof(cpu.flash.data));
		while (fgets(buf, sizeof(buf), f)) {
			std::string label, mnemonic, address;
			std::queue<std::string>args;
			WORD waddr=0;
			BYTE warg=0;
			bool to_file=false;
			directive_skip = false;
			std::map<std::string, SmartPtr<Register> >::iterator found_register;

			if (PC > cpu.flash.size())
				throw(std::string("PC exceeds device limits: @") + int_to_hex(PC));
			if (EEC > cpu.eeprom.size())
				throw(std::string("EEC exceeds device limits: @") + int_to_hex(EEC));

			if (translate(buf, label, mnemonic, address, args)) {
//					std::cout << "label: ["<<label<<"]; mnemonic: ["<<mnemonic<<"]; address: ["<<address<<"]; arg: ["<<arg<<"]\n";
				label = to_upper(label);
				mnemonic = to_upper(mnemonic);
				address = to_upper(address);
				if (mnemonic.length()) {
					directive_skip = true;
					if (mnemonic == "ORG") {  // set PC
						char *p;
						PC = strtoul(address.c_str(), &p, 10);
						if (!p) throw(std::string("Invalid ORG directive: [")+address+"] @"+int_to_hex(PC));
					} else if (mnemonic == "EEORG") {  // set EEC
							char *p;
							EEC = strtoul(address.c_str(), &p, 10);
							if (!p) throw(std::string("Invalid EEORG directive: [")+address+"] @"+int_to_hex(PC));
					} else if (mnemonic == "DT") {  // compile bytes as a RETLW table, with each byte contained in an instruction

					} else if (mnemonic == "DE") {     // compile bytes of eeprom data

					} else if (mnemonic == "DATA") {   // Create numeric & text data in flash memory

					} else if (mnemonic == "EQU") {
						if (cpu.Registers.find(address)!=cpu.Registers.end())
							throw(std::string("Invalid EQU directive: Cannot redefine register name [")+address+"] @"+int_to_hex(PC));
						if (variables.find(label) != variables.end())
							throw(std::string("Invalid EQU directive: Cannot redefine existing variable [")+address+"] @"+int_to_hex(PC));
						if (label.length() == 0)
							throw(std::string("Invalid EQU directive: A label must be specified [")+address+"] @"+int_to_hex(PC));
						if (address.length() == 0)
							throw(std::string("Invalid EQU directive: Must have an address [")+address+"] @"+int_to_hex(PC));
						variables[label] = address;
					} else if (mnemonic == "CONFIG" || mnemonic == "__CONFIG") {
						char *p;
						cpu.Config = strtoul(address.c_str(), &p, radix);
						if (!p) throw(std::string("Invalid CONFIG directive: [")+address+"] @"+int_to_hex(PC));
					} else if (mnemonic == "RADIX") {
						if (is_decimal(address)) {
							char *p;
							radix = strtoul(address.c_str(), &p, 10);
						}
						else if (address == "HEX")
							radix = 16;
						else if (address.substr(3) == "DEC")
							radix = 10;
						else
							throw(std::string("Invalid RADIX directive: [")+address+"] @"+int_to_hex(PC));
					} else if (address.length()) {
						directive_skip = false;
						found_register = cpu.Registers.end();

						if (label.length()) {
							if (!(is_decimal(label) || is_hex(label))) {    // Not a real label
								if (labels.find(to_upper(label))==labels.end()) {
									labels[label] = PC;  // record current address as a label
								} else if (!pass) {
									throw(std::string("Format error: Non-unique label: ")+label);
								}
							}
						}

						if (variables.find(address) != variables.end())
							address = variables[address];

						if (address.substr(0,2) == "0X") { // a Hex address
							char *p;
							waddr = strtoul(address.c_str()+2, &p, 16);
							if (!p) throw(std::string("Invalid hex digits found: [")+address+"] @"+int_to_hex(PC));
						} else if (radix==10 && is_decimal(address)) {   // its a decimal number
							char *p;
							waddr = strtoul(address.c_str()+2, &p, radix);
							if (!p) throw(std::string("Invalid decimal digits found: [")+address+"] @"+int_to_hex(PC));
						} else if (radix==16 && is_hex(address)) {   // its a decimal number
							char *p;
							waddr = strtoul(address.c_str(), &p, 16);
							if (!p) throw(std::string("Invalid hex digits found: [")+address+"] @"+int_to_hex(PC));
						} else {   // a label or file register name

							if (labels.find(address) != labels.end()) {
								waddr = labels[address];
							} else {
								found_register = cpu.Registers.find(address);
								if (found_register==cpu.Registers.end())
									throw(std::string("Unknown file register name [")+address+"] @"+int_to_hex(PC));
								waddr = cpu.Registers[address]->index();
							}
						}
						if (args.size()) {
							std::string arg = args.front();  args.pop();
							if (arg=="1" || arg=="F") {
								warg = 1;
								to_file = true;
							} else if (arg=="0" || arg=="W") {
								warg = 0;
								to_file = false;
							} else {
								if (is_decimal(arg.c_str())) {
									char *p;
									warg = strtoul(arg.c_str(), &p, 10);
									if (!p) throw(std::string("Invalid argument value: [")+arg+"] @"+int_to_hex(PC));
								} else if (found_register != cpu.Registers.end()) {
									if (!Flags::bit_number_for_bitname(found_register->second->index(), arg, warg)) {
										throw(std::string("Bit name: [" + arg + "] does not apply to register [") + buf + "] @"+int_to_hex(PC));
									}
								}
							}
						}
					} else {  // a mnemonic with no address or arg
						directive_skip = false;
					}
					if (!directive_skip) {
						if (pass) { // write flash on second pass
							WORD opcode = instructions.assemble(mnemonic, waddr, warg, to_file);
							SmartPtr<Instruction> op = instructions.find(opcode);   // check
							if (!op || op->mnemonic != mnemonic) throw(std::string("Error while checking assembly: ") + mnemonic);
							cpu.flash.data[PC] = opcode;
						}
						++PC;  // all instructions are single word
					}
				} else {
					throw(std::string("Problem decoding line: No mnemonic in [") + buf + "] @"+int_to_hex(PC));
				}
			} else {
				std::cout << std::string("Cannot decode assembly line: ") + buf + " @"+int_to_hex(PC) << std::endl;
			}
		}
	}
	return true;
}

void disassemble(CPU_DATA &cpu, InstructionSet &instructions, std::vector<Disasm> &listing) {
	int limit = FLASH_SIZE;
	while (limit>0 && cpu.flash.data[limit-1]==0) --limit;
	for (int pc=0; pc < limit; ++pc) {
		WORD opcode = cpu.flash.data[pc];
		SmartPtr<Instruction> op = instructions.find(opcode);
		listing.push_back(Disasm(pc, opcode, op->disasm(opcode, cpu)));
	}
}

void disassemble(CPU_DATA &cpu, InstructionSet &instructions) {
	int limit = FLASH_SIZE;
	while (limit>0 && cpu.flash.data[limit-1]==0) --limit;
	for (int pc=0; pc < limit; ++pc) {
		WORD opcode = cpu.flash.data[pc];
		SmartPtr<Instruction> op = instructions.find(opcode);

		std::cout <<  std::setfill('0') << std::hex << std::setw(4) << pc << ":\t" << op->disasm(opcode, cpu) << "\n";
	}
}

void disassemble(const std::string &a_filename, CPU_DATA &cpu, InstructionSet &instructions) {
	load_hex(a_filename, cpu);
	disassemble(cpu, instructions);
}

#ifdef TESTING
#include <cassert>
void test_assembler_parse_args() {
	std::queue<std::string> args;

	assert(args.size() == 0);
	args.push("testing");
	assert(args.size() == 1);
	assert(args.front()=="testing"); args.pop();
	assert(args.size() == 0);

	parse_args("   testing        ", args);
	assert(args.size() == 1);
	assert(args.front()=="TESTING"); args.pop();
	assert(args.size() == 0);

	parse_args(" 1, \"2\", 3, \"An argument with Spaces\" ", args);

	assert(args.size() == 4);
	assert(args.front()=="1"); args.pop();
	assert(args.front()=="2"); args.pop();
	assert(args.front()=="3"); args.pop();
	assert(args.front()=="An argument with Spaces"); args.pop();

	std::cout << "Argument parsing tests all ok" << std::endl;
}

#endif
