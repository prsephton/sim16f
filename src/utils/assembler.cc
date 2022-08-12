#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>

#include "../cpu_data.h"
#include "assembler.h"
#include "hex.h"
#include "smart_ptr.cc"
#include "utility.h"

bool translate(const char *in,
		std::string &label,  std::string &mnemonic, std::string &address, std::string &arg) {
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
		while (*pt && *pt <= 32) pt++;                 // white space skip
		if (*pt) {                                     // found something
			if (*pt == ';') return true;               // a comment
			if ((sep = strchr(pt, ','))!=NULL) {
				address = to_upper(std::string(pt, sep-pt)); pt = sep+1;
				if (*pt == ';') return false;              // a comment
				if (*pt == 0) return false;                // no arg
				for (sep=pt; *sep && *sep > 32; ++sep);    // look for next whitespace
				arg = to_upper(std::string(pt, sep-pt));
				return true;
			} else {  // no arg
				for (sep=pt; *sep && *sep > 32; ++sep);    // look for next whitespace
				if (sep > pt) {
					address = to_upper(std::string(pt, sep-pt));
					arg = "";
					return true;
				}
			}
		}
	}
	return false;
}


bool assemble(const std::string &a_filename, CPU_DATA &cpu, InstructionSet &instructions) {   // assembly file read into flash
	// expected format:
	//     [label:] mnemonic [args] [; comments] \n
	//
	// <label> may be on the preceding line. and may be preceded by whitespace.
	// <label> may not be a number or hex number.  If it is, it will be ignored.
	// In an instruction, prefix a label with ':'; for example, GOTO :mylabel
	// <mnemonic> may be preceded by whitespace.
	// The W or Flag register destination argument is separated by a comma and
	// is the small letter 'w' or 'f'.  For example:  "ADDWF  0x33,f".
	// Instead of ,w or ,f, ,0 or ,1 are also accepted.
	// It is valid to use register names instead of values.  Eg. BTS TRISA,2
	// anything on a line after a semicolon is considered to be a comment
	// Case is not important. XORWF STATUS,w is equivalent to xorwf StaTus, W

	FILE *f = fopen(a_filename.c_str(), "r");
	char buf[256];
	WORD PC = 0;
	std::map<std::string, WORD> labels;

	for (int pass=0; pass<2; ++pass) {
		PC = 0;
		fseek(f, 0, SEEK_SET);
		if (pass)  // clear flash on second pass
			memset(cpu.flash.data, 0, sizeof(cpu.flash.data));
		while (fgets(buf, sizeof(buf), f)) {
			std::string label, mnemonic, address, arg;
			WORD waddr=0;
			BYTE warg=0;
			bool to_file=false;


			if (translate(buf, label, mnemonic, address, arg)) {
//					std::cout << "label: ["<<label<<"]; mnemonic: ["<<mnemonic<<"]; address: ["<<address<<"]; arg: ["<<arg<<"]\n";
				label = to_upper(label);
				mnemonic = to_upper(mnemonic);
				address = to_upper(address);
				arg = to_upper(arg);
				if (label.length()) {
					if (!(is_decimal(label) || is_hex(label))) {    // Not a real label
						if (labels.find(to_upper(label))==labels.end()) {
							labels[label] = PC;  // record current address as a label
						} else if (!pass) {
							throw(std::string("Format error: Non-unique label: ")+label);
						}
					}
				}
				if (mnemonic.length()) {
					if (address.length()) {
						if (address.substr(0,2) == "0X") { // a Hex address
							char *p;
							waddr = strtoul(address.c_str()+2, &p, 16);
							if (!p) throw(std::string("Invalid hex digits found: [")+address+"] @"+int_to_hex(PC));
						} else {   // a label or file register name
							if (address[0]==':') {
								waddr = 0;
								if (labels.find(label)==labels.end()) {  // should be seen in pass 2
									if (pass) throw(std::string("Unknown label referenced: ") + label + " @"+int_to_hex(PC));
								} else {
									waddr = labels[label];
								}
							} else {
								if (cpu.Registers.find(address)==cpu.Registers.end())
									throw(std::string("Unknown file register name [")+address+"] @"+int_to_hex(PC));
								waddr = cpu.Registers[address]->index();
							}
						}
						if (arg.length()) {
							if (arg=="1" || arg=="F") {
								warg = 1;
								to_file = true;
							} else if (arg=="0" || arg=="W") {
								warg = 0;
								to_file = false;
							} else {
								char *p;
								warg = strtoul(arg.c_str(), &p, 10);
								if (!p) throw(std::string("Invalid argument value: [")+arg+"] @"+int_to_hex(PC));
							}
						}
					}
					if (pass) { // write flash on second pass
						WORD opcode = instructions.assemble(mnemonic, waddr, warg, to_file);
						SmartPtr<Instruction> op = instructions.find(opcode);   // check
						if (!op || op->mnemonic != mnemonic) throw(std::string("Error while checking assembly: ") + mnemonic);
						cpu.flash.data[PC] = opcode;
					}
				} else {
					std::cout << "Problem decoding line: " << buf << "\n";
				}
			} else {
				throw(std::string("Cannot decode assembly line: ")+buf);
			}
			++PC;  // all instructions are single word
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
