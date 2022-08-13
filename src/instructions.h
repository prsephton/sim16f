/*
 * A single CPU instruction is defined by a 14 bit OP code.  When executed, the instruction
 * performs a change to CPU status flags, data or registers.  A full set of registers is
 * found in cpu_data.cc and cpu_data.h.  Status flags are themselves found in register 0x03,
 * and reflect the result of various operations.  Program flow is often controlled by these
 * flags, for example to branch depending on the value of the zero flag.
 *
 * The whole instruction set is represented as a tree.  The binary representation of an OP code
 * in a real processor maps directly to the operation being performed.   Here we simulate that
 * mapping by traversing each bit in the OP code to locate an object in memory which represents
 * the appropriate instruction, and then executing that instruction.
 *
 * We include methods here for assembling and disassembling instructions to and from assembler
 * or binary code.
 *
 */
#ifndef __instructions_h__
#define __instructions_h__
#include <vector>
#include <string>
#include <map>

#include "utils/smart_ptr.h"
#include "devices/constants.h"
#include "cpu_data.h"

//___________________________________________________________________________________
// A CPU instruction.
class Instruction {
public:
	WORD opcode:14;                  // The OP Code
	BYTE bits;                       // The number of identifying bits in the OP Code
	std::string mnemonic;            // Instruction mnemonic
	std::string description;         // A description for the instruction
	char cycles;                     // Clock cycles required for this instruction
	Instruction(
			WORD a_opcode, BYTE a_bits, BYTE a_cycles,
			const std::string &a_mnemonic, const std::string & a_description):
		opcode(a_opcode), bits(a_bits), mnemonic(a_mnemonic), description(a_description), cycles(a_cycles) {
		description = description + "                                         ";
		description.resize(27);
	}
	virtual ~Instruction() {}

	virtual bool execute(WORD opcode, CPU_DATA &cpu){ throw(std::string("Unimplemented Instruction")); }
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu);
	virtual WORD assemble(WORD f, BYTE b, bool d);
	bool flush() { return(cycles > 1); }
};


//___________________________________________________________________________________
// Represents the instruction set, and a way to locate an instruction from an opcode.
class InstructionSet {
	std::map<std::string, SmartPtr<Instruction> > operands;
	typedef std::map<std::string, SmartPtr<Instruction> >::iterator operand_each;


	typedef struct tree_struct {
		SmartPtr<struct tree_struct> left;
		SmartPtr<struct tree_struct> right;
		SmartPtr<Instruction> instruction;
	} tree_type;
	tree_type m_tree;

	void add_tree(tree_type &a_tree, SmartPtr<Instruction> a_instruction, short a_bits, WORD a_opcode);
	SmartPtr<Instruction> find_tree(const tree_type &a_tree, WORD a_opcode);

  public:
	InstructionSet();
	SmartPtr<Instruction>find(WORD opcode);
	WORD assemble(const std::string &mnemonic, WORD f, WORD b, bool d);
};

#endif
