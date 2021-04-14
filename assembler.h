#ifndef __assembler_h__
#define __assembler_h__

#include "cpu_data.h"
#include "instructions.h"

bool assemble(const std::string &a_filename, CPU_DATA &cpu, InstructionSet &instructions);
void disassemble(CPU_DATA &cpu, InstructionSet &instructions);
void disassemble(const std::string &a_filename, CPU_DATA &cpu, InstructionSet &instructions);

#endif
