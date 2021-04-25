#pragma once

#include <vector>
#include "../cpu_data.h"
#include "../instructions.h"

bool assemble(const std::string &a_filename, CPU_DATA &cpu, InstructionSet &instructions);

class Disasm {
  public:
	WORD PC;
	WORD opcode;
	std::string astext;

	Disasm(WORD a_pc, WORD a_opcode, const std::string &a_astext) :
		PC(a_pc), opcode(a_opcode), astext(a_astext) {}
};

void disassemble(CPU_DATA &cpu, InstructionSet &instructions, std::vector<Disasm> &listing);
void disassemble(CPU_DATA &cpu, InstructionSet &instructions);
void disassemble(const std::string &a_filename, CPU_DATA &cpu, InstructionSet &instructions);

