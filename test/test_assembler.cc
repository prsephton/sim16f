#ifdef TESTING

#include <iostream>
#include <iomanip>
#include <cassert>
#include "run_tests.h"
#include "../src/utils/assembler.h"

namespace Tests {
	void test_assembler() {
		test_assembler_parse_args();
		CPU_DATA cpu;
		InstructionSet instructions;
		cpu.set_params(Params{"PIC16f628a", 2048, 128, 4, 0x80, 18, 8});

		FILE *f = fopen("assembler_test.a", "w+");
		fputs("\tradix hex\n", f);
		fputs("\tconfig 3ff1\n", f);
		fputs("\torg 0\n", f);
		fputs("\tmovlw 0\n", f);
		fputs("\tbsf status, 5\n", f);
		fputs("\tmovwf trisb\n", f);
		fputs("\tbcf status, RP0\n", f);
		fputs("\tmovlw 0f\n", f);
		fputs("\tmovwf portb\n", f);
		fputs("circle:\tgoto circle\n", f);
		fclose(f);
		try {
			assemble("assembler_test.a", cpu, instructions);
		} catch(const std::string &err) {
			std::cout << "Error in assembly: " << err;
		}
		assert(cpu.Config == 0x3ff1);
		std::vector<Disasm> listing;
		disassemble(cpu, instructions, listing);
		assert(listing.size() == 7);
		assert(listing[0].opcode == 0x3000);
		assert(listing[1].opcode == 0x1683);
		assert(listing[2].opcode == 0x86);
		assert(listing[3].opcode == 0x1283);
		assert(listing[4].opcode == 0x300f);
		assert(listing[5].opcode == 0x86);
		assert(listing[6].opcode == 0x2806);
		for (auto d: listing) {
			std::cout << d.PC << ": " << d.astext << " [" << std::hex << d.opcode << "]" << std::endl;
		}
	}
}
#endif

