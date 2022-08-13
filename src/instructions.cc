/*
 * Here we implement each of the CPU instructions, every one as a class specialisation of
 * the 'Instruction' class.  Each instruction initialises itself with an OPCode, the number
 * of identifying bits in the OPCode, the number of clock cycles needed for execution, a
 * mnemonic and a description.
 *
 * The instruction implements methods for assembling binary code, disassembling from binary,
 * and executing the instruction.
 *
 * Executing an instruction performs changes to registers and CPU state.  In the PIC, a
 * status register contains flags such as zero, carry, and so forth which other processors
 * may do differently.
 *
 * Devices are implemented to monitor changes to particular registers, and update state
 * accordingly.  Register changes may be monitored via a publish-subscribe queue.
 *
 * Devices themselves have their own queue; for example, the clock device publishes
 * clock events which are monitored by other devices, and in fact, the CPU implementation
 * itself reacts to clock events by processing instructions read from the flash device.
 *
 * We use a std::map to store instances of instruction classes keyed by mnemonic.  This
 * allows us to easily find instructions while assembling binary code from assembly language.
 *
 * For each instruction, we also add its smart pointer to a binary tree, by traversing each
 * bit in it's OPCode until the total number of identifying bits have been visited.  This
 * tree may later be used to locate the appropriate instruction to be executed.
 *
 * To execute instructions, we retrieve the current instruction using the Program Counter,
 * locate the closest instruction using the binary tree by examining each bit in the OpCode
 * from the front, retrieving the appropriate instruction instance, and call it's
 * execute() method.
 *
 * The way we decode instructions by shifting the OPCode left and comparing the value of the
 * top bit approximates the way a real CPU may decode instructions. Of course, things go
 * south rather quickly from there, but we do our best.
 *
 */

#include <iostream>
#include <cstdio>
#include "cpu_data.h"
#include "instructions.h"
#include "utils/smart_ptr.cc"
#include "utils/utility.h"
#include "devices/constants.h"
#include "devices/flags.h"

std::string pad(const std::string &payload) {
	std::string padded = std::string("\t") + payload + "                        ";
	padded.resize(14);
	return padded + "\t; ";
}

const std::string Instruction::disasm(WORD opcode, CPU_DATA &cpu) {
	return mnemonic + pad("") + description;
}

WORD Instruction::assemble(WORD f, BYTE b, bool d) {
	return opcode;  // best effort
}
// "\t" + cpu.register_name(idx) + std::string(to_file?",f":",w")  +"    \t; "

class ADDWF: public Instruction {

  public:
	ADDWF(): Instruction(0b00011100000000, 6, 1, "ADDWF", "Add W and f") {}
	inline void decode(WORD opcode, BYTE &idx, bool &to_file) {
		idx = opcode & 0x7f;
		to_file = opcode & 0x80;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		if (d) f |= 0x80;
		return (opcode & 0x3f00) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		return mnemonic + pad(cpu.register_name(idx) + std::string(to_file?",f":",w")) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		WORD data = cpu.read_sram(idx);
		WORD ldata = (data & 0x0f) + (cpu.W & 0x0f);
		data = data + cpu.W;
		BYTE Z = data==0?Flags::STATUS::Z:0;
		BYTE C = data&0x100?Flags::STATUS::C:0;
		BYTE DC = ldata&0x10?Flags::STATUS::DC:0;

		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::Z | Flags::STATUS::C | Flags::STATUS::DC;

		if (to_file) {
			cpu.write_sram(idx, data & 0xff);
		} else {
			cpu.W = data & 0xff;
		}
		status = (status & ~mask) | Z | C | DC ;
		return false;
	}
};

class ANDWF: public Instruction {
  public:
	ANDWF(): Instruction(0b00010100000000, 6, 1, "ANDWF", "AND W with f") {}
	inline void decode(WORD opcode, BYTE &idx, bool &to_file) {
		idx = opcode & 0x7f;
		to_file = opcode & 0x80;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		if (d) f |= 0x80;
		return (opcode & 0x3f00) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		return mnemonic + pad(cpu.register_name(idx) + std::string(to_file?",f":",w")) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		WORD data = cpu.read_sram(idx);
		data = data & cpu.W;
		BYTE Z = data==0?Flags::STATUS::Z:0;
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::Z;

		if (to_file) {
			cpu.write_sram(idx, data & 0xff);
		} else {
			cpu.W = data & 0xff;
		}
		status = (status & ~mask) | Z;
		return false;
	}
};

class CLRF: public Instruction {
  public:
	CLRF(): Instruction(0b00000110000000, 7, 1, "CLRF", "Clear f") {}
	inline void decode(WORD opcode, BYTE &idx) {
		idx = opcode & 0x7f;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		return (opcode & 0x3f80) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		decode(opcode, idx);
		return mnemonic + pad(cpu.register_name(idx)) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		decode(opcode, idx);
		BYTE Z = true;
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::Z;
		cpu.read_sram(idx);
		cpu.write_sram(idx, (BYTE)0);
		status = (status & ~mask) | Z;
		return false;
	}
};

class CLRW: public Instruction {
  public:
	CLRW(): Instruction(0b00000100000000, 7, 1, "CLRW", "Clear W") {}
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		return opcode;
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		return mnemonic + pad("") + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE Z = true;
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::Z;
		status = (status & ~mask) | Z;
		cpu.W = 0;
		return false;
	}
};

class COMF: public Instruction {
  public:
	COMF(): Instruction(0b00100100000000, 6, 1, "COMF", "Complement f") {}
	inline void decode(WORD opcode, BYTE &idx, bool &to_file) {
		idx = opcode & 0x7f;
		to_file = opcode & 0x80;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		if (d) f |= 0x80;
		return (opcode & 0x3f00) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		return mnemonic + pad(cpu.register_name(idx) + std::string(to_file?",f":",w")) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		WORD data = cpu.read_sram(idx);
		data = ~data;
		BYTE Z = data==0?Flags::STATUS::Z:0;
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::Z;

		if (to_file) {
			cpu.write_sram(idx, data & 0xff);
		} else {
			cpu.W = data & 0xff;
		}
		status = (status & ~mask) | Z;
		return false;
	}
};

class DECF: public Instruction {
  public:
	DECF(): Instruction(0b00001100000000, 6, 1, "DECF", "Decrement f") {}
	inline void decode(WORD opcode, BYTE &idx, bool &to_file) {
		idx = opcode & 0x7f;
		to_file = opcode & 0x80;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		if (d) f |= 0x80;
		return (opcode & 0x3f00) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		return mnemonic + pad(cpu.register_name(idx) + std::string(to_file?",f":",w")) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		WORD data = cpu.read_sram(idx);
		--data;
		BYTE Z = data==0?Flags::STATUS::Z:0;
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::Z;

		if (to_file) {
			cpu.write_sram(idx, data & 0xff);
		} else {
			cpu.W = data & 0xff;
		}
		status = (status & ~mask) | Z;
		return false;
	}
};

class DECFSZ: public Instruction {
  public:
	DECFSZ(): Instruction(0b00101100000000, 6, 1, "DECFSZ", "Decrement f, Skip if 0") {}
	inline void decode(WORD opcode, BYTE &idx, bool &to_file) {
		idx = opcode & 0x7f;
		to_file = opcode & 0x80;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		if (d) f |= 0x80;
		return (opcode & 0x3f00) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		return mnemonic + pad(cpu.register_name(idx) + std::string(to_file?",f":",w")) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		WORD data = cpu.read_sram(idx);
		--data;
		BYTE Z = data==0?Flags::STATUS::Z:0;
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::Z;

		if (to_file) {
			cpu.write_sram(idx, data & 0xff);
		} else {
			cpu.W = data & 0xff;
		}
		status = (status & ~mask) | Z;
		return Z != 0;
	}
};

class INCF: public Instruction {
  public:
	INCF(): Instruction(0b00101000000000, 6, 1, "INCF", "Increment f") {}
	inline void decode(WORD opcode, BYTE &idx, bool &to_file) {
		idx = opcode & 0x7f;
		to_file = (opcode & 0x80) != 0;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		if (d) f |= 0x80;
		return (opcode & 0x3f00) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		return mnemonic + pad(cpu.register_name(idx) + std::string(to_file?",f":",w")) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		WORD data = cpu.read_sram(idx);
		++data;
		BYTE Z = data==0?Flags::STATUS::Z:0;
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::Z;

		if (to_file) {
			cpu.write_sram(idx, data & 0xff);
		} else {
			cpu.W = data & 0xff;
		}
		status = (status & ~mask) | Z;
		return false;
	}
};

class INCFSZ: public Instruction {
  public:
	INCFSZ(): Instruction(0b00111100000000, 6, 1, "INCFSZ", "Increment f, Skip if 0") {}
	inline void decode(WORD opcode, BYTE &idx, bool &to_file) {
		idx = opcode & 0x7f;
		to_file = opcode & 0x80;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		if (d) f |= 0x80;
		return (opcode & 0x3f00) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		return mnemonic + pad(cpu.register_name(idx) + std::string(to_file?",f":",w")) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		WORD data = cpu.read_sram(idx);
		--data;
		BYTE Z = data==0?Flags::STATUS::Z:0;
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::Z;

		if (to_file) {
			cpu.write_sram(idx, data & 0xff);
		} else {
			cpu.W = data & 0xff;
		}
		status = (status & ~mask) | Z;
		return Z != 0;
	}
};

class IORWF: public Instruction {
  public:
	IORWF(): Instruction(0b00010000000000, 6, 1, "IORWF", "Inclusive OR W with f") {}
	inline void decode(WORD opcode, BYTE &idx, bool &to_file) {
		idx = opcode & 0x7f;
		to_file = opcode & 0x80;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		if (d) f |= 0x80;
		return (opcode & 0x3f00) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		return mnemonic + pad(cpu.register_name(idx) + std::string(to_file?",f":",w")) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		WORD data = cpu.read_sram(idx);
		data = data | cpu.W;
		BYTE Z = data==0?Flags::STATUS::Z:0;
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::Z;

		if (to_file) {
			cpu.write_sram(idx, data & 0xff);
		} else {
			cpu.W = data & 0xff;
		}
		status = (status & ~mask) | Z;
		return false;
	}
};


class MOVF: public Instruction {
  public:
	MOVF(): Instruction(0b00100000000000, 6, 1, "MOVF", "Move f") {}
	inline void decode(WORD opcode, BYTE &idx, bool &to_file) {
		idx = opcode & 0x7f;
		to_file = opcode & 0x80;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		if (d) f |= 0x80;
		return (opcode & 0x3f00) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		return mnemonic + pad(cpu.register_name(idx) + std::string(to_file?",f":",w")) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		WORD data = cpu.read_sram(idx);
		BYTE Z = data==0?Flags::STATUS::Z:0;
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::Z;

		if (to_file) {
			cpu.write_sram(idx, data & 0xff);
		} else {
			cpu.W = data & 0xff;
		}
		status = (status & ~mask) | Z;
		return false;
	}
};


class MOVWF: public Instruction {
  public:
	MOVWF(): Instruction(0b00000010000000, 7, 1, "MOVWF", "Move W to f") {}
	inline void decode(WORD opcode, BYTE &idx) {
		idx = opcode & 0x7f;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		return (opcode & 0x3f80) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		decode(opcode, idx);
		return mnemonic + pad(cpu.register_name(idx)) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		decode(opcode, idx);
		WORD data = cpu.W;
		cpu.read_sram(idx);
		cpu.write_sram(idx, data & 0xff);
		return false;
	}
};


class NOP: public Instruction {
  public:
	NOP(): Instruction(0b00000000000000, 14, 1, "NOP", "No Operation") {}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) { return false; }
};


class RLF: public Instruction {
  public:
	RLF(): Instruction(0b00110100000000, 6, 1, "RLF", "Rotate Left f through Carry") {}
	inline void decode(WORD opcode, BYTE &idx, bool &to_file) {
		idx = opcode & 0x7f;
		to_file = opcode & 0x80;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		if (d) f |= 0x80;
		return (opcode & 0x3f00) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		return mnemonic + pad(cpu.register_name(idx) + std::string(to_file?",f":",w")) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		WORD data = cpu.read_sram(idx);
		BYTE &status = cpu.sram.status();
		data = data << 1;
		if (status & Flags::STATUS::C) data |= 1;

		BYTE C = data&0x100?Flags::STATUS::C:0;

		if (to_file) {
			cpu.write_sram(idx, data & 0xff);
		} else {
			cpu.W = data & 0xff;
		}
		status = (status & ~Flags::STATUS::C) | C;
		return false;
	}
};

class RRF: public Instruction {
  public:
	RRF(): Instruction(0b00110000000000, 6, 1, "RRF", "Rotate Right f through Carry") {}
	inline void decode(WORD opcode, BYTE &idx, bool &to_file) {
		idx = opcode & 0x7f;
		to_file = opcode & 0x80;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		if (d) f |= 0x80;
		return (opcode & 0x3f00) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		return mnemonic + pad(cpu.register_name(idx) + std::string(to_file?",f":",w")) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		WORD data = cpu.read_sram(idx);
		BYTE &status = cpu.sram.status();
		BYTE C = data&0x01?Flags::STATUS::C:0;
		data = data >> 1;

		if (status & Flags::STATUS::C) data |= 0x80;

		if (to_file) {
			cpu.write_sram(idx, data & 0xff);
		} else {
			cpu.W = data & 0xff;
		}
		status = (status & ~Flags::STATUS::C) | C;
		return false;
	}
};

class SUBWF: public Instruction {
  public:
	SUBWF(): Instruction(0b00001000000000, 6, 1, "SUBWF", "Subtract W from f") {}
	inline void decode(WORD opcode, BYTE &idx, bool &to_file) {
		idx = opcode & 0x7f;
		to_file = opcode & 0x80;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		if (d) f |= 0x80;
		return (opcode & 0x3f00) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		return mnemonic + pad(cpu.register_name(idx) + std::string(to_file?",f":",w")) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		WORD data = cpu.read_sram(idx);
		bool lborrow = (data & 0x0f) < (cpu.W & 0x0f);
		bool borrow = data < cpu.W;

		data = borrow?0x100 + data - cpu.W:data - cpu.W;

		BYTE Z = data==0?Flags::STATUS::Z:0;
		BYTE C = borrow?Flags::STATUS::C:0;
		BYTE DC = lborrow?Flags::STATUS::DC:0;
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::Z | Flags::STATUS::C | Flags::STATUS::DC;

		if (to_file) {
			cpu.write_sram(idx, data & 0xff);
		} else {
			cpu.W = data & 0xff;
		}
		status = (status & ~mask) | Z | C | DC ;
		return false;
	}
};


class SWAPF: public Instruction {
  public:
	SWAPF(): Instruction(0b00111000000000, 6, 1, "SWAPF", "Swap nibbles in f") {}
	inline void decode(WORD opcode, BYTE &idx, bool &to_file) {
		idx = opcode & 0x7f;
		to_file = opcode & 0x80;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		if (d) f |= 0x80;
		return (opcode & 0x3f00) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		return mnemonic + pad(cpu.register_name(idx) + std::string(to_file?",f":",w")) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		WORD data = cpu.read_sram(idx);
		data = (data << 4) + (data >> 4);

		if (to_file) {
			cpu.write_sram(idx, data & 0xff);
		} else {
			cpu.W = data & 0xff;
		}
		return false;
	}
};

class XORWF: public Instruction {
  public:
	XORWF(): Instruction(0b00011000000000, 6, 1, "XORWF", "Exclusive OR W with f") {}
	inline void decode(WORD opcode, BYTE &idx, bool &to_file) {
		idx = opcode & 0x7f;
		to_file = opcode & 0x80;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		if (d) f |= 0x80;
		return (opcode & 0x3f00) | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		return mnemonic + pad(cpu.register_name(idx) + std::string(to_file?",f":",w")) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		bool to_file;
		decode(opcode, idx, to_file);
		WORD data = cpu.read_sram(idx);
		BYTE &status = cpu.sram.status();
		data = data ^ cpu.W;

		BYTE Z = data==0?Flags::STATUS::Z:0;
		BYTE mask = Flags::STATUS::Z;

		if (to_file) {
			cpu.write_sram(idx, data & 0xff);
		} else {
			cpu.W = data & 0xff;
		}
		status = (status & ~mask) | Z;
		return false;
	}
};

class MOVLW: public Instruction {
  public:
	MOVLW(): Instruction(0b11000000000000, 4, 1, "MOVLW", "Move literal to W") {}
	inline void decode(WORD opcode, BYTE &literal) {
		literal = opcode & 0xff;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		return opcode | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE literal;
		decode(opcode, literal);
		return mnemonic + pad(int_to_hex(literal)) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE literal;
		decode(opcode, literal);
		cpu.W = literal;
		return false;
	}
};

class RETLW: public Instruction {
  public:
	RETLW(): Instruction(0b11010000000000, 4, 2, "RETLW", "Return with literal in W") {}
	inline void decode(WORD opcode, BYTE &literal) {
		literal = opcode & 0xff;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		return opcode | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE literal;
		decode(opcode, literal);
		return mnemonic + pad(int_to_hex(literal)) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE literal;
		decode(opcode, literal);
		cpu.W = literal;
		WORD address = cpu.pop();
		cpu.sram.set_PC(address);
		return false;
	}
};

class SUBLW: public Instruction {
  public:
	SUBLW(): Instruction(0b11110000000000, 5, 1, "SUBLW", "Subtract W from literal") {}
	inline void decode(WORD opcode, BYTE &literal) {
		literal = opcode & 0xff;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		return opcode | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE literal;
		decode(opcode, literal);
		return mnemonic + pad(int_to_hex(literal)) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE literal;
		decode(opcode, literal);

		bool lborrow = (literal & 0x0f) < (cpu.W & 0x0f);
		bool borrow = literal < cpu.W;

		cpu.W = borrow?0x100 + literal - cpu.W:literal - cpu.W;

		BYTE Z = cpu.W==0?Flags::STATUS::Z:0;
		BYTE C = borrow?Flags::STATUS::C:0;
		BYTE DC = lborrow?Flags::STATUS::DC:0;
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::Z | Flags::STATUS::C | Flags::STATUS::DC;

		status = (status & ~mask) | Z | C | DC ;
		return false;
	}
};

class ADDLW: public Instruction {
  public:
	ADDLW(): Instruction(0b11111000000000, 5, 1, "ADDLW", "Add literal and W") {}
	inline void decode(WORD opcode, BYTE &literal) {
		literal = opcode & 0xff;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		return opcode | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE literal;
		decode(opcode, literal);
		return mnemonic + pad(int_to_hex(literal)) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE literal;
		decode(opcode, literal);

		WORD data = (WORD)literal + cpu.W;
		bool dcarry = (literal & 0x0f) + (cpu.W & 0x0f) > 0xf;
		bool carry = (data & 0x100) == 0x100;

		cpu.W = data & 0xff;
		BYTE Z = cpu.W==0?Flags::STATUS::Z:0;
		BYTE C = carry?Flags::STATUS::C:0;
		BYTE DC = dcarry?Flags::STATUS::DC:0;
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::Z | Flags::STATUS::C | Flags::STATUS::DC;
		status = (status & ~mask) | Z | C | DC ;
		return false;
	}
};

class XORLW: public Instruction {
  public:
	XORLW(): Instruction(0b11101000000000, 6, 1, "XORLW", "Exclusive OR literal with W") {}
	inline void decode(WORD opcode, BYTE &literal) {
		literal = opcode & 0xff;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		return opcode | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE literal;
		decode(opcode, literal);
		return mnemonic + pad(int_to_hex(literal)) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE literal;
		decode(opcode, literal);

		cpu.W = literal ^ cpu.W;

		BYTE Z = cpu.W==0?Flags::STATUS::Z:0;
		BYTE &status = cpu.sram.status();
		status = (status & ~Flags::STATUS::Z) | Z;
		return false;
	}
};

class IORLW: public Instruction {
  public:
	IORLW(): Instruction(0b11100000000000, 6, 1, "IORLW", "Inclusive OR literal with W") {}
	inline void decode(WORD opcode, BYTE &literal) {
		literal = opcode & 0xff;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		return opcode | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE literal;
		decode(opcode, literal);
		return mnemonic + pad(int_to_hex(literal)) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE literal;
		decode(opcode, literal);

		cpu.W = literal | cpu.W;

		BYTE Z = cpu.W==0?Flags::STATUS::Z:0;
		BYTE &status = cpu.sram.status();
		status = (status & ~Flags::STATUS::Z) | Z;
		return false;
	}
};

class ANDLW: public Instruction {
  public:
	ANDLW(): Instruction(0b11100100000000, 6, 1, "ANDLW", "AND literal with W") {}
	inline void decode(WORD opcode, BYTE &literal) {
		literal = opcode & 0xff;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		return opcode | (f & 0xff);
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE literal;
		decode(opcode, literal);
		return mnemonic + pad(int_to_hex(literal)) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE literal;
		decode(opcode, literal);

		cpu.W = literal & cpu.W;

		BYTE Z = cpu.W==0?Flags::STATUS::Z:0;
		BYTE &status = cpu.sram.status();
		status = (status & ~Flags::STATUS::Z) | Z;
		return false;
	}
};

class BCF: public Instruction {
  public:
	BCF(): Instruction(0b01000000000000, 4, 1, "BCF", "Bit Clear f") {}
	inline void decode(WORD opcode, BYTE &idx, BYTE &cbits) {
		idx   = opcode & 0x7f;
		cbits = (opcode & 0x0380) >> 7;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		f = (WORD)(f & 0xff) | ((WORD)b << 7);
		return opcode | f;
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		BYTE cbits;
		decode(opcode, idx, cbits);
		BYTE bitmask = 1 << cbits;
		if (idx == cpu.sram.STATUS && (bitmask == Flags::STATUS::RP0 || bitmask == Flags::STATUS::RP1)) {
			BYTE &status = cpu.sram.status();
			status &=  ~bitmask;
		}
		const std::string bitname = Flags::bit_name_for_register_bit(idx, cbits);
		return mnemonic + pad(cpu.register_name(idx) + "," + (bitname.length()?bitname:int_to_string(cbits))) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		BYTE cbits;
		decode(opcode, idx, cbits);
		cbits = 1 << cbits;
		BYTE data = cpu.read_sram(idx);
		data = data & ~cbits;
		cpu.write_sram(idx, data);
		return false;
	}
};

class BSF: public Instruction {
  public:
	BSF(): Instruction(0b01010000000000, 4, 1, "BSF", "Bit Set f") {}
	inline void decode(WORD opcode, BYTE &idx, BYTE &cbits) {
		idx   = opcode & 0x7f;
		cbits = (opcode & 0x0380) >> 7;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		f = (f & 0xff) | (b << 7);
		return opcode | f;
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		BYTE cbits;
		decode(opcode, idx, cbits);
		BYTE bitmask = 1 << cbits;
		if (idx == cpu.sram.STATUS && (bitmask == Flags::STATUS::RP0 || bitmask == Flags::STATUS::RP1)) {
			BYTE &status = cpu.sram.status();
			status |= bitmask;
		}
		const std::string bitname = Flags::bit_name_for_register_bit(idx, cbits);
		return mnemonic + pad(cpu.register_name(idx) + "," + (bitname.length()?bitname:int_to_string(cbits))) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		BYTE cbits;
		decode(opcode, idx, cbits);
		cbits = 1 << cbits;
		WORD data = cpu.read_sram(idx);
		data = data | cbits;
		cpu.write_sram(idx, data);
		return false;
	}
};

class BTFSC: public Instruction {
  public:
	BTFSC(): Instruction(0b01100000000000, 4, 1, "BTFSC", "Bit Test f, Skip if Clear") {}
	inline void decode(WORD opcode, BYTE &idx, BYTE &cbits) {
		idx   = opcode & 0x7f;
		cbits = (opcode & 0x0380) >> 7;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		f = (f & 0xff) | (b << 7);
		return opcode | f;
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		BYTE cbits;
		decode(opcode, idx, cbits);
		const std::string bitname = Flags::bit_name_for_register_bit(idx, cbits);
		return mnemonic + pad(cpu.register_name(idx) + "," + (bitname.length()?bitname:int_to_string(cbits))) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		BYTE cbits;
		decode(opcode, idx, cbits);
		cbits = 1 << cbits;
		WORD data = cpu.read_sram(idx);
		return (data & cbits) == 0;
	}
};

class BTFSS: public Instruction {
  public:
	BTFSS(): Instruction(0b01110000000000, 4, 1, "BTFSS", "Bit Test f, Skip if Set") {}
	inline void decode(WORD opcode, BYTE &idx, BYTE &cbits) {
		idx   = opcode & 0x7f;
		cbits = (opcode & 0x0380) >> 7;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		f = (f & 0xff) | (b << 7);
		return opcode | f;
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		BYTE cbits;
		decode(opcode, idx, cbits);
		const std::string bitname = Flags::bit_name_for_register_bit(idx, cbits);
		return mnemonic + pad(cpu.register_name(idx) + "," + (bitname.length()?bitname:int_to_string(cbits))) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		BYTE idx;
		BYTE cbits;
		decode(opcode, idx, cbits);
		cbits = 1 << cbits;
		WORD data = cpu.read_sram(idx);
		return (data & cbits) != 0;
	}
};


class CALL: public Instruction {
  public:
	CALL(): Instruction(0b10000000000000, 3, 2, "CALL", "Call subroutine") {}
	inline void decode(WORD opcode, WORD &address) {
		address = opcode & 0x7ff;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		f = f & 0x7ff;
		return opcode | f;
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		WORD address;
		decode(opcode, address);
		return mnemonic + pad(int_to_hex(address)) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		WORD address;
		decode(opcode, address);
		WORD PC = cpu.sram.get_PC();
		cpu.push(PC);
		cpu.sram.set_PC(address);
		return false;
	}
};

class GOTO: public Instruction {
  public:
	GOTO(): Instruction(0b10100000000000, 3, 2, "GOTO", "Go to address") {}
	inline void decode(WORD opcode, WORD &address) {
		address = opcode & 0x7ff;
	};
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		f = f & 0x7ff;
		return opcode | f;
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		WORD address;
		decode(opcode, address);
		return mnemonic + pad(int_to_hex(address)) + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		WORD address;
		decode(opcode, address);
		cpu.sram.set_PC(address);
		return false;
	}
};

class RETURN: public Instruction {
  public:
	RETURN(): Instruction(0b00000000001000, 14, 2, "RETURN", "Return from Subroutine") {}
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		return opcode;
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		return mnemonic + pad("") + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		WORD address = cpu.pop();
		cpu.sram.set_PC(address);
		return false;
	}
};

class RETFIE: public Instruction {
  public:
	RETFIE(): Instruction(0b00000000001001, 14, 2, "RETFIE", "Return from interrupt") {}
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		return opcode;
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		return mnemonic + pad("") + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		WORD address = cpu.pop();
		cpu.sram.set_PC(address);
		BYTE intcon = cpu.read_sram(cpu.sram.INTCON);
		intcon |= Flags::INTCON::GIE;
		cpu.write_sram(cpu.sram.INTCON, intcon);
		return false;
	}
};

class SLEEP: public Instruction {
  public:
	SLEEP(): Instruction(0b00000001100011, 14, 1, "SLEEP", "Go into Standby mode") {}
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		return opcode;
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		return mnemonic + pad("") + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		// WDT = 0, WDT Prescaler = 0, TO = 1, PD = 0
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::TO | Flags::STATUS::PD;
		status = (status & ~mask) | Flags::STATUS::TO;
		cpu.wdt.sleep();
		return false;
	}
};

class CLRWDT: public Instruction {
  public:
	CLRWDT(): Instruction(0b00000001100100, 14, 1, "CLRWDT", "Clear Watchdog Timer") {}
	virtual WORD assemble(WORD f, BYTE b, bool d) {
		return opcode;
	}
	virtual const std::string disasm(WORD opcode, CPU_DATA &cpu) {
		return mnemonic + pad("") + description;
	}
	virtual bool execute(WORD opcode, CPU_DATA &cpu) {
		// WDT = 0, WDT Prescaler = 0, TO = 1, PD = 1
		BYTE &status = cpu.sram.status();
		BYTE mask = Flags::STATUS::TO | Flags::STATUS::PD;
		status = (status & ~mask) | Flags::STATUS::TO | Flags::STATUS::PD;
		cpu.wdt.clear();
		return false;
	}
};



//______________________________________________________________________________________________________________________
// The instruction set represents the complete set of individual CPU instructions
void InstructionSet::add_tree(struct tree_struct &a_tree, SmartPtr<Instruction> a_instruction, short a_bits, WORD a_opcode) {
	if (!a_bits) {
		if (a_tree.instruction)
			throw(std::string("Mnemonic Operand Clash: " + a_instruction->mnemonic + " redefines " + a_tree.instruction->mnemonic));
		a_tree.instruction = a_instruction;
	} else {
		a_opcode = a_opcode << 1;
		bool rhs = a_opcode & 0x4000;   // test bit 14
		if (rhs) {
			if (!a_tree.right) a_tree.right = new tree_type();
			add_tree(*a_tree.right, a_instruction, --a_bits, a_opcode);
		} else {
			if (!a_tree.left) a_tree.left = new tree_type();
			add_tree(*a_tree.left, a_instruction, --a_bits, a_opcode);
		}
	}
}

SmartPtr<Instruction> InstructionSet::find_tree(const tree_type &a_tree, WORD a_opcode) {
	if (a_tree.instruction) return a_tree.instruction;
	a_opcode = a_opcode << 1;
	bool rhs = a_opcode & 0x4000;   // test bit 14
	if (rhs) {
		if (!a_tree.right) throw(std::string("Invalid OP Code: ") + int_to_hex(a_opcode));
		return find_tree(*a_tree.right, a_opcode);
	} else {
		if (!a_tree.left) throw(std::string("Invalid OP Code: ") + int_to_hex(a_opcode));
		return find_tree(*a_tree.left, a_opcode);
	}
}

SmartPtr<Instruction>InstructionSet::find(WORD opcode) {
	try {
		return find_tree(m_tree, opcode);
	} catch(std::string &error) {
		std::cerr << error << ": " << "\n";
		throw(error);
	}
}

WORD InstructionSet::assemble(const std::string &mnemonic, WORD f, WORD b, bool d) {
	auto instruction = operands.find(mnemonic);
	if ( instruction == operands.end())
		throw(std::string("Invalid OP code in assembly: ") + mnemonic);
	SmartPtr<Instruction>ins = instruction->second;
	return ins->assemble(f, b, d);
}

InstructionSet::InstructionSet() {
	operands["ADDWF"]  = new ADDWF();
	operands["ANDWF"]  = new ANDWF();
	operands["CLRF"]   = new CLRF();
	operands["CLRW"]   = new CLRW();
	operands["COMF"]   = new COMF();
	operands["DECF"]   = new DECF();
	operands["DECFSZ"] = new DECFSZ();
	operands["INCF"]   = new INCF();
	operands["INCFSZ"] = new INCFSZ();
	operands["IORWF"]  = new IORWF();
	operands["MOVF"]   = new MOVF();
	operands["MOVWF"]  = new MOVWF();
	operands["NOP"]    = new NOP();
	operands["RLF"]    = new RLF();
	operands["RRF"]    = new RRF();
	operands["SUBWF"]  = new SUBWF();
	operands["SWAPF"]  = new SWAPF();
	operands["XORWF"]  = new XORWF();

	operands["BCF"]    = new BCF();
	operands["BSF"]    = new BSF();
	operands["BTFSC"]  = new BTFSC();
	operands["BTFSS"]  = new BTFSS();

	operands["CALL"]   = new CALL();
	operands["GOTO"]   = new GOTO();
	operands["MOVLW"]  = new MOVLW();
	operands["RETLW"]  = new RETLW();
	operands["SUBLW"]  = new SUBLW();
	operands["ADDLW"]  = new ADDLW();
	operands["XORLW"]  = new XORLW();
	operands["IORLW"]  = new IORLW();
	operands["ANDLW"]  = new ANDLW();

	operands["RETURN"] = new RETURN();
	operands["RETFIE"] = new RETFIE();
	operands["SLEEP"]  = new SLEEP();
	operands["CLRWDT"] = new CLRWDT();

	for (operand_each i = operands.begin(); i != operands.end(); ++i) {
		add_tree(m_tree, i->second, i->second->bits, i->second->opcode);
	}
	for (operand_each i = operands.begin(); i != operands.end(); ++i) {
		SmartPtr<Instruction>ins = find(i->second->opcode);
		if (ins->mnemonic != i->first) {
			throw(std::string("Mnemonic ") + i->first + " not correctly indexed.  Returns " + ins->mnemonic + " on find().");
		}
	}
}
