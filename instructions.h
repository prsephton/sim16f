#include <vector>
#include <string>
#include <map>

#include "constants.h"
#include "cpu_data.h"
#include "smart_ptr.h"

//___________________________________________________________________________________
// A CPU instruction.
class Instruction {
public:
	WORD opcode:14;
	BYTE bits;
	std::string mnemonic;
	std::string description;
	char cycles;
	Instruction(
			WORD a_opcode, BYTE a_bits, BYTE a_cycles,
			const std::string &a_mnemonic, const std::string & a_description):
		opcode(a_opcode), bits(a_bits), mnemonic(a_mnemonic), description(a_description), cycles(a_cycles) {
		description = description + "                                         ";
		description.resize(25);
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
		std::string mnemonic;
	} tree_type;
	tree_type m_tree;

	void add_tree(tree_type &a_tree, const std::string &a_mnemonic, short a_bits, WORD a_opcode);
	const std::string &find_tree(const tree_type &a_tree, WORD a_opcode);

  public:
	InstructionSet();
	SmartPtr<Instruction>find(WORD opcode);
	WORD assemble(const std::string &mnemonic, WORD f, WORD b, bool d);
};

