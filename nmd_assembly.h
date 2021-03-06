// This is a C library containing an x86(16, 32 and 64) disassembler.
//
// Setup:
// Define the 'NMD_ASSEMBLY_IMPLEMENTATION' macro in one and only one source file before the include statement for this header file. If the compiler generates naming
// conflict errors caused by this header you should create a source file(e.g. "nmd_assembly.c"), define the implementation macro and include this header there. Example:
//#define NMD_ASSEMBLY_IMPLEMENTATION
//#include "nmd_assembly.h"
//
// High level functions overview:
//  nmd_x86_assemble() -> takes as input a string and fills a 'NMD_X86Instruction'.
//  nmd_x86_disassemble() -> takes as input a sequence of bytes and fills a 'NMD_X86Instruction'.
//  nmd_x86_construct_string() -> takes as input a 'NMD_X86Instruction' and constructs its string representation.
//
// Enabling and disabling features of the disassembler:
// To dynamically choose what features are used by the disassembler, use the 'featureFlags' parameter of nmd_x86_disassemble(). The less features specified in the mask, the
// faster the function runs. By default all features are available, some can be completely disabled at compile time(thus reducing code size and increasing code speed) using
// the following macros:
//  - 'NMD_ASSEMBLY_DISABLE_VALIDITY_CHECK': the disassembler function does not check if the instruction is valid.
//  - 'NMD_ASSEMBLY_DISABLE_NUM_OPERANDS': the disassembler doesn't fill the 'numOperands' member of the NMD_X86Instruction struct.
//  - 'NMD_ASSEMBLY_DISABLE_GROUP': the disassembler doesn't fill the 'group' member of the NMD_X86Instruction struct.
//  - 'NMD_ASSEMBLY_DISABLE_AFFECTED_CPU_STATE': the disassembler doesn't fill the following members: 'numRegsRead', 'regsRead', 'numRegsWrite' and 'regsWrite' members of the NMD_X86Instruction struct.
//  - 'NMD_ASSEMBLY_DISABLE_VEX': the assembler and disassembler doesn't support VEX instructions.
//  - 'NMD_ASSEMBLY_DISABLE_EVEX': the assembler and disassembler doesn't support EVEX instructions.
//
/* Usage demonstration:
//Expected output
//xor eax, eax
//inc eax
//ret
//mov esp, [ebp - 18h]

#include <stdio.h> // printf()

#define NMD_ASSEMBLY_IMPLEMENTATION
#include "nmd_assembly.h"
int main()
{
	const char code[] = { 0x33, 0xC0, 0x40, 0xC3, 0x8B, 0x65, 0xE8 };

	NMD_X86Instruction instruction;
	char instructionString[NMD_X86_MAXIMUM_INSTRUCTION_STRING_LENGTH];

	for(size_t i = 0; i < sizeof(code); i += instruction.length)
	{
		if (!nmd_x86_disassemble(code + i, &instruction, NMD_X86_INVALID_RUNTIME_ADDRESS, NMD_X86_MODE_32, NMD_X86_FEATURE_FLAGS_MINIMAL))
			break;

		nmd_x86_construct_string(&instruction, instructionString, NMD_X86_FORMAT_FLAGS_DEFAULT);

		printf("%s\n", instructionString);
	}

	return 0;
}
*/
//
// TODO
//  Short-Term
//   - Implement instruction id.
//   - Implement archirecture specific groups.
//   - Add a 'bufferSize' parameter to nmd_x86_disassemble(). 
//   - Add a variable that stores operand data.
//   - Implement affected cpu states on NMD_X86Instruction(read/written read/cpu flags).
//   - implement instruction set extensions to the disassembler : VEX, EVEX, MVEX, 3DNOW, XOP.
//   - Fix default segment override(DS,SS). Some instructions are showing the wrong one.
//   - Implement x86 Assembler(only the initial parsing is done).
//  Long-Term 
//   - Implement decompiler for x86.
//   - Add support for other architectures.
//
// References:
//  - Intel 64 and IA - 32 Architectures Software Developer's Manual Combined Volumes : 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4.
//  - Capstone Engine.
//  - Zydis Disassembler.
//  - VIA PadLock Programming Guide.

#ifndef NMD_ASSEMBLY_H
#define NMD_ASSEMBLY_H

//Dependencies
#include <stdbool.h>
#include <stdint.h>

#define NMD_X86_INVALID_RUNTIME_ADDRESS -1
#define NMD_X86_MAXIMUM_INSTRUCTION_STRING_LENGTH 128

//These format flags specify how a string that represents an instruction should be constructed.
enum NMD_X86_FORMAT_FLAGS
{
	NMD_X86_FORMAT_FLAGS_HEX                       = (1 << 0),  // If set, numbers are displayed in hex base, otherwise they are displayed in decimal base.
	NMD_X86_FORMAT_FLAGS_POINTER_SIZE              = (1 << 1),  // Pointer sizes(e.g. 'dword ptr', 'byte ptr') are displayed.
	NMD_X86_FORMAT_FLAGS_ONLY_SEGMENT_OVERRIDE     = (1 << 2),  // If set, only segment overrides using prefixes(e.g. '2EH', '64H') are displayed, otherwise a segment is always present before a memory operand.
	NMD_X86_FORMAT_FLAGS_COMMA_SPACES              = (1 << 3),  // A space is placed after a comma.
	NMD_X86_FORMAT_FLAGS_OPERATOR_SPACES           = (1 << 4),  // A space is placed before and after the '+' and '-' characters.
	NMD_X86_FORMAT_FLAGS_UPPERCASE                 = (1 << 5),  // The string is uppercase.
	NMD_X86_FORMAT_FLAGS_0X_PREFIX                 = (1 << 6),  // Hexadecimal numbers have the '0x'('0X' if uppercase) prefix.
	NMD_X86_FORMAT_FLAGS_H_SUFFIX                  = (1 << 7),  // Hexadecimal numbers have the 'h'('H' if uppercase') suffix.
	NMD_X86_FORMAT_FLAGS_ENFORCE_HEX_ID            = (1 << 8),  // If the HEX flag is set and either the prefix or suffix flag is also set, numbers less than 10 are displayed with preffix or suffix.
	NMD_X86_FORMAT_FLAGS_HEX_LOWERCASE             = (1 << 9),  // If the HEX flag is set and the UPPERCASE flag is not set, hexadecimal numbers are displayed in lowercase.
	NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW = (1 << 10), // If set, signed numbers are displayed as they are represented in memory(e.g. -1 = 0xFFFFFFFF).
	NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_HEX    = (1 << 11), // If set and NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW is also set, the number's hexadecimal representation is displayed in parenthesis.
	NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_DEC    = (1 << 12), // Same as NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_HEX, but the number is displayed in decimal base.
	NMD_X86_FORMAT_FLAGS_SCALE_ONE                 = (1 << 13), // If set, scale one is displayed. E.g. add byte ptr [eax+eax*1], al.
	NMD_X86_FORMAT_FLAGS_ALL                       = (1 << 14) - 1, // Specifies all format flags.
	NMD_X86_FORMAT_FLAGS_DEFAULT                   = (NMD_X86_FORMAT_FLAGS_HEX | NMD_X86_FORMAT_FLAGS_H_SUFFIX | NMD_X86_FORMAT_FLAGS_ONLY_SEGMENT_OVERRIDE | NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW | NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_DEC),
	NMD_X86_FORMAT_FLAGS_CAPSTONE_ENGINE           = (NMD_X86_FORMAT_FLAGS_HEX | NMD_X86_FORMAT_FLAGS_0X_PREFIX | NMD_X86_FORMAT_FLAGS_HEX_LOWERCASE | NMD_X86_FORMAT_FLAGS_POINTER_SIZE | NMD_X86_FORMAT_FLAGS_ONLY_SEGMENT_OVERRIDE | NMD_X86_FORMAT_FLAGS_COMMA_SPACES | NMD_X86_FORMAT_FLAGS_OPERATOR_SPACES) // Specifies a format similar to the default capstone engine's stlye.
};

enum NMD_X86_INSTRUCTION_PREFIXES
{
	NMD_X86_NONE_PREFIX                  = 0,
	NMD_X86_ES_SEGMENT_OVERRIDE_PREFIX   = (1 << 0),
	NMD_X86_CS_SEGMENT_OVERRIDE_PREFIX   = (1 << 1),
	NMD_X86_SS_SEGMENT_OVERRIDE_PREFIX   = (1 << 2),
	NMD_X86_DS_SEGMENT_OVERRIDE_PREFIX   = (1 << 3),
	NMD_X86_FS_SEGMENT_OVERRIDE_PREFIX   = (1 << 4),
	NMD_X86_GS_SEGMENT_OVERRIDE_PREFIX   = (1 << 5),
	NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX = (1 << 6),
	NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX = (1 << 7),
	NMD_X86_LOCK_PREFIX                  = (1 << 8),
	NMD_X86_REPEAT_NOT_ZERO_PREFIX       = (1 << 9),
	NMD_X86_REPEAT_PREFIX                = (1 << 10),
	NMD_X86_REX_PREFIX                   = (1 << 11),
	NMD_X86_REX_W_PREFIX                 = (1 << 12),
	NMD_X86_REX_R_PREFIX                 = (1 << 13),
	NMD_X86_REX_X_PREFIX                 = (1 << 14),
	NMD_X86_REX_B_PREFIX                 = (1 << 15)
};

enum NMD_X86_IMM_MASK
{
	NMD_X86_IMM_NONE = 0,
	NMD_X86_IMM8     = 1,
	NMD_X86_IMM16    = 2,
	NMD_X86_IMM32    = 4,
	NMD_X86_IMM48    = 6,
	NMD_X86_IMM64    = 8,
	ANY_NMD_X86_IMM  = (NMD_X86_IMM8 | NMD_X86_IMM16 | NMD_X86_IMM32 | NMD_X86_IMM64)
};

enum NMD_X86_DISP_MASK
{
	NMD_X86_DISP_NONE        = 0,
	NMD_X86_DISP8            = (1 << 0),
	NMD_X86_DISP16           = (1 << 1),
	NMD_X86_DISP32           = (1 << 2),
	NMD_X86_ANY_NMD_X86_DISP = (NMD_X86_DISP8 | NMD_X86_DISP16 | NMD_X86_DISP32)
};

typedef union NMD_Modrm
{
	struct
	{
		uint8_t rm  : 3;
		uint8_t reg : 3;
		uint8_t mod : 2;
	};
	uint8_t modrm;
} NMD_Modrm;

typedef union NMD_SIB
{
	struct
	{
		uint8_t base  : 3;
		uint8_t index : 3;
		uint8_t scale : 2;
	};
	uint8_t sib;
} NMD_SIB;

//You may use this enum to construct a mask and check against NMD_X86InstructionFlags::flags.
enum NMD_X86_INSTRUCTION_FLAGS
{
	NMD_X86_INSTRUCTION_FLAGS_VALID           = (1 << 0),
	NMD_X86_INSTRUCTION_FLAGS_X86_64          = (1 << 1),
	NMD_X86_INSTRUCTION_FLAGS_HAS_MODRM       = (1 << 2),
	NMD_X86_INSTRUCTION_FLAGS_HAS_SIB         = (1 << 3),
	NMD_X86_INSTRUCTION_FLAGS_IS_3OP_38H      = (1 << 4),
	NMD_X86_INSTRUCTION_FLAGS_OPERAND_SIZE_64 = (1 << 5),
	NMD_X86_INSTRUCTION_FLAGS_REPEAT_PREFIX   = (1 << 6),
};

//X86 mode.
typedef enum NMD_X86_MODE
{
	NMD_X86_MODE_16 = 1,
	NMD_X86_MODE_32 = 2,
	NMD_X86_MODE_64 = 3,
} NMD_X86_MODE;

enum NMD_X86_OPCODE_MAP
{
	NMD_X86_OPCODE_MAP_DEFAULT,
	NMD_X86_OPCODE_MAP_0F,
	NMD_X86_OPCODE_MAP_0F_38,
	NMD_X86_OPCODE_MAP_0F_3A
};

enum NMD_X86_INSTRUCTION_ENCODING
{
	NMD_X86_INSTRUCTION_ENCODING_LEGACY,  // Legacy encoding.
	//NMD_X86_INSTRUCTION_ENCODING_3DNOW, // AMD's 3DNow! extension. [TODO]
	//NMD_X86_INSTRUCTION_ENCODING_XOP,   // AMD's XOP(eXtended Operations) instruction set. [TODO]
	NMD_X86_INSTRUCTION_ENCODING_VEX,     // Intel's VEX(vector extensions) coding scheme.
	NMD_X86_INSTRUCTION_ENCODING_EVEX,    // Intel's EVEX(Enhanced vector extension) coding scheme.
	//NMD_X86_INSTRUCTION_ENCODING_MVEX,  // MVEX used by Intel's "Xeon Phi" ISA. [TODO]
};

typedef struct NMD_X86Vex
{
	uint8_t byte0; // Either C4h(3-byte VEX) or C5h(2-byte VEX).
	bool R;
	bool X;
	bool B;
	bool L;
	bool W;
	uint8_t m_mmmm;
	uint8_t vvvv;
	uint8_t pp;
	uint8_t vex[3]; //The full vex prefix.
} NMD_X86Vex;

typedef enum NMD_X86_REG_PORTION
{
	NMD_X86_REG_PORTION_NONE = 0,
	NMD_X86_REG_PORTION_LOWER_8,
	NMD_X86_REG_PORTION_HIGHER_8,
	NMD_X86_REG_PORTION_LOWER_16,
	NMD_X86_REG_PORTION_LOWER_32,
	NMD_X86_REG_PORTION_64,
	NMD_X86_REG_PORTION_128,
	NMD_X86_REG_PORTION_256,
	NMD_X86_REG_PORTION_512,
} NMD_X86_REG_PORTION;

typedef enum NMD_X86_REG
{
	NMD_X86_REG_NONE = 0,
	NMD_X86_REG_RAX,
	NMD_X86_REG_RBX,
	NMD_X86_REG_RCX,
	NMD_X86_REG_RDX,
	NMD_X86_REG_RBP,
	NMD_X86_REG_RSP,
	NMD_X86_REG_RSI,
	NMD_X86_REG_RDI,
	NMD_X86_REG_R8,
	NMD_X86_REG_R9,
	NMD_X86_REG_R10,
	NMD_X86_REG_R11,
	NMD_X86_REG_R12,
	NMD_X86_REG_R13,
	NMD_X86_REG_R14,
	NMD_X86_REG_R15,

	NMD_X86_REG_CR0,
	NMD_X86_REG_CR1,
	NMD_X86_REG_CR2,
	NMD_X86_REG_CR3,
	NMD_X86_REG_CR4,
	NMD_X86_REG_CR5,
	NMD_X86_REG_CR6,
	NMD_X86_REG_CR7,
	NMD_X86_REG_CR8,
	NMD_X86_REG_CR9,
	NMD_X86_REG_CR10,
	NMD_X86_REG_CR11,
	NMD_X86_REG_CR12,
	NMD_X86_REG_CR13,
	NMD_X86_REG_CR14,
	NMD_X86_REG_CR15,

	NMD_X86_REG_DR0,
	NMD_X86_REG_DR1,
	NMD_X86_REG_DR2,
	NMD_X86_REG_DR3,
	NMD_X86_REG_DR4,
	NMD_X86_REG_DR5,
	NMD_X86_REG_DR6,
	NMD_X86_REG_DR7,
	NMD_X86_REG_DR8,
	NMD_X86_REG_DR9,
	NMD_X86_REG_DR10,
	NMD_X86_REG_DR11,
	NMD_X86_REG_DR12,
	NMD_X86_REG_DR13,
	NMD_X86_REG_DR14,
	NMD_X86_REG_DR15,

	NMD_X86_REG_FP0,
	NMD_X86_REG_FP1,
	NMD_X86_REG_FP2,
	NMD_X86_REG_FP3,
	NMD_X86_REG_FP4,
	NMD_X86_REG_FP5,
	NMD_X86_REG_FP6,
	NMD_X86_REG_FP7,

	NMD_X86_REG_K0,
	NMD_X86_REG_K1,
	NMD_X86_REG_K2,
	NMD_X86_REG_K3,
	NMD_X86_REG_K4,
	NMD_X86_REG_K5,
	NMD_X86_REG_K6,
	NMD_X86_REG_K7,

	NMD_X86_REG_ST0,
	NMD_X86_REG_ST1,
	NMD_X86_REG_ST2,
	NMD_X86_REG_ST3,
	NMD_X86_REG_ST4,
	NMD_X86_REG_ST5,
	NMD_X86_REG_ST6,
	NMD_X86_REG_ST7,

	NMD_X86_REG_MM0,
	NMD_X86_REG_MM1,
	NMD_X86_REG_MM2,
	NMD_X86_REG_MM3,
	NMD_X86_REG_MM4,
	NMD_X86_REG_MM5,
	NMD_X86_REG_MM6,
	NMD_X86_REG_MM7,

	NMD_X86_REG_XMM0,
	NMD_X86_REG_XMM1,
	NMD_X86_REG_XMM2,
	NMD_X86_REG_XMM3,
	NMD_X86_REG_XMM4,
	NMD_X86_REG_XMM5,
	NMD_X86_REG_XMM6,
	NMD_X86_REG_XMM7,
	NMD_X86_REG_XMM8,
	NMD_X86_REG_XMM9,
	NMD_X86_REG_XMM10,
	NMD_X86_REG_XMM11,
	NMD_X86_REG_XMM12,
	NMD_X86_REG_XMM13,
	NMD_X86_REG_XMM14,
	NMD_X86_REG_XMM15,
	NMD_X86_REG_XMM16,
	NMD_X86_REG_XMM17,
	NMD_X86_REG_XMM18,
	NMD_X86_REG_XMM19,
	NMD_X86_REG_XMM20,
	NMD_X86_REG_XMM21,
	NMD_X86_REG_XMM22,
	NMD_X86_REG_XMM23,
	NMD_X86_REG_XMM24,
	NMD_X86_REG_XMM25,
	NMD_X86_REG_XMM26,
	NMD_X86_REG_XMM27,
	NMD_X86_REG_XMM28,
	NMD_X86_REG_XMM29,
	NMD_X86_REG_XMM30,
	NMD_X86_REG_XMM31,

	NMD_X86_REG_YMM0,
	NMD_X86_REG_YMM1,
	NMD_X86_REG_YMM2,
	NMD_X86_REG_YMM3,
	NMD_X86_REG_YMM4,
	NMD_X86_REG_YMM5,
	NMD_X86_REG_YMM6,
	NMD_X86_REG_YMM7,
	NMD_X86_REG_YMM8,
	NMD_X86_REG_YMM9,
	NMD_X86_REG_YMM10,
	NMD_X86_REG_YMM11,
	NMD_X86_REG_YMM12,
	NMD_X86_REG_YMM13,
	NMD_X86_REG_YMM14,
	NMD_X86_REG_YMM15,
	NMD_X86_REG_YMM16,
	NMD_X86_REG_YMM17,
	NMD_X86_REG_YMM18,
	NMD_X86_REG_YMM19,
	NMD_X86_REG_YMM20,
	NMD_X86_REG_YMM21,
	NMD_X86_REG_YMM22,
	NMD_X86_REG_YMM23,
	NMD_X86_REG_YMM24,
	NMD_X86_REG_YMM25,
	NMD_X86_REG_YMM26,
	NMD_X86_REG_YMM27,
	NMD_X86_REG_YMM28,
	NMD_X86_REG_YMM29,
	NMD_X86_REG_YMM30,
	NMD_X86_REG_YMM31,

	NMD_X86_REG_ZMM0,
	NMD_X86_REG_ZMM1,
	NMD_X86_REG_ZMM2,
	NMD_X86_REG_ZMM3,
	NMD_X86_REG_ZMM4,
	NMD_X86_REG_ZMM5,
	NMD_X86_REG_ZMM6,
	NMD_X86_REG_ZMM7,
	NMD_X86_REG_ZMM8,
	NMD_X86_REG_ZMM9,
	NMD_X86_REG_ZMM10,
	NMD_X86_REG_ZMM11,
	NMD_X86_REG_ZMM12,
	NMD_X86_REG_ZMM13,
	NMD_X86_REG_ZMM14,
	NMD_X86_REG_ZMM15,
	NMD_X86_REG_ZMM16,
	NMD_X86_REG_ZMM17,
	NMD_X86_REG_ZMM18,
	NMD_X86_REG_ZMM19,
	NMD_X86_REG_ZMM20,
	NMD_X86_REG_ZMM21,
	NMD_X86_REG_ZMM22,
	NMD_X86_REG_ZMM23,
	NMD_X86_REG_ZMM24,
	NMD_X86_REG_ZMM25,
	NMD_X86_REG_ZMM26,
	NMD_X86_REG_ZMM27,
	NMD_X86_REG_ZMM28,
	NMD_X86_REG_ZMM29,
	NMD_X86_REG_ZMM30,
	NMD_X86_REG_ZMM31,
} NMD_X86_REG;

typedef enum NMD_Group {
	NMD_GROUP_NONE = 0, // The instruction is not part of any group.
								   
	NMD_GROUP_JUMP                 = (1 << 0), // Jump instruction.
	NMD_GROUP_CALL                 = (1 << 1), // Call instruction.
	NMD_GROUP_RET                  = (1 << 2), // Return instruction.
	NMD_GROUP_IRET                 = (1 << 3), // Interrupt return instruction.
	NMD_GROUP_INT                  = (1 << 4), // Interrupt instruction.
	NMD_GROUP_PRIVILEGE            = (1 << 5), // Privileged instruction.
	NMD_GROUP_CONDITIONAL_BRANCH   = (1 << 6), // Conditional branch instruction.
	NMD_GROUP_UNCONDITIONAL_BRANCH = (1 << 7), // Unconditional branch instruction.
	NMD_GROUP_RELATIVE_ADDRESSING  = (1 << 8), // Relative addressing instruction.

	//These are not actual groups, but rather masks of groups.
	NMD_GROUP_BRANCH = (NMD_GROUP_CONDITIONAL_BRANCH | NMD_GROUP_UNCONDITIONAL_BRANCH), // Mask used to check if the instruction is a branch instruction.
	NMD_GROUP_ANY    = (1 << 8) - 1, // Mask used to check if the instruction is part of any group.
} NMD_Group;

typedef struct NMD_X86RegInfo
{
	uint8_t portion; // The portion of the register. A member of the 'NMD_X86_REG_PORTION'.
	uint8_t reg;     // The register. A member of the 'NMD_X86_REG'.
} NMD_X86RegInfo;

typedef union NMD_X86InstructionFlags
{
	struct
	{
		bool valid         : 1; // If true, the instruction is valid.
		bool hasModrm      : 1; // If true, the instruction has a modrm byte.
		bool hasSIB        : 1; // If true, the instruction has an SIB byte.
		bool operandSize64 : 1; // If true, a REX.W prefix is closer to the opcode than a operand size override prefix.
		bool repeatPrefix  : 1; // If true, a 'repeat'(F3h) prefix is closer to the opcode than a 'repeat not zero'(F2h) prefix.
	};
	uint8_t flags;
} NMD_X86InstructionFlags;

typedef struct NMD_X86Instruction
{
	NMD_X86InstructionFlags flags; // See the 'NMD_X86InstructionFlags' union.
	uint8_t opcodeMap;             // The opcode map the instruction belongs to. A member of the 'NMD_x86_OPCODE_MAP' enum.
	uint8_t encoding;              // The instruction's encoding. A member of the 'NMD_X86_INSTRUCTION_ENCODING' enum.
	uint8_t numPrefixes;           // Number of prefixes.
	uint8_t length;                // The instruction's length in bytes.
	uint8_t opcodeSize;            // The opcode's size in bytes.
	uint8_t numOperands;           // The number of operands.
	uint16_t group;                // Instruction group. A member of the 'NMD_GROUP' enum.
	uint8_t numRegsRead;           // Number of registers read from.
	NMD_X86RegInfo regsRead[4];    // Registers read from.
	uint8_t numRegsWrite;          // Numbers of registers written to.
	NMD_X86RegInfo regsWrite[4];   // Register written to.
	uint8_t mode;                  // A member of the 'NMD_X86_MODE' enum.
	uint8_t immMask;               // A member of the 'NMD_X86_IMM_MASK' enum.
	uint8_t dispMask;              // A member of the 'NMD_X86_DISP_MASK' enum.
	uint16_t prefixes;             // A mask of members of the 'NMD_X86_INSTRUCTION_PREFIXES' enum.
	uint8_t segmentOverride;       // One segment override prefix in the 'NMD_X86_INSTRUCTION_PREFIXES' enum that is closest to the opcode.
	uint16_t simdPrefix;           // Either one of these prefixes that is closest to the opcode: 66h, F0h, F2h, F3h, or NMD_X86_NONE_PREFIX. The prefixes are specified as members of the 'INSTRUCTION_PREFIXES' enum.
	uint8_t opcode;                // Opcode byte.
	NMD_X86Vex vex;                // VEX prefix.
	NMD_Modrm modrm;               // The modrm byte. Check 'flags.hasModrm'.
	NMD_SIB sib;                   // The SIB byte. Check 'flags.hasSIB'.
	uint64_t cpuFlags;             // The cpu flags(eflags, rflags, fpu flags) affected by the instruction.
	uint32_t displacement;         // Displacement. Check 'dispMask'.
	uint64_t immediate;            // Immediate. Check 'immMask'.
	uint64_t runtimeAddress;       // Runtime address provided by the user.
	uint8_t fullInstruction[15];   // Buffer containing the full instruction.
} NMD_X86Instruction;

enum NMD_X86_FEATURE_FLAGS
{
	NMD_X86_FEATURE_FLAGS_NONE               = 0,
	NMD_X86_FEATURE_FLAGS_VALIDITY_CHECK     = (1 << 0), // The disassembler checks if the instruction is valid.
	NMD_X86_FEATURE_FLAGS_NUM_OPERANDS       = (1 << 1), // The instruction's number of operands is calculated and the 'numOperands' variable is filled.
	NMD_X86_FEATURE_FLAGS_GROUP              = (1 << 2), // The instruction's group is parsed and the 'group' variable is filled.
	NMD_X86_FEATURE_FLAGS_AFFECTED_CPU_STATE = (1 << 3),
	NMD_X86_FEATURE_FLAGS_VEX                = (1 << 4), // VEX instructions are parsed.
	NMD_X86_FEATURE_FLAGS_EVEX               = (1 << 5), // EVEX instructions are parsed.

	//These are not actual features, but rather masks of features.
	NMD_X86_FEATURE_FLAGS_MINIMAL            = (NMD_X86_FEATURE_FLAGS_VALIDITY_CHECK | NMD_X86_FEATURE_FLAGS_VEX | NMD_X86_FEATURE_FLAGS_EVEX), // Mask that specifies minimal features to provide acurate results in any environment.
	NMD_X86_FEATURE_FLAGS_ALL                = (1 << 6) - 1, // Mask that specifies all features.
};

//Assembles an instruction from a string. Returns true if the operation was successful, false otherwise.
//Parameters:
//  string      [in]  A pointer to a string.
//  instruction [out] A pointer to a variable of type 'NMD_X86Instruction'.
//	mode        [in]  A member of the 'NMD_X86_MODE' enum.The architecture mode. A member of the 'NMD_X86_MODE' enum.
bool nmd_x86_assemble(const char* string, NMD_X86Instruction* instruction, NMD_X86_MODE mode);

//Disassembles an instruction. Returns true if the instruction is valid, false otherwise.
//Parameters:
//	buffer         [in]  A pointer to a sequence of bytes(encoded instruction).
//	instruction    [out] A pointer to a variable of type 'NMD_X86Instruction'.
//  runtimeAddress [in]  The instruction's runtime address.
//	mode           [in]  The architecture mode. A member of the 'NMD_X86_MODE' enum.
//  featureFlags   [in]  A mask of 'NMD_X86_FEATURE_FLAGS_XXX'.
bool nmd_x86_disassemble(const void* buffer, NMD_X86Instruction* instruction, uint64_t runtimeAddress, NMD_X86_MODE mode, uint32_t featureFlags);

//Constructs a string from a pointer to variable of type 'NMD_X86Instruction'.
//This function may cause a crash if you modify 'instruction' manually.
//Parameters:
//	instruction [in]  A pointer to a variable of type 'NMD_X86Instruction'.
//	buffer      [out] A pointer to buffer that receives the string.
//  formatFlags [in]  A mask of 'NMD_X86_FORMAT_FLAGS_XXX' specifying how the string should be constructed.
void nmd_x86_construct_string(const NMD_X86Instruction* instruction, char* buffer, uint32_t formatFlags);

#ifdef NMD_ASSEMBLY_IMPLEMENTATION

#define NMD_R(b) ((b) >> 4) // Four high-order bits of an opcode to index a row of the opcode table
#define NMD_C(b) ((b) & 0xF) // Four low-order bits to index a column of the table

bool nmd_findByte(const uint8_t* arr, const size_t N, const uint8_t x) { for (size_t i = 0; i < N; i++) { if (arr[i] == x) { return true; } }; return false; }

void parseModrm(const uint8_t** b, NMD_X86Instruction* const instruction)
{
	instruction->flags.hasModrm = true;
	instruction->modrm.modrm = *++ * b;
	const bool addressPrefix = (bool)(instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX);

	if (instruction->mode == NMD_X86_MODE_16)
	{
		if (instruction->modrm.mod != 0b11)
		{
			if (instruction->modrm.mod == 0b00)
			{
				if (instruction->modrm.rm == 0b110)
					instruction->dispMask = NMD_X86_DISP16;
			}
			else
				instruction->dispMask = instruction->modrm.mod == 0b01 ? NMD_X86_DISP8 : NMD_X86_DISP16;
		}
	}
	else
	{
		if (addressPrefix && instruction->mode == NMD_X86_MODE_32)
		{
			if ((instruction->modrm.mod == 0b00 && instruction->modrm.rm == 0b110) || instruction->modrm.mod == 0b10)
				instruction->dispMask = NMD_X86_DISP16;
			else if (instruction->modrm.mod == 0b01)
				instruction->dispMask = NMD_X86_DISP8;
		}
		else //if (!addressPrefix || /*(addressPrefix && **b >= 0x40) ||*/ (addressPrefix && instruction->mode == NMD_X86_MODE_64))
		{
			//Check for SIB byte
			if (instruction->modrm.modrm < 0xC0 && instruction->modrm.rm == 0b100 && (!addressPrefix || (addressPrefix && instruction->mode == NMD_X86_MODE_64)))
				instruction->flags.hasSIB = true, instruction->sib.sib = *++ * b;

			if (instruction->modrm.mod == 0b01) // disp8 (ModR/M)
				instruction->dispMask = NMD_X86_DISP8;
			else if ((instruction->modrm.mod == 0b00 && instruction->modrm.rm == 0b101) || instruction->modrm.mod == 0b10) //disp16,32 (ModR/M)
				instruction->dispMask = (addressPrefix && !(instruction->mode == NMD_X86_MODE_64 && instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX) ? NMD_X86_DISP16 : NMD_X86_DISP32);
			else if (instruction->flags.hasSIB && instruction->sib.base == 0b101) //disp8,32 (SIB)
				instruction->dispMask = (instruction->modrm.mod == 0b01 ? NMD_X86_DISP8 : NMD_X86_DISP32);
		}
	}

	for (size_t i = 0; i < instruction->dispMask; i++, (*b)++)
		((uint8_t*)(&instruction->displacement))[i] = *(*b + 1);
};

const char* nmd_strstr(const char* buffer, const char* string)
{
	const char* begin = buffer;
	const char* b = buffer;
	const char* s = string;
	for (; *b; b++, s++)
	{
		if (*s == '\0')
			return begin;

		if (*b != *s)
			s = string, begin = b;
	}

	return b;
}

//Assembles an instruction from a string. Returns true if the operation was successful, false otherwise.
//Parameters:
//  string      [in]  A pointer to a string.
//  instruction [out] A pointer to a variable of type 'NMD_X86Instruction'.
//	mode        [in]  A member of the 'NMD_X86_MODE' enum.The architecture mode. A member of the 'NMD_X86_MODE' enum.
bool nmd_x86_assemble(const char* string, NMD_X86Instruction* instruction, NMD_X86_MODE mode)
{
	if (*string == '\0')
		return false;

	//Clear instruction
	for (size_t i = 0; i < sizeof(NMD_X86Instruction); i++)
		((uint8_t*)(instruction))[i] = 0x00;

	instruction->mode = mode;

	char buffer[NMD_X86_MAXIMUM_INSTRUCTION_STRING_LENGTH];

	//Copy 'string' to 'buffer' converting it to lowercase. If the character ';' is found, stop.
	size_t length = 0;
	for (; string[length]; length++)
	{
		const char c = string[length];
		if (c == ';')
			break;

		buffer[length] = (c >= 'A' && c <= 'Z') ? c + 0x20 : c;
	}

	//Remove any number of contiguous of white-spaces at the end of the string.
	for (size_t i = length - 1; i > 0; i--)
	{
		if (buffer[i] != ' ')
		{
			length = i + 1;
			break;
		}
	}

	//Remove all white-spaces where the previous character is not a lowercase letter. 
	for (size_t i = 0; i < length; i++)
	{
		if (buffer[i] == ' ' && (i == 0 || !(buffer[i - 1] >= 'a' && buffer[i - 1] <= 'z')))
		{
			for (size_t j = i; j < length; j++)
				buffer[j] = buffer[j + 1];

			length--;
		}
	}

	//After all the string manipulation, place the null character.
	buffer[length] = '\0';

	//"index"
	const char* b = buffer;

	if (nmd_strstr(buffer, "lock ") == buffer)
		instruction->prefixes |= NMD_X86_LOCK_PREFIX, b += 5;
	else if (nmd_strstr(buffer, "rep ") == buffer)
		instruction->prefixes |= NMD_X86_REPEAT_PREFIX, b += 4;
	else if (nmd_strstr(buffer, "repe ") == buffer || nmd_strstr(buffer, "repz ") == buffer)
		instruction->prefixes |= NMD_X86_REPEAT_PREFIX, b += 5;
	else if (nmd_strstr(buffer, "repne ") == buffer || nmd_strstr(buffer, "repnz ") == buffer)
		instruction->prefixes |= NMD_X86_REPEAT_NOT_ZERO_PREFIX, b += 6;

	if (nmd_strstr(buffer, "xacquire ") == buffer)
	{
	}
	else if (nmd_strstr(buffer, "xrelease ") == buffer)
	{
	}

	//find opcode...

	return true;
}

//Disassembles an instruction. Returns true if the instruction is valid, false otherwise.
//Parameters:
//	buffer         [in]  A pointer to a sequence of bytes(encoded instruction).
//	instruction    [out] A pointer to a variable of type 'NMD_X86Instruction'.
//  runtimeAddress [in]  The instruction's runtime address.
//	mode           [in]  The architecture mode. A member of the 'NMD_X86_MODE' enum.
//  featureFlags   [in]  A mask of 'NMD_X86_FEATURE_FLAGS_XXX'.
bool nmd_x86_disassemble(const void* buffer, NMD_X86Instruction* instruction, uint64_t runtimeAddress, NMD_X86_MODE mode, uint32_t featureFlags)
{
	const uint8_t prefixes[] = { 0xF0, 0xF2, 0xF3, 0x2E, 0x36, 0x3E, 0x26, 0x64, 0x65, 0x66, 0x67 };
	const uint8_t op1modrm[] = { 0x62, 0x63, 0x69, 0x6B, 0xC0, 0xC1, 0xC4, 0xC5, 0xC6, 0xC7, 0xD0, 0xD1, 0xD2, 0xD3, 0xF6, 0xF7, 0xFE, 0xFF };
	const uint8_t op1imm8[] = { 0x6A, 0x6B, 0x80, 0x82, 0x83, 0xA8, 0xC0, 0xC1, 0xC6, 0xCD, 0xD4, 0xD5, 0xEB };
	const uint8_t op1imm32[] = { 0x68, 0x69, 0x81, 0xA9, 0xC7, 0xE8, 0xE9 };
	const uint8_t op2modrm[] = { 0x0D, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF };

	//Clear instruction
	for (size_t i = 0; i < sizeof(NMD_X86Instruction); i++)
		((uint8_t*)(instruction))[i] = 0x00;

	instruction->runtimeAddress = runtimeAddress;
	instruction->mode = mode;

	size_t offset = 0;
	const uint8_t* b = (const uint8_t*)(buffer);

	//Parse legacy prefixes & REX prefixes
	for (size_t i = 0; i < 14; i++, b++)
	{
		switch (*b)
		{
		case 0xF0: instruction->prefixes |= (instruction->simdPrefix = NMD_X86_LOCK_PREFIX); continue;
		case 0xF2: instruction->prefixes |= (instruction->simdPrefix = NMD_X86_REPEAT_NOT_ZERO_PREFIX), instruction->flags.repeatPrefix = false; continue;
		case 0xF3: instruction->prefixes |= (instruction->simdPrefix = NMD_X86_REPEAT_PREFIX), instruction->flags.repeatPrefix = true; continue;
		case 0x2E: instruction->prefixes |= (instruction->segmentOverride = NMD_X86_CS_SEGMENT_OVERRIDE_PREFIX); continue;
		case 0x36: instruction->prefixes |= (instruction->segmentOverride = NMD_X86_SS_SEGMENT_OVERRIDE_PREFIX); continue;
		case 0x3E: instruction->prefixes |= (instruction->segmentOverride = NMD_X86_DS_SEGMENT_OVERRIDE_PREFIX); continue;
		case 0x26: instruction->prefixes |= (instruction->segmentOverride = NMD_X86_ES_SEGMENT_OVERRIDE_PREFIX); continue;
		case 0x64: instruction->prefixes |= (instruction->segmentOverride = NMD_X86_FS_SEGMENT_OVERRIDE_PREFIX); continue;
		case 0x65: instruction->prefixes |= (instruction->segmentOverride = NMD_X86_GS_SEGMENT_OVERRIDE_PREFIX); continue;
		case 0x66: instruction->prefixes |= (instruction->simdPrefix = NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX), instruction->flags.operandSize64 = false; continue;
		case 0x67: instruction->prefixes |= NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX; continue;
		default:
			if ((instruction->mode == NMD_X86_MODE_64) && NMD_R(*b) == 4) // 0x40
			{
				instruction->prefixes |= NMD_X86_REX_PREFIX;
				instruction->prefixes &= ~(NMD_X86_REX_B_PREFIX | NMD_X86_REX_X_PREFIX | NMD_X86_REX_R_PREFIX | NMD_X86_REX_W_PREFIX);

				if (*b & 0b0001) // bit position 0
					instruction->prefixes |= NMD_X86_REX_B_PREFIX;
				if (*b & 0b0010) // bit position 1
					instruction->prefixes |= NMD_X86_REX_X_PREFIX;
				if (*b & 0b0100) // bit position 2
					instruction->prefixes |= NMD_X86_REX_R_PREFIX;
				if (*b & 0b1000) // bit position 3
					instruction->prefixes |= NMD_X86_REX_W_PREFIX, instruction->flags.operandSize64 = true;

				continue;
			}
		}

		break;
	}

	instruction->numPrefixes = (uint8_t)((ptrdiff_t)(b)-(ptrdiff_t)(buffer));

	//Assume NMD_X86_INSTRUCTION_ENCODING_LEGACY.
	instruction->encoding = NMD_X86_INSTRUCTION_ENCODING_LEGACY;

	//Parse opcode(s)
	if (*b == 0x0F) // 2,3 bytes
	{
		b++;

		if (*b == 0x38 || *b == 0x3A) // 3 bytes
		{
			instruction->opcodeSize = 3;
			instruction->opcode = *++b;
			const NMD_Modrm modrm = *(NMD_Modrm*)(b + 1);

			if (*(b - 1) == 0x38)
			{
				instruction->opcodeMap = NMD_X86_OPCODE_MAP_0F_38;

#ifndef NMD_ASSEMBLY_DISABLE_VALIDITY_CHECK
				if (featureFlags & NMD_X86_FEATURE_FLAGS_VALIDITY_CHECK)
				{
					// Check if the instruction is invalid.
					if (((*b == 0xf0 || *b == 0xf1) && ((instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX || !(instruction->prefixes & (NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX | NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX))) && modrm.mod == 0b11)) ||
						!(*b <= 0xb || (*b >= 0x1c && *b <= 0x1e) || (*b >= 0xc8 && *b <= 0xcd) ||
							(instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX && (*b == 0x10 || *b == 0x14 || *b == 0x15 || *b == 0x17 || (*b >= 0x20 && *b <= 0x25) || (*b == 0x2a && modrm.mod != 0b11) || (*b >= 0x28 && *b <= 0x2b && *b != 0x2a) || (NMD_R(*b) == 3 && *b != 0x36) || *b == 0x40 || *b == 0x41 || ((*b >= 0x80 && *b <= 0x82) && modrm.mod != 0b11) || (*b >= 0xdb && *b <= 0xdf))) ||
							((*b == 0xf0 || *b == 0xf1) && !(instruction->prefixes & NMD_X86_REPEAT_PREFIX)) ||
							(*b == 0xf6 && instruction->prefixes & (NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX | NMD_X86_REPEAT_PREFIX))))
						return false;
				}
#endif

#ifndef NMD_ASSEMBLY_DISABLE_NUM_OPERANDS
				// Parse the instruction's number of operands.
				if(featureFlags & NMD_X86_FEATURE_FLAGS_NUM_OPERANDS)
					instruction->numOperands = 2;
#endif
			}
			else
			{
				instruction->opcodeMap = NMD_X86_OPCODE_MAP_0F_3A;
				instruction->immMask = NMD_X86_IMM8, offset++;

#ifndef NMD_ASSEMBLY_DISABLE_VALIDITY_CHECK
				if (featureFlags & NMD_X86_FEATURE_FLAGS_VALIDITY_CHECK)
				{
					// Check if the instruction is invalid.
					if (!(((instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX || !(instruction->prefixes & (NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX | NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX))) && *b == 0xf) || *b == 0xcc ||
						(instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX && ((*b >= 0x8 && *b <= 0xe) || (*b >= 0x14 && *b <= 0x17) || (*b >= 0x20 && *b <= 0x22) || (*b >= 0x40 && *b <= 0x42) || *b == 0x44 || (*b >= 0x60 && *b <= 0x63) || *b == 0xdf))))
						return false;
				}
#endif

#ifndef NMD_ASSEMBLY_DISABLE_NUM_OPERANDS
				// Parse the instruction's number of operands.
				if (featureFlags & NMD_X86_FEATURE_FLAGS_NUM_OPERANDS)
					instruction->numOperands = 3;
#endif
			}

			parseModrm(&b, instruction);
		}
		else // 2 bytes
		{
			instruction->opcodeMap = NMD_X86_OPCODE_MAP_0F;
			instruction->opcodeSize = 2;
			instruction->opcode = *b;


			const NMD_Modrm modrm = *(NMD_Modrm*)(b + 1);
#ifndef NMD_ASSEMBLY_DISABLE_VALIDITY_CHECK
			if (featureFlags & NMD_X86_FEATURE_FLAGS_VALIDITY_CHECK)
			{
				// Check if the instruction is invalid.
				const uint8_t invalid2op[] = { 0x04, 0x0a, 0x0c, 0x0f, 0x7a, 0x7b };
				if (nmd_findByte(invalid2op, sizeof(invalid2op), *b) ||
					(*b == 0xc7 && (modrm.reg == 0b000 || modrm.reg == 0b010 || (modrm.mod == 0b11 && (instruction->prefixes & NMD_X86_REPEAT_PREFIX ? modrm.reg != 0b111 : (!(instruction->prefixes & NMD_X86_REPEAT_NOT_ZERO_PREFIX) ? modrm.reg <= 0b101 : true))) || (instruction->prefixes & (NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX | NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX) && modrm.reg >= 0b010 && modrm.reg <= 0b101))) ||
					(*b == 0x00 && modrm.reg >= 0b110) ||
					(*b == 0x01 && (modrm.mod == 0b11 ? ((instruction->prefixes & (NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX | NMD_X86_REPEAT_PREFIX) && ((modrm.modrm >= 0xc0 && modrm.modrm <= 0xc5) || (modrm.modrm >= 0xc8 && modrm.modrm <= 0xcb) || (modrm.modrm >= 0xcf && modrm.modrm <= 0xd1) || (modrm.modrm >= 0xd4 && modrm.modrm <= 0xd7) || modrm.modrm == 0xee || modrm.modrm == 0xef || modrm.modrm == 0xfa || modrm.modrm == 0xfb)) || (modrm.reg == 0b000 && modrm.rm >= 0b110) || (modrm.reg == 0b001 && modrm.rm >= 0b100 && modrm.rm <= 0b110) || (modrm.reg == 0b010 && (modrm.rm == 0b010 || modrm.rm == 0b011)) || (modrm.reg == 0b101 && modrm.rm < 0b110 && (!(instruction->prefixes & NMD_X86_REPEAT_PREFIX) || (instruction->prefixes & NMD_X86_REPEAT_PREFIX && (modrm.rm != 0b000 && modrm.rm != 0b010)))) || (modrm.reg == 0b111 && (modrm.rm > 0b101 || (!(instruction->mode == NMD_X86_MODE_64) && modrm.rm == 0b000)))) : (!(instruction->prefixes & NMD_X86_REPEAT_PREFIX) && modrm.reg == 0b101))) ||
					(instruction->prefixes & (NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX) && ((*b >= 0x13 && *b <= 0x17 && !(*b == 0x16 && instruction->prefixes & NMD_X86_REPEAT_PREFIX)) || *b == 0x28 || *b == 0x29 || *b == 0x2e || *b == 0x2f || (*b <= 0x76 && *b >= 0x74))) ||
					(modrm.mod == 0b11 && (*b == 0xb2 || *b == 0xb4 || *b == 0xb5 || *b == 0xc3 || *b == 0xe7 || *b == 0x2b || (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX && (*b == 0x12 || *b == 0x16)) || (!(instruction->prefixes & (NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX)) && (*b == 0x13 || *b == 0x17)))) ||
					((*b == 0x1A || *b == 0x1B) && (modrm.reg >= 0b100 && modrm.mod != 0b11)) ||
					(*b >= 0x24 && *b <= 0x27) || *b == 0x36 || *b == 0x39 || (*b >= 0x3b && *b <= 0x3f) ||
					(NMD_R(*b) == 5 && ((*b == 0x50 && modrm.mod != 0b11) || (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX && (*b == 0x52 || *b == 0x53)) || (instruction->prefixes & NMD_X86_REPEAT_PREFIX && (*b == 0x50 || (*b >= 0x54 && *b <= 0x57))) || (instruction->prefixes & NMD_X86_REPEAT_NOT_ZERO_PREFIX && (*b == 0x50 || (*b >= 0x52 && *b <= 0x57) || *b == 0x5b)))) ||
					(NMD_R(*b) == 6 && ((!(instruction->prefixes & (NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX | NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX)) && (*b == 0x6c || *b == 0x6d)) || (instruction->prefixes & NMD_X86_REPEAT_PREFIX && *b != 0x6f) || instruction->prefixes & NMD_X86_REPEAT_NOT_ZERO_PREFIX)) ||
					((*b == 0x78 || *b == 0x79) && instruction->prefixes & (NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX | NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX)) ||
					((*b == 0x7c || *b == 0x7d) && (instruction->prefixes & NMD_X86_REPEAT_PREFIX || !(instruction->prefixes & (NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX | NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX)))) ||
					((*b == 0x7e || *b == 0x7f) && instruction->prefixes & NMD_X86_REPEAT_NOT_ZERO_PREFIX) ||
					(((*b >= 0x71 && *b <= 0x73) && (instruction->prefixes & (NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX) || modrm.modrm <= 0xcf || (modrm.modrm >= 0xe8 && modrm.modrm <= 0xef)))) ||
					((*b == 0x71 || *b == 0x72 || (*b == 0x73 && !(instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX))) && ((modrm.modrm >= 0xd8 && modrm.modrm <= 0xdf) || modrm.modrm >= 0xf8)) ||
					(*b == 0x73 && (modrm.modrm >= 0xe0 && modrm.modrm <= 0xe8)) ||
					(*b == 0xa6 && modrm.modrm != 0xc0 && modrm.modrm != 0xc8 && modrm.modrm != 0xd0) ||
					(*b == 0xa7 && !(modrm.mod == 0b11 && modrm.reg <= 0b101 && modrm.rm == 0b000)) ||
					(*b == 0xae && (instruction->prefixes & NMD_X86_REPEAT_NOT_ZERO_PREFIX || (!(instruction->prefixes & (NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX | NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX)) && modrm.modrm >= 0xc0 && modrm.modrm <= 0xe7) || (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX && (modrm.mod == 0b11 ? modrm.modrm != 0xf8 : modrm.reg <= 0b101)) || (instruction->prefixes & NMD_X86_REPEAT_PREFIX && (modrm.mod == 0b11 ? modrm.reg != 0b101 : (modrm.reg != 0b100 && modrm.reg != 0b110))))) ||
					(*b == 0xb8 && !(instruction->prefixes & NMD_X86_REPEAT_PREFIX)) ||
					(*b == 0xba && modrm.reg <= 0b011) ||
					(*b == 0xd0 && !(instruction->prefixes & (NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX))) ||
					((*b == 0xd6 || *b == 0xe6) && !(instruction->prefixes & (NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX | NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX))) ||
					(*b == 0xf0 && (!(instruction->prefixes & NMD_X86_REPEAT_NOT_ZERO_PREFIX) || modrm.mod == 0b11)) ||
					(((*b == 0xd6 && instruction->prefixes & (NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX)) || *b == 0xd7 || *b == 0xf7 || *b == 0xc5) && modrm.mod != 0b11))
					return false;
			}
#endif

			if (NMD_R(*b) == 8) // disp32
				instruction->immMask = (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? NMD_X86_IMM16 : NMD_X86_IMM32), offset += instruction->immMask;
			else if ((NMD_R(*b) == 7 && NMD_C(*b) < 4) || *b == 0xA4 || *b == 0xC2 || (*b > 0xC3 && *b <= 0xC6) || *b == 0xBA || *b == 0xAC) //imm8
				instruction->immMask = NMD_X86_IMM8, offset++;

#ifndef NMD_ASSEMBLY_DISABLE_NUM_OPERANDS
			// Parse the instruction's number of operands.
			if (featureFlags & NMD_X86_FEATURE_FLAGS_NUM_OPERANDS)
			{
				if (*b == 0xa4 || *b == 0xa5 || *b == 0xc2 || (*b >= 0xc4 && *b <= 0xc6))
					instruction->numOperands = 3;
				else if (*b == 0x2 || *b == 0x3 || (*b >= 0x10 && *b <= 0x17) || NMD_R(*b) == 2 || (NMD_R(*b) >= 4 && NMD_R(*b) <= 7) || *b == 0xa3 || *b == 0xab || *b == 0xaf || (NMD_R(*b) >= 0xc && *b != 0xc7 && *b != 0xff))
					instruction->numOperands = 2;
				else if (NMD_R(*b) == 8 || NMD_R(*b) == 9 || (NMD_R(*b) == 0xa && *b % 8 < 2) || *b == 0xc7)
					instruction->numOperands = 1;
			}
#endif

#ifndef NMD_ASSEMBLY_DISABLE_GROUP
			// Parse the instruction's group.
			if (featureFlags & NMD_X86_FEATURE_FLAGS_GROUP)
			{
				if (NMD_R(*b) == 8)
					instruction->group = NMD_GROUP_JUMP | NMD_GROUP_CONDITIONAL_BRANCH | NMD_GROUP_RELATIVE_ADDRESSING;
				else if ((*b == 0x01 && modrm.rm == 0b111 && (modrm.mod == 0b00 || modrm.modrm == 0xf8)) || *b == 0x06 || *b == 0x08 || *b == 0x09)
					instruction->group = NMD_GROUP_PRIVILEGE;
				else if (*b == 0x05)
					instruction->group = NMD_GROUP_INT;
			}
#endif

			// Check for ModR/M, SIB and displacement.
			if (*b >= 0x20 && *b <= 0x23)
				instruction->flags.hasModrm = true, instruction->modrm.modrm = *++b;
			else if (nmd_findByte(op2modrm, sizeof(op2modrm), *b) || *b < 4 || (NMD_R(*b) != 3 && NMD_R(*b) > 0 && NMD_R(*b) < 7) || (*b >= 0xD0 && *b != 0xFF) || (NMD_R(*b) == 7 && NMD_C(*b) != 7) || NMD_R(*b) == 9 || (NMD_R(*b) == 0xB && *b != 0xb9) || (NMD_R(*b) == 0xC && NMD_C(*b) < 8))
				parseModrm(&b, instruction);
		}
	}
	else // 1 byte
	{
		instruction->opcodeSize = 1;
		instruction->opcode = *b;
		instruction->opcodeMap = NMD_X86_OPCODE_MAP_DEFAULT;

		const NMD_Modrm modrm = *(NMD_Modrm*)(b + 1);
#ifndef NMD_ASSEMBLY_DISABLE_VALIDITY_CHECK
		// Check if the instruction is invalid.
		if (featureFlags & NMD_X86_FEATURE_FLAGS_VALIDITY_CHECK)
		{
			if (((*b == 0xC6 || *b == 0xC7) && ((modrm.reg != 0b000 && modrm.reg != 0b111) || (modrm.reg == 0b111 && (modrm.mod != 0b11 || modrm.rm != 0b000)))) ||
				(*b == 0x8f && modrm.reg != 0b000) ||
				(*b == 0xfe && modrm.reg >= 0b010) ||
				(*b == 0xff && (modrm.reg == 0b111 || (modrm.mod == 0b11 && (modrm.reg == 0b011 || modrm.reg == 0b101)))) ||
				((*b == 0x8c || *b == 0x8e) && modrm.reg >= 6) ||
				(*b == 0x8e && modrm.reg == 0b001) ||
				(modrm.mod == 0b11 && *b == 0x8d) ||
				((*b == 0xc4 || *b == 0xc5) && mode == NMD_X86_MODE_64 && modrm.mod != 0b11))
				return false;
			else if (*b >= 0xd8 && *b <= 0xdf)
			{
				switch (*b)
				{
				case 0xd9:
					if ((modrm.reg == 0b001 && modrm.mod != 0b11) || (modrm.modrm > 0xd0 && modrm.modrm < 0xd8) || modrm.modrm == 0xe2 || modrm.modrm == 0xe3 || modrm.modrm == 0xe6 || modrm.modrm == 0xe7 || modrm.modrm == 0xef)
						return false;
					break;
				case 0xda:
					if (modrm.modrm >= 0xe0 && modrm.modrm != 0xe9)
						return false;
					break;
				case 0xdb:
					if (((modrm.reg == 0b100 || modrm.reg == 0b110) && modrm.mod != 0b11) || (modrm.modrm >= 0xe5 && modrm.modrm <= 0xe7) || modrm.modrm >= 0xf8)
						return false;
					break;
				case 0xdd:
					if ((modrm.reg == 0b101 && modrm.mod != 0b11) || NMD_R(modrm.modrm) == 0xf)
						return false;
					break;
				case 0xde:
					if (modrm.modrm == 0xd8 || (modrm.modrm >= 0xda && modrm.modrm <= 0xdf))
						return false;
					break;
				case 0xdf:
					if ((modrm.modrm >= 0xe1 && modrm.modrm <= 0xe7) || modrm.modrm >= 0xf8)
						return false;
					break;
				}
			}
			else if (mode == NMD_X86_MODE_64 && (*b == 0x6 || *b == 0x7 || *b == 0xe || *b == 0x16 || *b == 0x17 || *b == 0x1e || *b == 0x1f || *b == 0x27 || *b == 0x2f || *b == 0x37 || *b == 0x3f || (*b >= 0x60 && *b <= 0x62) || *b == 0x82 || *b == 0xce || (*b >= 0xd4 && *b <= 0xd6)))
				return false;
		}
#endif

#ifndef NMD_ASSEMBLY_DISABLE_EVEX
		// Check if instruction is EVEX.
		if (featureFlags & NMD_X86_FEATURE_FLAGS_EVEX && *b == 0x62 && modrm.mod == 0b11)
		{
			instruction->encoding = NMD_X86_INSTRUCTION_ENCODING_VEX;
		}
#endif

#if !defined(NMD_ASSEMBLY_DISABLE_EVEX) && !defined(NMD_ASSEMBLY_DISABLE_VEX)
		else
#endif
#ifndef NMD_ASSEMBLY_DISABLE_VEX
		// Check if instruction is VEX.
		if (featureFlags & NMD_X86_FEATURE_FLAGS_VEX && (*b == 0xc4 || *b == 0xc5) && modrm.mod == 0b11)
		{
			instruction->encoding = NMD_X86_INSTRUCTION_ENCODING_VEX;

			instruction->vex.byte0 = *b;
			const uint8_t byte1 = *++b;

			instruction->vex.R = byte1 & 0b10000000;
			if (instruction->vex.byte0 == 0xc4)
			{
				instruction->vex.X = (byte1 & 0b01000000) == 0b01000000;
				instruction->vex.B = (byte1 & 0b00100000) == 0b00100000;
				instruction->vex.m_mmmm = byte1 & 0b00011111;

				const uint8_t byte2 = *++b;
				instruction->vex.W = (byte2 & 0b10000000) == 0b10000000;
				instruction->vex.vvvv = (byte2 & 0b01111000) >> 3;
				instruction->vex.L = (byte2 & 0b00000100) == 0b00000100;
				instruction->vex.pp = byte2 & 0b00000011;

				instruction->opcode = *++b;
			}
			else
			{
				instruction->vex.vvvv = byte1 & 0b01111000;
				instruction->vex.L = byte1 & 0b00000100;
				instruction->vex.pp = byte1 & 0b00000011;

				b++;
				instruction->opcode = *b;
			}

			parseModrm(&b, instruction);
		}
#endif
#if !(defined(NMD_ASSEMBLY_DISABLE_EVEX) && defined(NMD_ASSEMBLY_DISABLE_VEX))
		else
#endif
		{
			// Check for immediate field
			if ((NMD_R(*b) == 0xE && NMD_C(*b) < 8) || (NMD_R(*b) == 0xB && NMD_C(*b) < 8) || NMD_R(*b) == 7 || (NMD_R(*b) < 4 && (NMD_C(*b) == 4 || NMD_C(*b) == 0xC)) || (*b == 0xF6 && !(*(b + 1) & 48)) || nmd_findByte(op1imm8, sizeof(op1imm8), *b)) //imm8
				instruction->immMask = NMD_X86_IMM8, offset++;
			else if (*b == 0xC2 || *b == 0xCA) //imm16
				instruction->immMask = NMD_X86_IMM16, offset += 2;
			else if (*b == 0xC8) //imm16 + imm8
				instruction->immMask = NMD_X86_IMM16 | NMD_X86_IMM8, offset += 3;
			else if ((NMD_R(*b) < 4 && (NMD_C(*b) == 5 || NMD_C(*b) == 0xD)) || (NMD_R(*b) == 0xB && NMD_C(*b) >= 8) || (*b == 0xF7 && !(*(b + 1) & 48)) || nmd_findByte(op1imm32, sizeof(op1imm32), *b)) //imm32,16
			{
				if (NMD_R(*b) == 0xB && NMD_C(*b) >= 8)
					instruction->immMask = instruction->prefixes & NMD_X86_REX_W_PREFIX ? NMD_X86_IMM64 : (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX || (instruction->mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)) ? NMD_X86_IMM16 : NMD_X86_IMM32);
				else
					instruction->immMask = (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX && instruction->mode == NMD_X86_MODE_32) || (instruction->mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)) ? NMD_X86_IMM16 : NMD_X86_IMM32;
				offset += instruction->immMask;
			}
			else if (*b == 0xEA || *b == 0x9A) //imm32,48
			{
				if ((instruction->mode == NMD_X86_MODE_64))
					return false;
				instruction->immMask = (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? NMD_X86_IMM32 : NMD_X86_IMM48), offset += instruction->immMask;
			}
			else if (NMD_R(*b) == 0xA && NMD_C(*b) < 4)
				instruction->immMask = (instruction->mode == NMD_X86_MODE_64) ? (instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX ? NMD_X86_IMM32 : NMD_X86_IMM64) : (instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX ? NMD_X86_IMM16 : NMD_X86_IMM32), offset += instruction->immMask;

#ifndef NMD_ASSEMBLY_DISABLE_NUM_OPERANDS
			// Parse the instruction's number of operands.
			if (featureFlags & NMD_X86_FEATURE_FLAGS_NUM_OPERANDS)
			{
				if (*b >= 0xd8 && *b <= 0xdf)
				{
					if (modrm.mod == 0b11)
					{
						if ((*b == 0xd9 && (NMD_R(modrm.modrm) == 0xc || (*b >= 0xc8 && *b <= 0xcf))) ||
							(*b == 0xda && NMD_R(modrm.modrm) <= 0xd) ||
							(*b == 0xdb && (NMD_R(modrm.modrm) <= 0xd || modrm.modrm >= 0xe8)) ||
							(*b == 0xde && modrm.modrm != 0xd9) ||
							(*b == 0xdf && modrm.modrm != 0xe0))
							instruction->numOperands = 2;
					}
					else
						instruction->numOperands = 1;
				}
				else if (*b == 0x69 || *b == 0x6b)
					instruction->numOperands = 3;
				else if ((NMD_R(*b) < 4 && *b % 8 < 6) || (NMD_R(*b) >= 8 && NMD_R(*b) <= 0xb && *b != 0x8f && *b != 0x90 && !(*b >= 0x98 && *b <= 0x9f)) || *b == 0x62 || *b == 0x63 || (*b >= 0x6c && *b <= 0x6f) || *b == 0xc0 || *b == 0xc1 || (*b >= 0xc4 && *b <= 0xc8) || (*b >= 0xd0 && *b <= 0xd3) || (NMD_R(*b) == 0xe && *b % 8 >= 4))
					instruction->numOperands = 2;
				else if (NMD_R(*b) == 4 || NMD_R(*b) == 5 || NMD_R(*b) == 7 || (*b == 0x68 || *b == 0x6a) || *b == 0x8f || *b == 0x9a || *b == 0xc2 || *b == 0xca || *b == 0xcd || *b == 0xd4 || *b == 0xd5 || (NMD_R(*b) == 0xe && *b % 8 <= 3) || (NMD_R(*b) == 0xf && *b % 8 >= 6))
					instruction->numOperands = 1;
			}
#endif

#ifndef NMD_ASSEMBLY_DISABLE_GROUP
			// Parse the instruction's group.
			if (featureFlags & NMD_X86_FEATURE_FLAGS_GROUP)
			{
				if (NMD_R(*b) == 7 || *b == 0xe3)
					instruction->group = NMD_GROUP_JUMP | NMD_GROUP_CONDITIONAL_BRANCH | NMD_GROUP_RELATIVE_ADDRESSING;
				else if (*b == 0xe9 || *b == 0xea || *b == 0xeb || (*b == 0xff && (modrm.rm == 0b100 || modrm.rm == 0b101)))
					instruction->group = NMD_GROUP_JUMP | NMD_GROUP_UNCONDITIONAL_BRANCH | (*b == 0xe9 || *b == 0xeb ? NMD_GROUP_RELATIVE_ADDRESSING : 0);
				else if (*b == 0x9a || *b == 0xe8 || (*b == 0xff && (modrm.rm == 0b010 || modrm.rm == 0b011)))
					instruction->group = NMD_GROUP_CALL | NMD_GROUP_UNCONDITIONAL_BRANCH | (*b == 0xe8 ? NMD_GROUP_RELATIVE_ADDRESSING : 0);
				else if (*b == 0xc2 || *b == 0xc3 || *b == 0xca || *b == 0xcb)
					instruction->group = NMD_GROUP_RET;
				else if (*b == 0xcf)
					instruction->group = NMD_GROUP_IRET;
				else if ((*b >= 0xcc && *b <= 0xce) || *b == 0xf1)
					instruction->group = NMD_GROUP_INT;
				else if (*b == 0xf4)
					instruction->group = NMD_GROUP_PRIVILEGE;
				else if (*b == 0xc7 && modrm.modrm == 0xf8)
					instruction->group = NMD_GROUP_UNCONDITIONAL_BRANCH | NMD_GROUP_RELATIVE_ADDRESSING;
				else if (*b >= 0xe0 && *b <= 0xe2)
					instruction->group = NMD_GROUP_CONDITIONAL_BRANCH | NMD_GROUP_RELATIVE_ADDRESSING;
			}
#endif

			// Check for ModR/M, SIB and displacement.
			if (nmd_findByte(op1modrm, sizeof(op1modrm), *b) || (NMD_R(*b) < 4 && (NMD_C(*b) < 4 || (NMD_C(*b) >= 8 && NMD_C(*b) < 0xC))) || NMD_R(*b) == 8 || (NMD_R(*b) == 0xD && NMD_C(*b) >= 8))
				parseModrm(&b, instruction);
		}
	}
	
	if (instruction->prefixes & NMD_X86_LOCK_PREFIX)
	{
		const uint8_t op = instruction->opcode;
		const uint8_t twoOpcodes[] = { 0xb0, 0xb1, 0xb3, 0xbb, 0xc0, 0xc1 };
		if (!(instruction->flags.hasModrm && instruction->modrm.mod != 0b11 &&
			((instruction->opcodeSize == 1 && (op == 0x86 || op == 0x87 || (NMD_R(op) < 4 && (op % 8) < 2 && op < 0x38) || ((op >= 0x80 && op <= 0x83) && instruction->modrm.reg != 0b111) || (op >= 0xfe && instruction->modrm.reg < 2) || ((op == 0xf6 || op == 0xf7) && (instruction->modrm.reg == 0b010 || instruction->modrm.reg == 0b011)))) ||
				(instruction->opcodeSize == 2 && (nmd_findByte(twoOpcodes, sizeof(twoOpcodes), op) || (op == 0xba && instruction->modrm.reg != 0b100) || (op == 0xc7 && instruction->modrm.reg == 0b001))))))
			return false;
	}

	instruction->length = (uint8_t)((ptrdiff_t)(++b + offset) - (ptrdiff_t)(buffer));
	for (int i = 0; i < instruction->length; i++)
		instruction->fullInstruction[i] = ((const uint8_t*)(buffer))[i];

	for (int i = 0; i < instruction->immMask; i++)
		((uint8_t*)(&instruction->immediate))[i] = b[i];

	instruction->flags.valid = true;

	return true;
}

//If there's was potential for a crash because of bad logic it won't crash, just print an empty string.
#define NMD_NULL ""

static const char* const reg8[]       = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
static const char* const reg8_x64[]   = { "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil" };
static const char* const reg16[]      = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
static const char* const reg32[]      = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" };
static const char* const reg64[]      = { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi" };
static const char* const regrx[]      = { "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };
static const char* const segmentReg[] = { "es", "cs", "ss", "ds", "fs", "gs" };

static const char* const conditionSuffixes[] = { "o", "no", "b", "ae", "e", "ne", "be", "a", "s", "ns", "p", "np", "l", "ge", "le", "g" };

static const char* const op1OpcodeMapMnemonics[]    = { "add", "adc", "and", "xor", "or", "sbb", "sub", "cmp" };
static const char* const opcodeExtensionsGrp1[]     = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
static const char* const opcodeExtensionsGrp2[]     = { "rol", "ror", "rcl", "rcr", "shl", "shr", "shl", "sar" };
static const char* const opcodeExtensionsGrp3[]     = { "test", "test", "not", "neg", "mul", "imul", "div", "idiv" };
static const char* const opcodeExtensionsGrp5[]     = { "inc", "dec", "call", "call far", "jmp", "jmp far", "push" };
static const char* const opcodeExtensionsGrp6[]     = { "sldt", "str", "lldt", "ltr", "verr", "verw" };
static const char* const opcodeExtensionsGrp7[]     = { "sgdt", "sidt", "lgdt", "lidt", "smsw", NMD_NULL, "lmsw", "invlpg" };
static const char* const opcodeExtensionsGrp7reg0[] = { "enclv", "vmcall", "vmlaunch", "vmresume", "vmxoff", "pconfig" };
static const char* const opcodeExtensionsGrp7reg1[] = { "monitor", "mwait", "clac", "stac", NMD_NULL, NMD_NULL, NMD_NULL, "encls" };
static const char* const opcodeExtensionsGrp7reg2[] = { "xgetbv", "xsetbv", NMD_NULL, NMD_NULL, "vmfunc", "xend", "xtest", "enclu" };
static const char* const opcodeExtensionsGrp7reg3[] = { "vmrun ", "vmmcall", "vmload ", "vmsave", "stgi", "clgi", "skinit eax", "invlpga " };
static const char* const opcodeExtensionsGrp7reg7[] = { "swapgs", "rdtscp", "monitorx", "mwaitx", "clzero ", "rdpru" };

static const char* const escapeOpcodesD8[]    = { "add", "mul", "com", "comp", "sub", "subr", "div", "divr" };
static const char* const escapeOpcodesD9[]    = { "ld", NMD_NULL, "st", "stp", "ldenv", "ldcw", "nstenv", "nstcw" };
static const char* const escapeOpcodesDA_DE[] = { "iadd", "imul", "icom", "icomp", "isub", "isubr", "idiv", "idivr" };
static const char* const escapeOpcodesDB[]    = { "ild", "isttp", "ist", "istp", NMD_NULL, "ld", NMD_NULL, "stp" };
static const char* const escapeOpcodesDC[]    = { "add", "mul", "com", "comp", "sub", "subr", "div", "divr" };
static const char* const escapeOpcodesDD[]    = { "ld", "isttp", "st", "stp", "rstor", NMD_NULL, "nsave", "nstsw" };
static const char* const escapeOpcodesDF[]    = { "ild", "isttp", "ist", "istp", "bld", "ild", "bstp", "istp" };
static const char* const* escapeOpcodes[]     = { escapeOpcodesD8, escapeOpcodesD9, escapeOpcodesDA_DE, escapeOpcodesDB, escapeOpcodesDC, escapeOpcodesDD, escapeOpcodesDA_DE, escapeOpcodesDF };

typedef struct StringInfo
{
	char* buffer;
	const NMD_X86Instruction* instruction;
	uint32_t formatFlags;
} StringInfo;

void appendString(StringInfo* const si, const char* source)
{
	while (*source)
		*si->buffer++ = *source++;
};

size_t getNumDigits(uint64_t n, bool hex)
{
	size_t numDigits = 0;
	while ((n /= (hex ? 16 : 10)) > 0)
		numDigits++;

	return numDigits;
}

void appendNumber(StringInfo* const si, uint64_t n)
{
	size_t numDigits = getNumDigits(n, si->formatFlags & NMD_X86_FORMAT_FLAGS_HEX);
	size_t bufferOffset = numDigits + 1;

	if (si->formatFlags & NMD_X86_FORMAT_FLAGS_HEX)
	{
		const bool condition = n > 9 || si->formatFlags & NMD_X86_FORMAT_FLAGS_ENFORCE_HEX_ID;
		if (si->formatFlags & NMD_X86_FORMAT_FLAGS_0X_PREFIX && condition)
			*si->buffer++ = '0', * si->buffer++ = 'x';

		const uint8_t baseChar = si->formatFlags & NMD_X86_FORMAT_FLAGS_HEX_LOWERCASE ? 0x57 : 0x37;
		do {
			size_t num = n % 16;
			*(si->buffer + numDigits--) = (char)(num > 9 ? baseChar + num : 0x30 + num);
		} while ((n /= 16) > 0);

		if (si->formatFlags & NMD_X86_FORMAT_FLAGS_H_SUFFIX && condition)
			*(si->buffer + bufferOffset++) = 'h';
	}
	else
	{
		do {
			*(si->buffer + numDigits--) = (char)(0x30 + n % 10);
		} while ((n /= 10) > 0);
	}

	si->buffer += bufferOffset;
}

void appendSignedNumber(StringInfo* const si, int64_t n, bool showPositiveSign)
{
	if (n >= 0)
	{
		if (showPositiveSign)
			*si->buffer++ = '+';

		appendNumber(si, n);
	}
	else
	{
		*si->buffer++ = '-';
		appendNumber(si, ~n + 1);
	}
}

void appendSignedNumberMemoryView(StringInfo* const si)
{
	appendNumber(si, (si->instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? 0xFF00 : (si->instruction->mode == NMD_X86_MODE_64 ? 0xFFFFFFFFFFFFFF00 : 0xFFFFFF00)) | si->instruction->immediate);
	if (si->formatFlags & NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_HEX)
	{
		*si->buffer++ = '(';
		appendSignedNumber(si, (int8_t)(si->instruction->immediate), false);
		*si->buffer++ = ')';
	}
	else if (si->formatFlags & NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_DEC)
	{
		*si->buffer++ = '(';
		const uint32_t previousMask = si->formatFlags;
		si->formatFlags &= ~NMD_X86_FORMAT_FLAGS_HEX;
		appendSignedNumber(si, (int8_t)(si->instruction->immediate), false);
		si->formatFlags = previousMask;
		*si->buffer++ = ')';
	}
}

void appendRelativeAddress8(StringInfo* const si)
{
	if (si->instruction->runtimeAddress == -1)
	{
		*si->buffer++ = '$';
		appendSignedNumber(si, (int64_t)((int8_t)(si->instruction->immediate) + (int8_t)(si->instruction->length)), true);
	}
	else
	{
		uint64_t n;
		if (si->instruction->mode == NMD_X86_MODE_64)
			n = (uint64_t)((int64_t)(si->instruction->runtimeAddress + si->instruction->length) + (int8_t)(si->instruction->immediate));
		else if (si->instruction->mode == NMD_X86_MODE_16)
			n = (uint16_t)((int16_t)(si->instruction->runtimeAddress + si->instruction->length) + (int8_t)(si->instruction->immediate));
		else
			n = (uint32_t)((int32_t)(si->instruction->runtimeAddress + si->instruction->length) + (int8_t)(si->instruction->immediate));
		appendNumber(si, n);
	}
}

void appendRelativeAddress16_32(StringInfo* const si)
{
	if (si->instruction->runtimeAddress == -1)
	{
		*si->buffer++ = '$';
		appendSignedNumber(si, (int64_t)((int32_t)(si->instruction->immediate) + (int32_t)(si->instruction->length)), true);
	}
	else
		appendNumber(si, ((si->instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX && si->instruction->mode == NMD_X86_MODE_32) || (si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)) ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & (si->instruction->mode == NMD_X86_MODE_64 ?
			(uint64_t)((uint64_t)((int64_t)(si->instruction->runtimeAddress + si->instruction->length) + (int32_t)(si->instruction->immediate))) :
			(uint64_t)((uint32_t)((int32_t)(si->instruction->runtimeAddress + si->instruction->length) + (int32_t)(si->instruction->immediate)))
			));
}

void appendModRmMemoryPrefix(StringInfo* const si, const char* addrSpecifierReg)
{
	if (si->formatFlags & NMD_X86_FORMAT_FLAGS_POINTER_SIZE)
	{
		appendString(si, addrSpecifierReg);
		appendString(si, " ptr ");
	}

	if (!(si->formatFlags & NMD_X86_FORMAT_FLAGS_ONLY_SEGMENT_OVERRIDE && !si->instruction->segmentOverride))
	{
		size_t i = 0;
		if (si->instruction->segmentOverride)
		{
			while (!(si->instruction->segmentOverride & (1 << i)))
				i++;
		}

		appendString(si, si->instruction->segmentOverride ? segmentReg[i] : ((si->instruction->modrm.mod == 0b01 || si->instruction->modrm.mod == 0b10) && si->instruction->modrm.rm == 0b101) ? "ss" : "ds");
		*si->buffer++ = ':';
	}
}

void appendModRm16Upper(StringInfo* const si)
{
	*si->buffer++ = '[';

	const char* addresses[] = { "bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx" };

	if (!(si->instruction->modrm.mod == 0b00 && si->instruction->modrm.rm == 0b110))
		appendString(si, addresses[si->instruction->modrm.rm]);

	if (si->instruction->dispMask != NMD_X86_DISP_NONE && (si->instruction->displacement != 0 || *(si->buffer - 1) == '['))
	{
		if (si->instruction->modrm.mod == 0b00 && si->instruction->modrm.rm == 0b110)
			appendNumber(si, si->instruction->displacement);
		else
		{
			const bool isNegative = si->instruction->displacement & (1 << (si->instruction->dispMask * 8 - 1));
			if (*(si->buffer - 1) != '[')
				*si->buffer++ = isNegative ? '-' : '+';

			if (isNegative)
			{
				const uint16_t mask = si->instruction->dispMask == 2 ? 0xFFFF : 0xFF;
				appendNumber(si, static_cast<uint64_t>(~si->instruction->displacement & mask) + 1);
			}
			else
				appendNumber(si, si->instruction->displacement);
		}
	}

	*si->buffer++ = ']';
}


void appendModRm32Upper(StringInfo* const si)
{
	*si->buffer++ = '[';

	if (si->instruction->flags.hasSIB)
	{
		if (si->instruction->sib.base == 0b101)
		{
			if (si->instruction->modrm.mod != 0b00)
				appendString(si, si->instruction->mode == NMD_X86_MODE_64 && !(si->instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX) ? (si->instruction->prefixes & NMD_X86_REX_B_PREFIX ? "r13" : "rbp") : "ebp");
		}
		else
			appendString(si, (si->instruction->mode == NMD_X86_MODE_64 && !(si->instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX) ? (si->instruction->prefixes & NMD_X86_REX_B_PREFIX ? regrx : reg64) : reg32)[si->instruction->sib.base]);

		if (si->instruction->sib.index != 0b100)
		{
			if (!(si->instruction->sib.base == 0b101 && si->instruction->modrm.mod == 0b00))
				*si->buffer++ = '+';
			appendString(si, (si->instruction->mode == NMD_X86_MODE_64 && !(si->instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX) ? (si->instruction->prefixes & NMD_X86_REX_X_PREFIX ? regrx : reg64) : reg32)[si->instruction->sib.index]);
			if (!(si->instruction->sib.scale == 0b00 && !(si->formatFlags & NMD_X86_FORMAT_FLAGS_SCALE_ONE)))
				*si->buffer++ = '*', * si->buffer++ = (char)('0' + (1 << si->instruction->sib.scale));
		}

		if (si->instruction->prefixes & NMD_X86_REX_X_PREFIX && si->instruction->sib.index == 0b100)
		{
			if (*(si->buffer - 1) != '[')
				*si->buffer++ = '+';
			appendString(si, "r12");
			if (!(si->instruction->sib.scale == 0b00 && !(si->formatFlags & NMD_X86_FORMAT_FLAGS_SCALE_ONE)))
				*si->buffer++ = '*', * si->buffer++ = (char)('0' + (1 << si->instruction->sib.scale));
		}
	}
	else if (!(si->instruction->modrm.mod == 0b00 && si->instruction->modrm.rm == 0b101))
	{
		if ((si->instruction->prefixes & (NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX | NMD_X86_REX_B_PREFIX)) == (NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX | NMD_X86_REX_B_PREFIX) && si->instruction->mode == NMD_X86_MODE_64)
			appendString(si, regrx[si->instruction->modrm.rm]), * si->buffer++ = 'd';
		else
			appendString(si, (si->instruction->mode == NMD_X86_MODE_64 && !(si->instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX) ? (si->instruction->prefixes & NMD_X86_REX_B_PREFIX ? regrx : reg64) : reg32)[si->instruction->modrm.rm]);
	}

	//Handle displacement
	if (si->instruction->dispMask != NMD_X86_DISP_NONE && (si->instruction->displacement != 0 || *(si->buffer - 1) == '['))
	{
		//relative address
		if (si->instruction->modrm.rm == 0b101 && si->instruction->mode == NMD_X86_MODE_64 && si->instruction->modrm.mod == 0b00 && si->instruction->runtimeAddress != -1)
		{
			if (si->instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX)
				appendNumber(si, (uint32_t)((int32_t)(si->instruction->runtimeAddress + si->instruction->length) + (int32_t)si->instruction->displacement));
			else
				appendNumber(si, (int64_t)(si->instruction->runtimeAddress + si->instruction->length) + (int64_t)((int32_t)si->instruction->displacement));
		}
		else if (si->instruction->modrm.mod == 0b00 && ((si->instruction->sib.base == 0b101 && si->instruction->sib.index == 0b100) || si->instruction->modrm.rm == 0b101) && *(si->buffer - 1) == '[')
			appendNumber(si, si->instruction->mode == NMD_X86_MODE_64 ? 0xFFFFFFFF00000000 | si->instruction->displacement : si->instruction->displacement);
		else
		{
			if (si->instruction->modrm.rm == 0b101 && si->instruction->mode == NMD_X86_MODE_64 && si->instruction->modrm.mod == 0b00)
				appendString(si, si->instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX ? "eip" : "rip");

			const bool isNegative = si->instruction->displacement & (1 << (si->instruction->dispMask * 8 - 1));
			if (*(si->buffer - 1) != '[')
				*si->buffer++ = isNegative ? '-' : '+';

			if (isNegative)
			{
				const uint32_t mask = si->instruction->dispMask == 4 ? -1 : (1 << (si->instruction->dispMask * 8)) - 1;
				appendNumber(si, static_cast<uint64_t>(~si->instruction->displacement & mask) + 1);
			}
			else
				appendNumber(si, si->instruction->displacement);
		}
	}

	*si->buffer++ = ']';
}

void appendModRmUpper(StringInfo* const si, const char* addrSpecifierReg)
{
	appendModRmMemoryPrefix(si, addrSpecifierReg);

	if ((si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX)) || (si->instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX && si->instruction->mode == NMD_X86_MODE_32))
		appendModRm16Upper(si);
	else
		appendModRm32Upper(si);
}

void appendModRmUpperWithoutAddressSpecifier(StringInfo* const si)
{
	if ((si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX)) || (si->instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX && si->instruction->mode == NMD_X86_MODE_32))
		appendModRm16Upper(si);
	else
		appendModRm32Upper(si);
}

void appendNq(StringInfo* const si)
{
	*si->buffer++ = 'm', * si->buffer++ = 'm';
	*si->buffer++ = (char)(0x30 + si->instruction->modrm.rm);
}

void appendPq(StringInfo* const si)
{
	*si->buffer++ = 'm', * si->buffer++ = 'm';
	*si->buffer++ = (char)(0x30 + si->instruction->modrm.reg);
}

void appendAvxRegisterReg(StringInfo* const si)
{
	*si->buffer++ = si->instruction->vex.L ? 'y' : 'x';
	appendPq(si);
}

void appendAvxVvvvRegister(StringInfo* const si)
{
	*si->buffer++ = si->instruction->vex.L ? 'y' : 'x';
	*si->buffer++ = 'm', * si->buffer++ = 'm';
	if ((15 - si->instruction->vex.vvvv) > 9)
		*si->buffer++ = '1', * si->buffer++ = (char)(0x26 + (15 - si->instruction->vex.vvvv));
	else
		*si->buffer++ = (char)(0x30 + (15 - si->instruction->vex.vvvv));
}

void appendVdq(StringInfo* const si)
{
	*si->buffer++ = 'x';
	appendPq(si);
}

void appendVqq(StringInfo* const si)
{
	*si->buffer++ = 'y';
	appendPq(si);
}

void appendVx(StringInfo* const si)
{
	if (si->instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
		appendVdq(si);
	else
		appendVqq(si);
}

void appendUdq(StringInfo* const si)
{
	*si->buffer++ = 'x';
	appendNq(si);
}

void appendUqq(StringInfo* const si)
{
	*si->buffer++ = 'y';
	appendNq(si);
}

void appendUx(StringInfo* const si)
{
	if (si->instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
		appendUdq(si);
	else
		appendUqq(si);
}

void appendQq(StringInfo* const si)
{
	if (si->instruction->modrm.mod == 0b11)
		appendNq(si);
	else
		appendModRmUpper(si, "qword");
}

void appendEv(StringInfo* const si)
{
	if (si->instruction->modrm.mod == 0b11)
	{
		if (si->instruction->prefixes & NMD_X86_REX_B_PREFIX)
		{
			appendString(si, regrx[si->instruction->modrm.rm]);
			if (!(si->instruction->prefixes & NMD_X86_REX_W_PREFIX))
				*si->buffer++ = 'd';
		}
		else
			appendString(si, ((si->instruction->flags.operandSize64 ? reg64 : (si->instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX && si->instruction->mode != NMD_X86_MODE_16) || (si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)) ? reg16 : reg32))[si->instruction->modrm.rm]);
	}
	else
		appendModRmUpper(si, (si->instruction->flags.operandSize64) ? "qword" : ((si->instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX && si->instruction->mode != NMD_X86_MODE_16) || (si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)) ? "word" : "dword"));
}

void appendEy(StringInfo* const si)
{
	if (si->instruction->modrm.mod == 0b11)
		appendString(si, (si->instruction->mode == NMD_X86_MODE_64 ? reg64 : reg32)[si->instruction->modrm.rm]);
	else
		appendModRmUpper(si, si->instruction->mode == NMD_X86_MODE_64 ? "qword" : "dword");
}

void appendEb(StringInfo* const si)
{
	if (si->instruction->modrm.mod == 0b11)
	{
		if (si->instruction->prefixes & NMD_X86_REX_B_PREFIX)
			appendString(si, regrx[si->instruction->modrm.rm]), * si->buffer++ = 'b';
		else
			appendString(si, (si->instruction->prefixes & NMD_X86_REX_PREFIX ? reg8_x64 : reg8)[si->instruction->modrm.rm]);
	}
	else
		appendModRmUpper(si, "byte");
}

void appendEw(StringInfo* const si)
{
	if (si->instruction->modrm.mod == 0b11)
		appendString(si, reg16[si->instruction->modrm.rm]);
	else
		appendModRmUpper(si, "word");
}

void appendEd(StringInfo* const si)
{
	if (si->instruction->modrm.mod == 0b11)
		appendString(si, reg32[si->instruction->modrm.rm]);
	else
		appendModRmUpper(si, "dword");
}

void appendEq(StringInfo* const si)
{
	if (si->instruction->modrm.mod == 0b11)
		appendString(si, reg64[si->instruction->modrm.rm]);
	else
		appendModRmUpper(si, "qword");
}

void appendRv(StringInfo* const si)
{
	appendString(si, (si->instruction->flags.operandSize64 ? reg64 : (si->instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? reg16 : reg32))[si->instruction->modrm.rm]);
}

void appendGv(StringInfo* const si)
{
	if (si->instruction->prefixes & NMD_X86_REX_R_PREFIX)
	{
		appendString(si, regrx[si->instruction->modrm.reg]);
		if (!(si->instruction->prefixes & NMD_X86_REX_W_PREFIX))
			*si->buffer++ = 'd';
	}
	else
		appendString(si, ((si->instruction->flags.operandSize64) ? reg64 : ((si->instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX && si->instruction->mode != NMD_X86_MODE_16) || (si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)) ? reg16 : reg32))[si->instruction->modrm.reg]);
}

void appendGy(StringInfo* const si)
{
	appendString(si, (si->instruction->mode == NMD_X86_MODE_64 ? reg64 : reg32)[si->instruction->modrm.reg]);
}

void appendGb(StringInfo* const si)
{
	if (si->instruction->prefixes & NMD_X86_REX_R_PREFIX)
		appendString(si, regrx[si->instruction->modrm.reg]), * si->buffer++ = 'b';
	else
		appendString(si, (si->instruction->prefixes & NMD_X86_REX_PREFIX ? reg8_x64 : reg8)[si->instruction->modrm.reg]);
}

void appendGw(StringInfo* const si)
{
	appendString(si, reg16[si->instruction->modrm.reg]);
}

void appendW(StringInfo* const si)
{
	if (si->instruction->modrm.mod == 0b11)
		appendString(si, "xmm"), * si->buffer++ = (char)(0x30 + si->instruction->modrm.rm);
	else
		appendModRmUpper(si, "xmmword");
}

//Constructs a string from a pointer to variable of type 'NMD_X86Instruction'.
//This function may cause a crash if you modify 'instruction' manually.
//Parameters:
//	instruction [in]  A pointer to a variable of type 'NMD_X86Instruction'.
//	buffer      [out] A pointer to buffer that receives the string.
//  formatFlags [in]  A mask of 'NMD_X86_FORMAT_FLAGS_XXX' specifying how the string should be constructed.
void nmd_x86_construct_string(const NMD_X86Instruction* const instruction, char* const buffer, uint32_t formatFlags)
{
	if (!instruction->flags.valid)
		return;

	StringInfo si;
	si.buffer = buffer;
	si.instruction = instruction;
	si.formatFlags = formatFlags;

	const uint8_t op = instruction->opcode;

	if (instruction->prefixes & (NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX) && ((instruction->prefixes & NMD_X86_LOCK_PREFIX || ((op == 0x86 || op == 0x87) && instruction->modrm.mod != 0b11))))
		appendString(&si, instruction->flags.repeatPrefix ? "xrelease " : "xacquire ");
	else if (instruction->prefixes & NMD_X86_REPEAT_NOT_ZERO_PREFIX && (instruction->opcodeSize == 1 && (op == 0xc2 || op == 0xc3 || op == 0xe8 || op == 0xe9 || NMD_R(op) == 7 || (op == 0xff && (instruction->modrm.reg == 0b010 || instruction->modrm.reg == 0b100)))))
		appendString(&si, "bnd ");

	if (instruction->prefixes & NMD_X86_LOCK_PREFIX)
		appendString(&si, "lock ");

	//const bool operandSize = (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX && instruction->mode != NMD_X86_MODE_32) || (instruction->mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX));
	const bool operandSize = (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX) || (instruction->mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX));

	if (instruction->opcodeMap == NMD_X86_OPCODE_MAP_DEFAULT)
	{
#ifndef NMD_ASSEMBLY_DISABLE_EVEX
		if (instruction->encoding == NMD_X86_INSTRUCTION_ENCODING_EVEX)
		{

		}
#endif

#if !defined(NMD_ASSEMBLY_DISABLE_EVEX) && !defined(NMD_ASSEMBLY_DISABLE_VEX)
		else
#endif
#ifndef NMD_ASSEMBLY_DISABLE_VEX
			if (instruction->encoding == NMD_X86_INSTRUCTION_ENCODING_VEX)
			{
				if (instruction->vex.byte0 == 0xc4)
				{
					if (instruction->opcode == 0x0d)
					{
						appendString(&si, "vblendpd ");

						appendAvxRegisterReg(&si);
						*si.buffer++ = ',';

						appendAvxVvvvRegister(&si);
						*si.buffer++ = ',';

						appendW(&si);
						*si.buffer++ = ',';

						appendNumber(&si, instruction->immediate);
					}
				}
			}
#endif

#if !(defined(NMD_ASSEMBLY_DISABLE_EVEX) && defined(NMD_ASSEMBLY_DISABLE_VEX))
			else //if (instruction->encoding == INSTRUCTION_ENCODING_LEGACY)
#endif
			{
				if (NMD_R(op) < 4 && (NMD_C(op) < 6 || (NMD_C(op) >= 8 && NMD_C(op) < 0xE)))
				{
					appendString(&si, op1OpcodeMapMnemonics[NMD_R((NMD_C(op) > 6 ? op + 0x40 : op))]);
					*si.buffer++ = ' ';

					switch (op % 8)
					{
					case 0:
						appendEb(&si);
						*si.buffer++ = ',';
						appendGb(&si);
						break;
					case 1:
						appendEv(&si);
						*si.buffer++ = ',';
						appendGv(&si);
						break;
					case 2:
						appendGb(&si);
						*si.buffer++ = ',';
						appendEb(&si);
						break;
					case 3:
						appendGv(&si);
						*si.buffer++ = ',';
						appendEv(&si);
						break;
					case 4:
						appendString(&si, "al,");
						appendNumber(&si, instruction->immediate);
						break;
					case 5:
						appendString(&si, instruction->flags.operandSize64 ? "rax" : (operandSize ? "ax" : "eax"));
						*si.buffer++ = ',';
						appendNumber(&si, instruction->immediate);
						break;
					}
				}
				else if (NMD_R(op) == 4 || NMD_R(op) == 5) //Parse all one byte opcodes in the interval [0x40, 0x60[
				{
					appendString(&si, NMD_C(op) < 8 ? (NMD_R(op) == 4 ? "inc " : "push ") : (NMD_R(op) == 4 ? "dec " : "pop "));
					appendString(&si, (instruction->prefixes & NMD_X86_REX_B_PREFIX ? regrx : (instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX) ? reg64 : (operandSize ? reg16 : reg32)))[op % 8]);
				}
				else if (op == 0x62)
				{
					appendString(&si, "bound ");
					appendGv(&si);
					*si.buffer++ = ',';
					appendModRmUpper(&si, operandSize ? "dword" : "qword");
				}
				else if (op == 0x63)
				{
					if (instruction->mode == NMD_X86_MODE_64)
					{
						appendString(&si, "movsxd ");
						appendString(&si, (instruction->mode == NMD_X86_MODE_64 ? (instruction->prefixes & NMD_X86_REX_R_PREFIX ? regrx : reg64) : (operandSize ? reg16 : reg32))[instruction->modrm.reg]);
						*si.buffer++ = ',';
						if (instruction->modrm.mod == 0b11)
						{
							if (instruction->prefixes & NMD_X86_REX_B_PREFIX)
								appendString(&si, regrx[instruction->modrm.rm]), * si.buffer++ = 'd';
							else
								appendString(&si, ((instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX && instruction->mode == NMD_X86_MODE_32) || (instruction->mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)) ? reg16 : reg32)[instruction->modrm.rm]);
						}
						else
							appendModRmUpper(&si, (instruction->flags.operandSize64 && !(instruction->prefixes & NMD_X86_REX_W_PREFIX)) ? "qword" : ((instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX && instruction->mode == NMD_X86_MODE_32) || (instruction->mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)) ? "word" : "dword"));
					}
					else
					{
						appendString(&si, "arpl ");
						appendEw(&si);
						*si.buffer++ = ',';
						appendGw(&si);
					}
				}
				else if (op == 0x68 || op == 0x6A)
				{
					appendString(&si, "push ");
					if (op == 0x6a)
					{
						if (formatFlags & NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW && instruction->immediate >= 0x80)
							appendSignedNumberMemoryView(&si);
						else
							appendSignedNumber(&si, (int8_t)instruction->immediate, false);
					}
					else
						appendNumber(&si, instruction->immediate);
				}
				else if (op == 0x69 || op == 0x6B)
				{
					appendString(&si, "imul ");
					appendGv(&si);
					*si.buffer++ = ',';
					appendEv(&si);
					*si.buffer++ = ',';
					if (op == 0x6b)
					{
						if (si.formatFlags & NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW && instruction->immediate >= 0x80)
							appendSignedNumberMemoryView(&si);
						else
							appendSignedNumber(&si, (int8_t)instruction->immediate, false);
					}
					else
						appendNumber(&si, instruction->immediate);
				}
				else if (NMD_R(op) == 7)
				{
					*si.buffer++ = 'j';
					appendString(&si, conditionSuffixes[NMD_C(op)]);
					*si.buffer++ = ' ';

					appendRelativeAddress8(&si);
				}
				else if (op >= 0x80 && op < 0x84) // [80,83]
				{
					appendString(&si, opcodeExtensionsGrp1[instruction->modrm.reg]);
					*si.buffer++ = ' ';
					if (op == 0x80 || op == 0x82)
						appendEb(&si);
					else
						appendEv(&si);
					*si.buffer++ = ',';
					if (op == 0x83)
					{
						if ((instruction->modrm.reg == 0b001 || instruction->modrm.reg == 0b100 || instruction->modrm.reg == 0b110) && instruction->immediate >= 0x80)
							appendNumber(&si, (instruction->prefixes & NMD_X86_REX_W_PREFIX ? 0xFFFFFFFFFFFFFF00 : (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX || instruction->mode == NMD_X86_MODE_16 ? 0xFF00 : 0xFFFFFF00)) | instruction->immediate);
						else
							appendSignedNumber(&si, (int8_t)(instruction->immediate), false);
					}
					else
						appendNumber(&si, instruction->immediate);
				}
				else if (op >= 0x84 && op <= 0x87)
				{
					appendString(&si, op > 0x85 ? "xchg " : "test ");
					if (op % 2 == 0)
					{
						appendEb(&si);
						*si.buffer++ = ',';
						appendGb(&si);
					}
					else
					{
						appendEv(&si);
						*si.buffer++ = ',';
						appendGv(&si);
					}
				}
				else if (op >= 0x88 && op <= 0x8e && op != 0x8d)
				{
					appendString(&si, "mov ");
					if (op == 0x88)
					{
						appendEb(&si);
						*si.buffer++ = ',';
						appendGb(&si);
					}
					else if (op == 0x89)
					{
						appendEv(&si);
						*si.buffer++ = ',';
						appendGv(&si);
					}
					else if (op == 0x8a)
					{
						appendGb(&si);
						*si.buffer++ = ',';
						appendEb(&si);
					}
					else if (op == 0x8b)
					{
						appendGv(&si);
						*si.buffer++ = ',';
						appendEv(&si);
					}
					else if (op == 0x8c)
					{
						if (si.instruction->modrm.mod == 0b11)
							appendString(&si, (si.instruction->flags.operandSize64 ? reg64 : (si.instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX || instruction->mode == NMD_X86_MODE_16 ? reg16 : reg32))[si.instruction->modrm.rm]);
						else
							appendModRmUpper(&si, "word");

						*si.buffer++ = ',';
						appendString(&si, segmentReg[instruction->modrm.reg]);
					}
					else if (op == 0x8e)
					{
						appendString(&si, segmentReg[instruction->modrm.reg]);
						*si.buffer++ = ',';
						appendEw(&si);
					}
				}
				else if (op == 0x8d)
				{
					appendString(&si, "lea ");
					appendGv(&si);
					*si.buffer++ = ',';
					appendModRmUpperWithoutAddressSpecifier(&si);
				}
				else if (op == 0x8f)
				{
					appendString(&si, "pop ");
					if (instruction->modrm.mod == 0b11)
						appendString(&si, (operandSize ? reg16 : reg32)[instruction->modrm.rm]);
					else
						appendModRmUpper(&si, instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX) ? "qword" : (operandSize ? "word" : "dword"));
				}
				else if (op > 0x90 && op <= 0x97)
				{
					appendString(&si, "xchg ");
					if (instruction->prefixes & NMD_X86_REX_B_PREFIX)
					{
						appendString(&si, regrx[NMD_C(op)]);
						if (!(instruction->prefixes & NMD_X86_REX_W_PREFIX))
							*si.buffer++ = 'd';
					}
					else
						appendString(&si, (instruction->prefixes & NMD_X86_REX_W_PREFIX ? reg64 : (operandSize ? reg16 : reg32))[NMD_C(op)]);
					appendString(&si, (instruction->prefixes & NMD_X86_REX_W_PREFIX ? ",rax" : (operandSize ? ",ax" : ",eax")));
				}
				else if (op >= 0xA0 && op < 0xA4)
				{
					appendString(&si, "mov ");

					if (op == 0xa0)
					{
						appendString(&si, "al,");
						appendModRmMemoryPrefix(&si, "byte");
						*si.buffer++ = '[';
						appendNumber(&si, (instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX || instruction->mode == NMD_X86_MODE_16 ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & instruction->immediate);
						*si.buffer++ = ']';
					}
					else if (op == 0xa1)
					{
						appendString(&si, instruction->flags.operandSize64 ? "rax," : (operandSize ? "ax," : "eax,"));
						appendModRmMemoryPrefix(&si, instruction->flags.operandSize64 ? "qword" : (operandSize ? "word" : "dword"));
						*si.buffer++ = '[';
						appendNumber(&si, (instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX || instruction->mode == NMD_X86_MODE_16 ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & instruction->immediate);
						*si.buffer++ = ']';
					}
					else if (op == 0xa2)
					{
						appendModRmMemoryPrefix(&si, "byte");
						*si.buffer++ = '[';
						appendNumber(&si, (instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX || instruction->mode == NMD_X86_MODE_16 ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & instruction->immediate);
						appendString(&si, "],al");
					}
					else if (op == 0xa3)
					{
						appendModRmMemoryPrefix(&si, instruction->flags.operandSize64 ? "qword" : (operandSize ? "word" : "dword"));
						*si.buffer++ = '[';
						appendNumber(&si, (instruction->prefixes & NMD_X86_ADDRESS_SIZE_OVERRIDE_PREFIX || instruction->mode == NMD_X86_MODE_16 ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & instruction->immediate);
						appendString(&si, "],");
						appendString(&si, instruction->flags.operandSize64 ? "rax" : (operandSize ? "ax" : "eax"));
					}
				}
				else if (op == 0x9A)
				{
					appendString(&si, "call far ");
					appendNumber(&si, (uint64_t)(*(uint16_t*)((char*)(&instruction->immediate) + (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? 2 : 4))));
					*si.buffer++ = ':';
					appendNumber(&si, (uint64_t)(operandSize ? *((uint16_t*)(&instruction->immediate)) : *((uint32_t*)(&instruction->immediate))));
				}
				else if (op == 0xa8)
				{
					appendString(&si, "test al,");
					appendNumber(&si, instruction->immediate);
				}
				else if (op == 0xa9)
				{
					appendString(&si, instruction->flags.operandSize64 ? "test rax" : (operandSize ? "test ax" : "test eax"));
					*si.buffer++ = ',';
					appendNumber(&si, instruction->immediate);
				}
				else if ((op >= 0x6c && op <= 0x6f) || (op >= 0xa4 && op <= 0xa7) || (op >= 0xaa && op <= 0xaf))
				{
					if (instruction->prefixes & NMD_X86_REPEAT_PREFIX)
						appendString(&si, "rep ");
					else if (instruction->prefixes & NMD_X86_REPEAT_NOT_ZERO_PREFIX)
						appendString(&si, "repne ");

					const char* str = 0;
					switch (op)
					{
					case 0x6c: case 0x6d: str = "ins"; break;
					case 0x6e: case 0x6f: str = "outs"; break;
					case 0xa4: case 0xa5: str = "movs"; break;
					case 0xa6: case 0xa7: str = "cmps"; break;
					case 0xaa: case 0xab: str = "stos"; break;
					case 0xac: case 0xad: str = "lods"; break;
					case 0xae: case 0xaf: str = "scas"; break;
					}
					appendString(&si, str);
					*si.buffer++ = (op % 2 == 0) ? 'b' : (operandSize ? 'w' : 'd');
				}
				else if (NMD_R(op) == 0xb)
				{
					appendString(&si, "mov ");
					if (instruction->prefixes & NMD_X86_REX_B_PREFIX)
						appendString(&si, regrx[op % 8]), * si.buffer++ = NMD_C(op) < 8 ? 'b' : 'd';
					else
						appendString(&si, (NMD_C(op) < 8 ? (instruction->prefixes & NMD_X86_REX_PREFIX ? reg8_x64 : reg8) : (instruction->flags.operandSize64 ? reg64 : (operandSize ? reg16 : reg32)))[op % 8]);
					*si.buffer++ = ',';
					appendNumber(&si, instruction->immediate);
				}
				else if (op == 0xC0 || op == 0xC1 || (NMD_R(op) == 0xd && NMD_C(op) < 4))
				{
					appendString(&si, opcodeExtensionsGrp2[instruction->modrm.reg]);
					*si.buffer++ = ' ';
					if (op % 2 == 0)
						appendEb(&si);
					else
						appendEv(&si);
					*si.buffer++ = ',';
					if (NMD_R(op) == 0xc)
						appendNumber(&si, instruction->immediate);
					else if (NMD_C(op) < 2)
						appendNumber(&si, 1);
					else
						appendString(&si, "cl");
				}
				else if (op == 0xc2)
				{
					appendString(&si, "ret ");
					appendNumber(&si, instruction->immediate);
				}
				else if (op == 0xc4 || op == 0xc5)
				{
					appendString(&si, op == 0xc4 ? "les" : "lds");
					*si.buffer++ = ' ';
					appendGv(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.mod == 0b11)
						appendString(&si, (si.instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? reg16 : reg32)[si.instruction->modrm.rm]);
					else
						appendModRmUpper(&si, si.instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? "dword" : "fword");
				}
				else if (op == 0xc6 || op == 0xc7)
				{
					appendString(&si, instruction->modrm.reg == 0b000 ? "mov " : (op == 0xc6 ? "xabort " : "xbegin "));
					if (instruction->modrm.reg == 0b111)
					{
						if (op == 0xc6)
							appendNumber(&si, instruction->immediate);
						else
							appendRelativeAddress16_32(&si);
					}
					else
					{
						if (op == 0xc6)
							appendEb(&si);
						else
							appendEv(&si);
						*si.buffer++ = ',';
						appendNumber(&si, instruction->immediate);
					}
				}
				else if (op == 0xc8)
				{
					appendString(&si, "enter ");
					appendNumber(&si, (uint64_t)(*(uint16_t*)(&instruction->immediate)));
					*si.buffer++ = ',';
					appendNumber(&si, (uint64_t)(*((uint8_t*)(&instruction->immediate) + 2)));
				}
				else if (op == 0xca)
				{
					appendString(&si, instruction->prefixes & NMD_X86_REX_W_PREFIX ? "retfq " : "retf ");
					appendNumber(&si, instruction->immediate);
				}
				else if (op == 0xcd)
				{
					appendString(&si, "int ");
					appendNumber(&si, instruction->immediate);
				}
				else if (op == 0xd4)
				{
					appendString(&si, "aam ");
					appendNumber(&si, instruction->immediate);
				}
				else if (op == 0xd5)
				{
					appendString(&si, "aad ");
					appendNumber(&si, instruction->immediate);
				}
				else if (op >= 0xd8 && op <= 0xdf)
				{
					*si.buffer++ = 'f';

					if (instruction->modrm.modrm < 0xc0)
					{
						appendString(&si, escapeOpcodes[NMD_C(op) - 8][instruction->modrm.reg]);
						*si.buffer++ = ' ';
						switch (op)
						{
						case 0xd8: case 0xda: appendModRmUpper(&si, "dword"); break;
						case 0xd9: appendModRmUpper(&si, instruction->modrm.reg & 0b100 ? (instruction->modrm.reg & 0b001 ? "word" : (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? "m14" : "m28")) : "dword"); break;
						case 0xdb: appendModRmUpper(&si, instruction->modrm.reg & 0b100 ? "tbyte" : "dword"); break;
						case 0xdc: appendModRmUpper(&si, "qword"); break;
						case 0xdd: appendModRmUpper(&si, instruction->modrm.reg & 0b100 ? ((instruction->modrm.reg & 0b111) == 0b111 ? "word" : "byte") : "qword"); break;
						case 0xde: appendModRmUpper(&si, "word"); break;
						case 0xdf: appendModRmUpper(&si, instruction->modrm.reg & 0b100 ? (instruction->modrm.reg & 0b001 ? "qword" : "tbyte") : "word"); break;
						}
					}
					else
					{
						switch (op)
						{
						case 0xd8:
							appendString(&si, escapeOpcodesD8[(NMD_R(instruction->modrm.modrm) - 0xc) * 2 + (NMD_C(instruction->modrm.modrm) > 7 ? 1 : 0)]);
							appendString(&si, " st(0),st(");
							*si.buffer++ = (char)(0x30 + instruction->modrm.modrm % 8), * si.buffer++ = ')';
							break;
						case 0xd9:
							if (NMD_R(instruction->modrm.modrm) == 0xc)
							{
								appendString(&si, NMD_C(instruction->modrm.modrm) < 8 ? "ld" : "xch");
								appendString(&si, " st(0),st(");
								*si.buffer++ = (char)(0x30 + instruction->modrm.modrm % 8), * si.buffer++ = ')';
							}
							else if (instruction->modrm.modrm >= 0xd8 && instruction->modrm.modrm <= 0xdf)
							{
								appendString(&si, "stpnce st(");
								*si.buffer++ = (char)(0x30 + instruction->modrm.modrm % 8);
								appendString(&si, "),st(0)");
							}
							else
							{
								const char* str = 0;
								switch (instruction->modrm.modrm)
								{
								case 0xd0: str = "nop"; break;
								case 0xe0: str = "chs"; break;
								case 0xe1: str = "abs"; break;
								case 0xe4: str = "tst"; break;
								case 0xe5: str = "xam"; break;
								case 0xe8: str = "ld1"; break;
								case 0xe9: str = "ldl2t"; break;
								case 0xea: str = "ldl2e"; break;
								case 0xeb: str = "ldpi"; break;
								case 0xec: str = "ldlg2"; break;
								case 0xed: str = "ldln2"; break;
								case 0xee: str = "ldz"; break;
								case 0xf0: str = "2xm1"; break;
								case 0xf1: str = "yl2x"; break;
								case 0xf2: str = "ptan"; break;
								case 0xf3: str = "patan"; break;
								case 0xf4: str = "xtract"; break;
								case 0xf5: str = "prem1"; break;
								case 0xf6: str = "decstp"; break;
								case 0xf7: str = "incstp"; break;
								case 0xf8: str = "prem"; break;
								case 0xf9: str = "yl2xp1"; break;
								case 0xfa: str = "sqrt"; break;
								case 0xfb: str = "sincos"; break;
								case 0xfc: str = "rndint"; break;
								case 0xfd: str = "scale"; break;
								case 0xfe: str = "sin"; break;
								case 0xff: str = "cos"; break;
								}
								appendString(&si, str);
							}
							break;
						case 0xda:
							if (instruction->modrm.modrm == 0xe9)
								appendString(&si, "ucompp");
							else
							{
								const char* mnemonics[4] = { "cmovb", "cmovbe", "cmove", "cmovu" };
								appendString(&si, mnemonics[(NMD_R(instruction->modrm.modrm) - 0xc) + (NMD_C(instruction->modrm.modrm) > 7 ? 2 : 0)]);
								appendString(&si, " st(0),st(");
								*si.buffer++ = (char)(0x30 + NMD_C(instruction->modrm.modrm) % 8);
								*si.buffer++ = ')';
							}
							break;
						case 0xdb:
							if (NMD_R(instruction->modrm.modrm) == 0xe && NMD_C(instruction->modrm.modrm) < 8)
							{
								const char* mnemonics[] = { "eni8087_nop", "disi8087_nop","nclex","ninit","setpm287_nop" };
								appendString(&si, mnemonics[NMD_C(instruction->modrm.modrm)]);
							}
							else
							{
								if (instruction->modrm.modrm >= 0xe0)
									appendString(&si, instruction->modrm.modrm < 0xf0 ? "ucomi" : "comi");
								else
								{
									appendString(&si, "cmovn");
									if (instruction->modrm.modrm < 0xc8)
										*si.buffer++ = 'b';
									else if (instruction->modrm.modrm < 0xd0)
										*si.buffer++ = 'e';
									else if (instruction->modrm.modrm >= 0xd8)
										*si.buffer++ = 'u';
									else
										appendString(&si, "be");
								}
								appendString(&si, " st(0),st(");
								*si.buffer++ = (char)(0x30 + NMD_C(instruction->modrm.modrm) % 8);
								*si.buffer++ = ')';
							}
							break;
						case 0xdc:
							if (NMD_R(instruction->modrm.modrm) == 0xc)
								appendString(&si, NMD_C(instruction->modrm.modrm) > 7 ? "mul" : "add");
							else
							{
								appendString(&si, NMD_R(instruction->modrm.modrm) == 0xd ? "com" : (NMD_R(instruction->modrm.modrm) == 0xe ? "subr" : "div"));
								if (NMD_R(instruction->modrm.modrm) == 0xd && NMD_C(instruction->modrm.modrm) >= 8)
								{
									if (NMD_R(instruction->modrm.modrm) >= 8)
										*si.buffer++ = 'p';
								}
								else
								{
									if (NMD_R(instruction->modrm.modrm) < 8)
										*si.buffer++ = 'r';
								}
							}

							if (NMD_R(instruction->modrm.modrm) == 0xd)
							{
								appendString(&si, " st(0),st(");
								*si.buffer++ = (char)(0x30 + NMD_C(instruction->modrm.modrm) % 8);
								*si.buffer++ = ')';
							}
							else
							{
								appendString(&si, " st(");
								*si.buffer++ = (char)(0x30 + NMD_C(instruction->modrm.modrm) % 8);
								appendString(&si, "),st(0)");
							}
							break;
						case 0xdd:
							if (NMD_R(instruction->modrm.modrm) == 0xc)
								appendString(&si, NMD_C(instruction->modrm.modrm) < 8 ? "free" : "xch");
							else
							{
								appendString(&si, instruction->modrm.modrm < 0xe0 ? "st" : "ucom");
								if (NMD_C(instruction->modrm.modrm) >= 8)
									*si.buffer++ = 'p';
							}

							appendString(&si, " st(");
							*si.buffer++ = (char)(0x30 + NMD_C(instruction->modrm.modrm) % 8);
							*si.buffer++ = ')';

							break;
						case 0xde:
							if (instruction->modrm.modrm == 0xd9)
								appendString(&si, "compp");
							else
							{
								if (instruction->modrm.modrm >= 0xd0 && instruction->modrm.modrm <= 0xd7)
								{
									appendString(&si, "comp st(0),st(");
									*si.buffer++ = (char)(0x30 + NMD_C(instruction->modrm.modrm) % 8);
									*si.buffer++ = ')';
								}
								else
								{
									if (NMD_R(instruction->modrm.modrm) == 0xc)
										appendString(&si, NMD_C(instruction->modrm.modrm) < 8 ? "add" : "mul");
									else
									{
										appendString(&si, instruction->modrm.modrm < 0xf0 ? "sub" : "div");
										if (NMD_R(instruction->modrm.modrm) < 8 || (NMD_R(instruction->modrm.modrm) >= 0xe && NMD_C(instruction->modrm.modrm) < 8))
											*si.buffer++ = 'r';
									}
									appendString(&si, "p st(");
									*si.buffer++ = (char)(0x30 + NMD_C(instruction->modrm.modrm) % 8);
									appendString(&si, "),st(0)");
								}
							}
							break;
						case 0xdf:
							if (instruction->modrm.modrm == 0xe0)
								appendString(&si, "nstsw ax");
							else
							{
								if (instruction->modrm.modrm >= 0xe8)
								{
									if (instruction->modrm.modrm < 0xf0)
										*si.buffer++ = 'u';
									appendString(&si, "comip");
									appendString(&si, " st(0),st(");
									*si.buffer++ = (char)(0x30 + NMD_C(instruction->modrm.modrm) % 8);
									*si.buffer++ = ')';
								}
								else
								{
									appendString(&si, instruction->modrm.modrm < 0xc8 ? "freep" : (instruction->modrm.modrm >= 0xd0 ? "stp" : "xch"));
									appendString(&si, " st(");
									*si.buffer++ = (char)(0x30 + NMD_C(instruction->modrm.modrm) % 8);
									*si.buffer++ = ')';
								}
							}

							break;
						}
					}
				}
				else if (op >= 0xe0 && op <= 0xe3)
				{
					const char* mnemonics[] = { "loopne", "loope", "loop", instruction->mode == NMD_X86_MODE_64 ? "jrcxz" : "jecxz" };
					appendString(&si, mnemonics[NMD_C(op)]);
					*si.buffer++ = ' ';
					appendRelativeAddress8(&si);
				}
				else if (op == 0xe4 || op == 0xe5)
				{
					appendString(&si, "in ");
					appendString(&si, op == 0xe4 ? "al" : (operandSize ? "ax" : "eax"));
					*si.buffer++ = ',';
					appendNumber(&si, instruction->immediate);
				}
				else if (op == 0xe6 || op == 0xe7)
				{
					appendString(&si, "out ");
					appendNumber(&si, instruction->immediate);
					*si.buffer++ = ',';
					appendString(&si, op == 0xe6 ? "al" : (operandSize ? "ax" : "eax"));
				}
				else if (op == 0xe8 || op == 0xe9 || op == 0xeb)
				{
					appendString(&si, op == 0xe8 ? "call " : "jmp ");
					if (op == 0xeb)
						appendRelativeAddress8(&si);
					else
						appendRelativeAddress16_32(&si);
				}
				else if (op == 0xea)
				{
					appendString(&si, "jmp far ");
					appendNumber(&si, (uint64_t)(*(uint16_t*)(((uint8_t*)(&instruction->immediate) + 4))));
					*si.buffer++ = ':';
					appendNumber(&si, (uint64_t)(*(uint32_t*)(&instruction->immediate)));
				}
				else if (op == 0xec || op == 0xed)
				{
					appendString(&si, "in ");
					appendString(&si, op == 0xec ? "al" : (operandSize ? "ax" : "eax"));
					appendString(&si, ",dx");
				}
				else if (op == 0xee || op == 0xef)
				{
					appendString(&si, "out dx,");
					appendString(&si, op == 0xee ? "al" : (operandSize ? "ax" : "eax"));
				}
				else if (op == 0xf6 || op == 0xf7)
				{
					appendString(&si, opcodeExtensionsGrp3[instruction->modrm.reg]);
					*si.buffer++ = ' ';
					if (op == 0xf6)
						appendEb(&si);
					else
						appendEv(&si);

					if (instruction->modrm.reg <= 0b001)
					{
						*si.buffer++ = ',';
						appendNumber(&si, instruction->immediate);
					}
				}
				else if (op == 0xfe)
				{
					appendString(&si, instruction->modrm.reg == 0b000 ? "inc " : "dec ");
					appendEb(&si);
				}
				else if (op == 0xff)
				{
					appendString(&si, opcodeExtensionsGrp5[instruction->modrm.reg]);
					*si.buffer++ = ' ';
					if (instruction->modrm.mod == 0b11)
						appendString(&si, (operandSize ? reg16 : reg32)[si.instruction->modrm.rm]);
					else
						appendModRmUpper(&si, (instruction->modrm.reg == 0b011 || instruction->modrm.reg == 0b101) ? "fword" : (instruction->mode == NMD_X86_MODE_64 && ((instruction->modrm.reg >= 0b010 && instruction->modrm.reg <= 0b110) || (instruction->prefixes & NMD_X86_REX_W_PREFIX && instruction->modrm.reg <= 0b010)) ? "qword" : (operandSize ? "word" : "dword")));
				}
				else //Try to parse all opcodes not parsed by the checks above
				{
					const char* str = 0;
					switch (instruction->opcode)
					{
					case 0xcc: str = "int3"; break;
					case 0xc3: str = "ret"; break;
					case 0x90:
						if (instruction->prefixes & NMD_X86_REPEAT_PREFIX)
							str = "pause";
						else if (instruction->prefixes & NMD_X86_REX_B_PREFIX)
							str = instruction->prefixes & NMD_X86_REX_W_PREFIX ? "xchg r8,rax" : "xchg r8d,eax";
						else
							str = "nop";
						break;
					case 0x9c: str = instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX) ? "pushfq" : (operandSize ? "pushf" : "pushfd"); break;
					case 0x9d: str = instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX) ? "popfq" : (operandSize ? "popf" : "popfd"); break;
					case 0x60:
					case 0x61:
						str = operandSize ? (instruction->opcode == 0x60 ? "pusha" : "popa") : (instruction->opcode == 0x60 ? "pushad" : "popad");
						break;
					case 0xcb: str = instruction->prefixes & NMD_X86_REX_W_PREFIX ? "retfq" : "retf"; break;
					case 0xc9: str = "leave"; break;
					case 0xf1: str = "int1"; break;
					case 0x06: str = "push es"; break;
					case 0x16: str = "push ss"; break;
					case 0x1e: str = "push ds"; break;
					case 0x0e: str = "push cs"; break;
					case 0x07: str = "pop es"; break;
					case 0x17: str = "pop ss"; break;
					case 0x1f: str = "pop ds"; break;
					case 0x27: str = "daa"; break;
					case 0x37: str = "aaa"; break;
					case 0x2f: str = "das"; break;
					case 0x3f: str = "aas"; break;
					case 0xd7: str = "xlat"; break;
					case 0x9b: str = "wait"; break;
					case 0xf4: str = "hlt"; break;
					case 0xf5: str = "cmc"; break;
					case 0x9e: str = "sahf"; break;
					case 0x9f: str = "lahf"; break;
					case 0xce: str = "into"; break;
					case 0xcf: str = (instruction->prefixes & NMD_X86_REX_W_PREFIX) ? "iretq" : (operandSize ? "iret" : "iretd"); break;
					case 0x98: str = (instruction->prefixes & NMD_X86_REX_W_PREFIX ? "cdqe" : (operandSize ? "cbw" : "cwde")); break;
					case 0x99: str = (instruction->prefixes & NMD_X86_REX_W_PREFIX ? "cqo" : (operandSize ? "cwd" : "cdq")); break;
					case 0xd6: str = "salc"; break;
					case 0xf8: str = "clc"; break;
					case 0xf9: str = "stc"; break;
					case 0xfa: str = "cli"; break;
					case 0xfb: str = "sti"; break;
					case 0xfc: str = "cld"; break;
					case 0xfd: str = "std"; break;
					default: return;
					}
					appendString(&si, str);
				}
			}
	}
	else if (instruction->opcodeMap == NMD_X86_OPCODE_MAP_0F)
	{
		if (op == 0x00)
		{
			appendString(&si, opcodeExtensionsGrp6[instruction->modrm.reg]);
			*si.buffer++ = ' ';
			if (NMD_R(instruction->modrm.modrm) == 0xc)
				appendEv(&si);
			else
				appendEw(&si);
		}
		else if (op == 0x01)
		{
			if (instruction->modrm.mod == 0b11)
			{
				if (instruction->modrm.reg == 0b000)
					appendString(&si, opcodeExtensionsGrp7reg0[instruction->modrm.rm]);
				else if (instruction->modrm.reg == 0b001)
					appendString(&si, opcodeExtensionsGrp7reg1[instruction->modrm.rm]);
				else if (instruction->modrm.reg == 0b010)
					appendString(&si, opcodeExtensionsGrp7reg2[instruction->modrm.rm]);
				else if (instruction->modrm.reg == 0b011)
				{
					appendString(&si, opcodeExtensionsGrp7reg3[instruction->modrm.rm]);
					if (instruction->modrm.rm == 0b000 || instruction->modrm.rm == 0b010 || instruction->modrm.rm == 0b111)
						appendString(&si, instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? "ax" : "eax");

					if (instruction->modrm.rm == 0b111)
						appendString(&si, ",ecx");
				}
				else if (instruction->modrm.reg == 0b100)
					appendString(&si, "smsw "), appendString(&si, (instruction->flags.operandSize64 ? reg64 : (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? reg16 : reg32))[instruction->modrm.rm]);
				else if (instruction->modrm.reg == 0b101)
				{
					if (instruction->prefixes & NMD_X86_REPEAT_PREFIX)
						appendString(&si, instruction->modrm.rm == 0b000 ? "setssbsy" : "saveprevssp");
					else
						appendString(&si, instruction->modrm.rm == 0b111 ? "wrpkru" : "rdpkru");
				}
				else if (instruction->modrm.reg == 0b110)
					appendString(&si, "lmsw "), appendString(&si, reg16[instruction->modrm.rm]);
				else if (instruction->modrm.reg == 0b111)
				{
					appendString(&si, opcodeExtensionsGrp7reg7[instruction->modrm.rm]);
					if (instruction->modrm.rm == 0b100)
						appendString(&si, instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? "ax" : "eax");
				}
			}
			else
			{
				if (instruction->modrm.reg == 0b101)
				{
					appendString(&si, "rstorssp ");
					appendModRmUpper(&si, "qword");
				}
				else
				{
					appendString(&si, opcodeExtensionsGrp7[instruction->modrm.reg]);
					*si.buffer++ = ' ';
					if (si.instruction->modrm.reg == 0b110)
						appendEw(&si);
					else
						appendModRmUpper(&si, si.instruction->modrm.reg == 0b111 ? "byte" : si.instruction->modrm.reg == 0b100 ? "word" : "fword");
				}
			}
		}
		else if (op == 0x02 || op == 0x03)
		{
			appendString(&si, op == 0x02 ? "lar" : "lsl");
			*si.buffer++ = ' ';
			appendGv(&si);
			*si.buffer++ = ',';
			if (si.instruction->modrm.mod == 0b11)
				appendString(&si, (operandSize ? reg16 : reg32)[si.instruction->modrm.rm]);
			else
				appendModRmUpper(&si, "word");
		}
		else if (op == 0x0d)
		{
			if (instruction->modrm.mod == 0b11)
			{
				appendString(&si, "nop ");
				appendString(&si, (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? reg16 : reg32)[instruction->modrm.rm]);
				*si.buffer++ = ',';
				appendString(&si, (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? reg16 : reg32)[instruction->modrm.reg]);
			}
			else
			{
				appendString(&si, "prefetch");
				if (instruction->modrm.reg == 0b001)
					*si.buffer++ = 'w';
				else if (instruction->modrm.reg == 0b010)
					appendString(&si, "wt1");

				*si.buffer++ = ' ';

				if (si.instruction->modrm.mod == 0b11)
					appendString(&si, (instruction->prefixes & NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? reg16 : reg32)[si.instruction->modrm.rm]);
				else
					appendModRmUpper(&si, "byte");
			}
		}
		else if (op >= 0x10 && op <= 0x17)
		{
			const char* noPrefixMnemonics[] = { "movups", "movups", "movlps", "movlps", "unpcklps", "unpckhps", "movhps", "movhps" };
			const char* prefix66Mnemonics[] = { "movupd", "movupd", "movlpd", "movlpd", "unpcklpd", "unpckhpd", "movhpd", "movhpd" };
			const char* prefixF3Mnemonics[] = { "movss", "movss", "movsldup", NMD_NULL, NMD_NULL, NMD_NULL, "movshdup" };
			const char* prefixF2Mnemonics[] = { "movsd", "movsd", "movddup" };
			if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
			{
				appendString(&si, prefix66Mnemonics[NMD_C(op)]);
				*si.buffer++ = ' ';

				switch (NMD_C(op))
				{
				case 0:
					appendVx(&si);
					*si.buffer++ = ',';
					appendW(&si);
					break;
				case 1:
					appendW(&si);
					*si.buffer++ = ',';
					appendVx(&si);
					break;
				case 2:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
					else
						appendModRmUpper(&si, "qword");
					break;
				case 6:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
					else
						appendModRmUpper(&si, "qword");
					break;
				default:
					break;
				}
			}
			else if (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX)
			{
				appendString(&si, prefixF3Mnemonics[NMD_C(op)]);
				*si.buffer++ = ' ';

				switch (NMD_C(op))
				{
				case 0:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
					else
						appendModRmUpper(&si, "dword");
					break;
				case 1:
					if (si.instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
					else
						appendModRmUpper(&si, "dword");
					*si.buffer++ = ',';
					appendVdq(&si);
					break;
				case 2:
				case 6:
					appendVdq(&si);
					*si.buffer++ = ',';
					appendW(&si);
					break;
				}
			}
			else if (instruction->simdPrefix == NMD_X86_REPEAT_NOT_ZERO_PREFIX)
			{
				appendString(&si, prefixF2Mnemonics[NMD_C(op)]);
				*si.buffer++ = ' ';

				switch (NMD_C(op))
				{
				case 0:
				case 2:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
					else
						appendModRmUpper(&si, "qword");
					break;
				case 1:
					if (si.instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
					else
						appendModRmUpper(&si, "qword");
					*si.buffer++ = ',';
					appendVdq(&si);
					break;
				}
			}
			else
			{
				if (op == 0x12 && instruction->modrm.mod == 0b11)
					appendString(&si, "movhlps");
				else if (op == 0x16 && instruction->modrm.mod == 0b11)
					appendString(&si, "movlhps");
				else
					appendString(&si, noPrefixMnemonics[NMD_C(op)]);
				*si.buffer++ = ' ';

				switch (NMD_C(op))
				{
				case 0:
					appendVdq(&si);
					*si.buffer++ = ',';
					appendW(&si);
					break;
				case 1:
					appendW(&si);
					*si.buffer++ = ',';
					appendVdq(&si);
					break;
				case 2:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
					else
						appendModRmUpper(&si, "qword");
					break;
				case 6:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
					else
						appendModRmUpper(&si, "qword");
					break;
				default:
					break;
				};

			}

			switch (NMD_C(op))
			{
			case 3:
			case 7:
				if (si.instruction->modrm.mod == 0b11)
					appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
				else
					appendModRmUpper(&si, "qword");
				*si.buffer++ = ',';
				appendVdq(&si);
				break;
			case 4:
			case 5:
				appendVdq(&si);
				*si.buffer++ = ',';
				appendW(&si);
				break;
			};
		}
		else if (op == 0x18)
		{
			if (instruction->modrm.mod == 0b11 || instruction->modrm.reg >= 0b100)
			{
				appendString(&si, "nop ");
				appendEv(&si);
			}
			else
			{
				if (instruction->modrm.reg == 0b000)
					appendString(&si, "prefetchnta");
				else
				{
					appendString(&si, "prefetcht");
					*si.buffer++ = (char)(0x30 + (instruction->modrm.reg - 1));
				}
				*si.buffer++ = ' ';

				appendEb(&si);
			}
		}
		else if (op == 0x19)
		{
			appendString(&si, "nop ");
			appendEv(&si);
			*si.buffer++ = ',';
			appendGv(&si);
		}
		else if (op == 0x1A)
		{
			if (instruction->modrm.mod == 0b11)
			{
				appendString(&si, "nop ");
				appendEv(&si);
				*si.buffer++ = ',';
				appendGv(&si);
			}
			else
			{
				if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
					appendString(&si, "bndmov");
				else if (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX)
					appendString(&si, "bndcl");
				else if (instruction->simdPrefix == NMD_X86_REPEAT_NOT_ZERO_PREFIX)
					appendString(&si, "bndcu");
				else
					appendString(&si, "bndldx");

				appendString(&si, " bnd");
				*si.buffer++ = (char)(0x30 + instruction->modrm.reg);
				*si.buffer++ = ',';
				if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
					*si.buffer++ = 'q';
				appendEv(&si);
			}
		}
		else if (op == 0x1B)
		{
			if (instruction->modrm.mod == 0b11)
			{
				appendString(&si, "nop ");
				appendEv(&si);
				*si.buffer++ = ',';
				appendGv(&si);
			}
			else
			{
				if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
					appendString(&si, "bndmov");
				else if (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX)
					appendString(&si, "bndmk");
				else if (instruction->simdPrefix == NMD_X86_REPEAT_NOT_ZERO_PREFIX)
					appendString(&si, "bndcn");
				else
					appendString(&si, "bndstx");

				*si.buffer++ = ' ';
				appendEv(&si);
				*si.buffer++ = ',';
				appendString(&si, "bnd");
				*si.buffer++ = (char)(0x30 + instruction->modrm.reg);
			}
		}
		else if (op >= 0x1c && op <= 0x1f)
		{
			appendString(&si, "nop ");
			appendEv(&si);
			*si.buffer++ = ',';
			appendGv(&si);
		}
		else if (op >= 0x20 && op <= 0x23)
		{
			appendString(&si, "mov ");
			if (op < 0x22)
			{
				appendString(&si, (instruction->mode == NMD_X86_MODE_64 ? reg64 : reg32)[instruction->modrm.rm]);
				appendString(&si, op == 0x20 ? ",cr" : ",dr");
				*si.buffer++ = (char)(0x30 + instruction->modrm.reg);
			}
			else
			{
				appendString(&si, op == 0x22 ? "cr" : "dr");
				*si.buffer++ = (char)(0x30 + instruction->modrm.reg);
				*si.buffer++ = ',';
				appendString(&si, (instruction->mode == NMD_X86_MODE_64 ? reg64 : reg32)[instruction->modrm.rm]);
			}
		}
		else if (op >= 0x28 && op <= 0x2f)
		{
			const char* noPrefixMnemonics[] = { "movaps", "movaps", "cvtpi2ps", "movntps", "cvttps2pi", "cvtps2pi", "ucomiss", "comiss" };
			const char* prefix66Mnemonics[] = { "movapd", "movapd", "cvtpi2pd", "movntpd", "cvttpd2pi", "cvtpd2pi", "ucomisd", "comisd" };
			const char* prefixF3Mnemonics[] = { NMD_NULL, NMD_NULL, "cvtsi2ss", "movntss", "cvttss2si", "cvtss2si", NMD_NULL, NMD_NULL };
			const char* prefixF2Mnemonics[] = { NMD_NULL, NMD_NULL, "cvtsi2sd", "movntsd", "cvttsd2si", "cvtsd2si", NMD_NULL, NMD_NULL };
			if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
			{
				appendString(&si, prefix66Mnemonics[op % 8]);
				*si.buffer++ = ' ';

				switch (op % 8)
				{
				case 0:
					appendVx(&si);
					*si.buffer++ = ',';
					appendW(&si);
					break;
				case 1:
					appendW(&si);
					*si.buffer++ = ',';
					appendVx(&si);
					break;
				case 2:
					appendVdq(&si);
					*si.buffer++ = ',';
					appendQq(&si);
					break;
				case 4:
				case 5:
					appendPq(&si);
					*si.buffer++ = ',';
					appendW(&si);
					break;
				case 6:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + instruction->modrm.rm);
					else
						appendModRmUpper(&si, "qword");
					break;
				case 7:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + instruction->modrm.rm);
					else
						appendModRmUpper(&si, "qword");
				default:
					break;
				}
			}
			else if (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX)
			{
				appendString(&si, prefixF3Mnemonics[op % 8]);
				*si.buffer++ = ' ';

				switch (op % 8)
				{
				case 3:
					appendModRmUpper(&si, "dword");
					*si.buffer++ = ',';
					appendVdq(&si);
					break;
				case 4:
				case 5:
					appendGv(&si);
					*si.buffer++ = ',';
					if (instruction->modrm.mod == 0b11)
						appendUdq(&si);
					else
						appendModRmUpper(&si, "dword");
					break;
				case 2:
				case 6:
					appendVdq(&si);
					*si.buffer++ = ',';
					appendEv(&si);
					break;
				}
			}
			else if (instruction->simdPrefix == NMD_X86_REPEAT_NOT_ZERO_PREFIX)
			{
				appendString(&si, prefixF2Mnemonics[op % 8]);
				*si.buffer++ = ' ';

				switch (op % 8)
				{
				case 2:
					appendVdq(&si);
					*si.buffer++ = ',';
					appendEv(&si);
					break;
				case 3:
					appendModRmUpper(&si, "qword");
					*si.buffer++ = ',';
					appendVdq(&si);
					break;
				case 4:
				case 5:
					appendGv(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
					else
						appendModRmUpper(&si, "qword");
					break;
				}
			}
			else
			{
				appendString(&si, noPrefixMnemonics[op % 8]);
				*si.buffer++ = ' ';

				switch (op % 8)
				{
				case 0:
					appendVdq(&si);
					*si.buffer++ = ',';
					appendW(&si);
					break;
				case 1:
					appendW(&si);
					*si.buffer++ = ',';
					appendVdq(&si);
					break;
				case 4:
				case 5:
					appendPq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
					else
						appendModRmUpper(&si, "qword");
					break;
				case 2:
					appendVdq(&si);
					*si.buffer++ = ',';
					appendQq(&si);
					break;
				case 6:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
					else
						appendModRmUpper(&si, "dword");
					break;
				case 7:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
					else
						appendModRmUpper(&si, "dword");
					break;
				default:
					break;
				};

			}

			if (!(instruction->prefixes & (NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX)) && (op % 8) == 3)
			{
				appendModRmUpper(&si, "xmmword");
				*si.buffer++ = ',';
				appendVdq(&si);
			}
		}
		else if (NMD_R(op) == 4)
		{
			appendString(&si, "cmov");
			appendString(&si, conditionSuffixes[NMD_C(op)]);
			*si.buffer++ = ' ';
			appendGv(&si);
			*si.buffer++ = ',';
			appendEv(&si);
		}
		else if (NMD_R(op) == 5)
		{
			const char* noPrefixMnemonics[] = { "movmskps", "sqrtps", "rsqrtps", "rcpps", "andps", "andnps", "orps", "xorps", "addps", "mulps", "cvtps2pd",  "cvtdq2ps", "subps", "minps", "divps", "maxps" };
			const char* prefix66Mnemonics[] = { "movmskpd", "sqrtpd", NMD_NULL, NMD_NULL, "andpd", "andnpd", "orpd", "xorpd", "addpd", "mulpd", "cvtpd2ps",  "cvtps2dq", "subpd", "minpd", "divpd", "maxpd" };
			const char* prefixF3Mnemonics[] = { NMD_NULL, "sqrtss", "rsqrtss", "rcpss", NMD_NULL, NMD_NULL, NMD_NULL, NMD_NULL, "addss", "mulss", "cvtss2sd", "cvttps2dq", "subss", "minss", "divss", "maxss" };
			const char* prefixF2Mnemonics[] = { NMD_NULL, "sqrtsd", NMD_NULL, NMD_NULL, NMD_NULL, NMD_NULL, NMD_NULL, NMD_NULL, "addsd", "mulsd", "cvtsd2ss", NMD_NULL, "subsd", "minsd", "divsd", "maxsd" };
			if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
			{
				appendString(&si, prefix66Mnemonics[op % 0x10]);
				*si.buffer++ = ' ';
				if (op == 0x50)
					appendString(&si, reg32[instruction->modrm.reg]);
				else
					appendVdq(&si);
				*si.buffer++ = ',';
				appendW(&si);
			}
			else if (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX)
			{
				appendString(&si, prefixF3Mnemonics[op % 0x10]);
				*si.buffer++ = ' ';
				appendVdq(&si);
				*si.buffer++ = ',';
				if (si.instruction->modrm.mod == 0b11)
					appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
				else
					appendModRmUpper(&si, op == 0x5b ? "xmmword" : "dword");
			}
			else if (instruction->simdPrefix == NMD_X86_REPEAT_NOT_ZERO_PREFIX)
			{
				appendString(&si, prefixF2Mnemonics[op % 0x10]);
				*si.buffer++ = ' ';
				appendVdq(&si);
				*si.buffer++ = ',';
				if (si.instruction->modrm.mod == 0b11)
					appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
				else
					appendModRmUpper(&si, "qword");
			}
			else
			{
				appendString(&si, noPrefixMnemonics[op % 0x10]);
				*si.buffer++ = ' ';
				if (op == 0x50)
				{
					appendString(&si, reg32[instruction->modrm.reg]);
					*si.buffer++ = ',';
					appendUdq(&si);
				}
				else
				{
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.mod == 0b11)
						appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + si.instruction->modrm.rm);
					else
						appendModRmUpper(&si, op == 0x5a ? "qword" : "xmmword");
				}
			}
		}
		else if (NMD_R(op) == 6 || (op >= 0x74 && op <= 0x76))
		{
			const char* noPrefixMnemonics[] = { "punpcklbw", "punpcklwd", "punpckldq", "packsswb", "pcmpgtb", "pcmpgtw", "pcmpgtd", "packuswb", "punpckhbw", "punpckhwd", "punpckhdq", "packssdw",         NMD_NULL,         NMD_NULL, "movd",   "movq" };
			const char* prefix66Mnemonics[] = { "punpcklbw", "punpcklwd", "punpckldq", "packsswb", "pcmpgtb", "pcmpgtw", "pcmpgtd", "packuswb", "punpckhbw", "punpckhwd", "punpckhdq", "packssdw", "punpcklqdq", "punpckhqdq", "movd", "movdqa" };

			if (op == 0x6e)
			{
				appendString(&si, "movd ");
				if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
					*si.buffer++ = 'x';
				appendPq(&si);
				*si.buffer++ = ',';
				if (si.instruction->modrm.mod == 0b11)
					appendString(&si, reg32[si.instruction->modrm.rm]);
				else
					appendModRmUpper(&si, "dword");
			}
			else
			{
				if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
				{
					appendString(&si, op == 0x74 ? "pcmpeqb" : (op == 0x75 ? "pcmpeqw" : (op == 0x76 ? "pcmpeqd" : prefix66Mnemonics[op % 0x10])));
					*si.buffer++ = ' ';
					appendVdq(&si);
					*si.buffer++ = ',';
					appendW(&si);
				}
				else if (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX)
				{
					appendString(&si, "movdqu ");
					appendVdq(&si);
					*si.buffer++ = ',';
					appendW(&si);
				}
				else
				{
					appendString(&si, op == 0x74 ? "pcmpeqb" : (op == 0x75 ? "pcmpeqw" : (op == 0x76 ? "pcmpeqd" : noPrefixMnemonics[op % 0x10])));
					*si.buffer++ = ' ';
					appendPq(&si);
					*si.buffer++ = ',';
					appendQq(&si);
				}
			}
		}
		else if (op == 0x70)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? "pshufd" : (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX ? "pshufhw" : (instruction->simdPrefix == NMD_X86_REPEAT_NOT_ZERO_PREFIX ? "pshuflw" : "pshufw")));
			*si.buffer++ = ' ';
			if (!(instruction->prefixes & (NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX | NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX)))
			{
				appendPq(&si);
				*si.buffer++ = ',';
				appendQq(&si);
			}
			else
			{
				appendVdq(&si);
				*si.buffer++ = ',';
				appendW(&si);
			}

			*si.buffer++ = ',';
			appendNumber(&si, instruction->immediate);
		}
		else if (op >= 0x71 && op <= 0x73)
		{
			if (instruction->modrm.reg % 2 == 1)
				appendString(&si, instruction->modrm.reg == 0b111 ? "pslldq" : "psrldq");
			else
			{
				const char* mnemonics[] = { "psrl", "psra", "psll" };
				appendString(&si, mnemonics[(instruction->modrm.reg >> 1) - 1]);
				*si.buffer++ = op == 0x71 ? 'w' : (op == 0x72 ? 'd' : 'q');
			}

			*si.buffer++ = ' ';
			if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
				appendUdq(&si);
			else
				appendNq(&si);
			*si.buffer++ = ',';
			appendNumber(&si, instruction->immediate);
		}
		else if (op == 0x78)
		{
			appendString(&si, "vmread ");
			appendEy(&si);
			*si.buffer++ = ',';
			appendGy(&si);
		}
		else if (op == 0x79)
		{
			appendString(&si, "vmwrite ");
			appendGy(&si);
			*si.buffer++ = ',';
			appendEy(&si);
		}
		else if (op == 0x7c || op == 0x7d)
		{
			appendString(&si, op == 0x7c ? "haddp" : "hsubp");
			*si.buffer++ = instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? 'd' : 's';
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			appendW(&si);
		}
		else if (op == 0x7e)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_REPEAT_PREFIX ? "movq " : "movd ");
			if (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX)
			{
				appendVdq(&si);
				*si.buffer++ = ',';
				if (si.instruction->modrm.mod == 0b11)
					appendUdq(&si);
				else
					appendModRmUpper(&si, "qword");
			}
			else
			{
				if (si.instruction->modrm.mod == 0b11)
					appendString(&si, reg32[instruction->modrm.rm]);
				else
					appendModRmUpper(&si, "dword");
				*si.buffer++ = ',';
				if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
					appendVdq(&si);
				else
					appendPq(&si);
			}
		}
		else if (op == 0x7f)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_REPEAT_PREFIX ? "movdqu" : (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? "movdqa" : "movq"));
			*si.buffer++ = ' ';
			if (instruction->prefixes & (NMD_X86_REPEAT_PREFIX | NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX))
			{
				appendW(&si);
				*si.buffer++ = ',';
				appendVdq(&si);
			}
			else
			{
				if (si.instruction->modrm.mod == 0b11)
					appendNq(&si);
				else
					appendModRmUpper(&si, "qword");
				*si.buffer++ = ',';
				appendPq(&si);
			}
		}
		else if (NMD_R(op) == 8)
		{
			*si.buffer++ = 'j';
			appendString(&si, conditionSuffixes[NMD_C(op)]);
			*si.buffer++ = ' ';
			appendRelativeAddress16_32(&si);
		}
		else if (NMD_R(op) == 9)
		{
			appendString(&si, "set");
			appendString(&si, conditionSuffixes[NMD_C(op)]);
			*si.buffer++ = ' ';
			appendEb(&si);
		}
		else if ((NMD_R(op) == 0xA || NMD_R(op) == 0xB) && op % 8 == 3)
		{
			appendString(&si, op == 0xa3 ? "bt" : (op == 0xb3 ? "btr" : (op == 0xab ? "bts" : "btc")));
			*si.buffer++ = ' ';
			appendEv(&si);
			*si.buffer++ = ',';
			appendGv(&si);
		}
		else if (NMD_R(op) == 0xA && (op % 8 == 4 || op % 8 == 5))
		{
			appendString(&si, op > 0xA8 ? "shrd" : "shld");
			*si.buffer++ = ' ';
			appendEv(&si);
			*si.buffer++ = ',';
			appendGv(&si);
			*si.buffer++ = ',';
			if (op % 8 == 4)
				appendNumber(&si, instruction->immediate);
			else
				appendString(&si, "cl");
		}
		else if (op == 0xb4 || op == 0xb5)
		{
			appendString(&si, op == 0xb4 ? "lfs " : "lgs ");
			appendGv(&si);
			*si.buffer++ = ',';
			appendModRmUpper(&si, "fword");
		}
		else if (op == 0xbc || op == 0xbd)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_REPEAT_PREFIX ? (op == 0xbc ? "tzcnt" : "lzcnt") : (op == 0xbc ? "bsf" : "bsr"));
			*si.buffer++ = ' ';
			appendGv(&si);
			*si.buffer++ = ',';
			appendEv(&si);
		}
		else if (op == 0xa6)
		{
			const char* mnemonics[] = { "montmul", "xsha1", "xsha256" };
			appendString(&si, mnemonics[instruction->modrm.reg]);
		}
		else if (op == 0xa7)
		{
			const char* mnemonics[] = { "xstorerng", "xcryptecb", "xcryptcbc", "xcryptctr", "xcryptcfb", "xcryptofb" };
			appendString(&si, mnemonics[instruction->modrm.reg]);
		}
		else if (op == 0xae)
		{
			if (instruction->modrm.mod == 0b11)
			{
				if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
					appendString(&si, "pcommit");
				else if (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX)
				{
					appendString(&si, "incsspd ");
					appendString(&si, reg32[instruction->modrm.rm]);
				}
				else
				{
					const char* mnemonics[] = { "rdfsbase", "rdgsbase", "wrfsbase", "wrgsbase", NMD_NULL, "lfence", "mfence", "sfence" };
					appendString(&si, mnemonics[instruction->modrm.reg]);
				}
			}
			else
			{
				if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
				{
					appendString(&si, instruction->modrm.reg == 0b110 ? "clwb " : "clflushopt ");
					appendModRmUpper(&si, "byte");
				}
				else if (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX)
				{
					appendString(&si, instruction->modrm.reg == 0b100 ? "ptwrite " : "clrssbsy ");
					appendModRmUpper(&si, instruction->modrm.reg == 0b100 ? "dword" : "qword");
				}
				else
				{
					const char* mnemonics[] = { "fxsave", "fxrstor", "ldmxcsr", "stmxcsr", "xsave", "xrstor", "xsaveopt", "clflush" };
					appendString(&si, mnemonics[instruction->modrm.reg]);
					*si.buffer++ = ' ';
					appendModRmUpper(&si, "dword");
				}
			}
		}
		else if (op == 0xaf)
		{
			appendString(&si, "imul ");
			appendGv(&si);
			*si.buffer++ = ',';
			appendEv(&si);
		}
		else if (op == 0xb0 || op == 0xb1)
		{
			appendString(&si, "cmpxchg ");
			if (op == 0xb0)
			{
				appendEb(&si);
				*si.buffer++ = ',';
				appendGb(&si);
			}
			else
			{
				appendEv(&si);
				*si.buffer++ = ',';
				appendGv(&si);
			}
		}
		else if (op == 0xb2)
		{
			appendString(&si, "lss ");
			appendGv(&si);
			*si.buffer++ = ',';
			appendModRmUpper(&si, "fword");
		}
		else if (NMD_R(op) == 0xb && (op % 8) >= 6)
		{
			appendString(&si, op > 0xb8 ? "movsx " : "movzx ");
			appendGv(&si);
			*si.buffer++ = ',';
			if ((op % 8) == 6)
				appendEb(&si);
			else
				appendEw(&si);
		}
		else if (op == 0xb8)
		{
			appendString(&si, "popcnt ");
			appendGv(&si);
			*si.buffer++ = ',';
			appendEv(&si);
		}
		else if (op == 0xba)
		{
			const char* mnemonics[] = { "bt","bts","btr","btc" };
			appendString(&si, mnemonics[instruction->modrm.reg - 4]);
			*si.buffer++ = ' ';
			appendEv(&si);
			*si.buffer++ = ',';
			appendNumber(&si, instruction->immediate);
		}
		else if (op == 0xc0 || op == 0xc1)
		{
			appendString(&si, "xadd ");
			if (op == 0xc0)
			{
				appendEb(&si);
				*si.buffer++ = ',';
				appendGb(&si);
			}
			else
			{
				appendEv(&si);
				*si.buffer++ = ',';
				appendGv(&si);
			}
		}
		else if (op == 0xc2)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? "cmppd" : (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX ? "cmpss" : (instruction->simdPrefix == NMD_X86_REPEAT_NOT_ZERO_PREFIX ? "cmpsd" : "cmpps")));
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			if (si.instruction->modrm.mod == 0b11)
				appendUdq(&si);
			else
				appendModRmUpper(&si, instruction->simdPrefix == NMD_X86_REPEAT_PREFIX ? "dword" : (instruction->simdPrefix == NMD_X86_REPEAT_NOT_ZERO_PREFIX ? "qword" : "xmmword"));
			*si.buffer++ = ',';
			appendNumber(&si, instruction->immediate);
		}
		else if (op == 0xc3)
		{
			appendString(&si, "movnti ");
			appendModRmUpper(&si, "dword");
			*si.buffer++ = ',';
			appendString(&si, reg32[instruction->modrm.reg]);
		}
		else if (op == 0xc4)
		{
			appendString(&si, "pinsrw ");
			if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
				appendVdq(&si);
			else
				appendPq(&si);
			*si.buffer++ = ',';
			if (si.instruction->modrm.mod == 0b11)
				appendString(&si, reg32[si.instruction->modrm.rm]);
			else
				appendModRmUpper(&si, "word");
			*si.buffer++ = ',';
			appendNumber(&si, instruction->immediate);
		}
		else if (op == 0xc5)
		{
			appendString(&si, "pextrw ");
			appendString(&si, reg32[si.instruction->modrm.reg]);
			*si.buffer++ = ',';
			if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
				appendUdq(&si);
			else
				appendNq(&si);
			*si.buffer++ = ',';
			appendNumber(&si, instruction->immediate);
		}
		else if (op == 0xc6)
		{
			appendString(&si, "shufp");
			*si.buffer++ = instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? 'd' : 's';
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			appendW(&si);
			*si.buffer++ = ',';
			appendNumber(&si, instruction->immediate);
		}
		else if (op == 0xC7)
		{
			if (instruction->modrm.reg == 0b001)
			{
				appendString(&si, "cmpxchg8b ");
				appendModRmUpper(&si, "qword");
			}
			else if (instruction->modrm.reg <= 0b101)
			{
				const char* mnemonics[] = { "xrstors", "xsavec", "xsaves" };
				appendString(&si, mnemonics[instruction->modrm.reg - 3]);
				*si.buffer++ = ' ';
				appendEb(&si);
			}
			else if (instruction->modrm.reg == 0b110)
			{
				if (instruction->modrm.mod == 0b11)
				{
					appendString(&si, "rdrand ");
					appendRv(&si);
				}
				else
				{
					appendString(&si, instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? "vmclear" : (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX ? "vmxon" : "vmptrld"));
					*si.buffer++ = ' ';
					appendModRmUpper(&si, "qword");
				}
			}
			else // reg == 0b111
			{
				if (instruction->modrm.mod == 0b11)
				{
					appendString(&si, "rdseed ");
					appendRv(&si);
				}
				else
				{
					appendString(&si, "vmptrst ");
					appendModRmUpper(&si, "qword");
				}
			}
		}
		else if (op >= 0xc8 && op <= 0xcf)
		{
			appendString(&si, "bswap ");
			appendString(&si, (operandSize ? reg16 : reg32)[op % 8]);
		}
		else if (op == 0xd0)
		{
			appendString(&si, "addsubp");
			*si.buffer++ = instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? 'd' : 's';
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			appendW(&si);
		}
		else if (op == 0xd6)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? "movq" : (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX ? "movq2dq" : "movdq2q"));
			*si.buffer++ = ' ';
			if (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX)
			{
				appendVdq(&si);
				*si.buffer++ = ',';
				appendNq(&si);
			}
			else if (instruction->simdPrefix == NMD_X86_REPEAT_NOT_ZERO_PREFIX)
			{
				appendPq(&si);
				*si.buffer++ = ',';
				appendUdq(&si);
			}
			else
			{
				if (si.instruction->modrm.mod == 0b11)
					appendUdq(&si);
				else
					appendModRmUpper(&si, "qword");
				*si.buffer++ = ',';
				appendVdq(&si);
			}
		}
		else if (op == 0xd7)
		{
			appendString(&si, "pmovmskb ");
			appendString(&si, reg32[instruction->modrm.reg]);
			*si.buffer++ = ',';
			if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
				appendUdq(&si);
			else
				appendNq(&si);
		}
		else if (op == 0xe6)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? "cvttpd2dq" : (instruction->simdPrefix == NMD_X86_REPEAT_PREFIX ? "cvtdq2pd" : "cvtpd2dq"));
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			if (si.instruction->modrm.mod == 0b11)
				appendUdq(&si);
			else
				appendModRmUpper(&si, instruction->simdPrefix == NMD_X86_REPEAT_PREFIX ? "qword" : "xmmword");
		}
		else if (op == 0xe7)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? "movntdq" : "movntq");
			*si.buffer++ = ' ';
			appendModRmUpper(&si, instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? "xmmword" : "qword");
			*si.buffer++ = ',';
			if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
				appendVdq(&si);
			else
				appendPq(&si);
		}
		else if (op == 0xf0)
		{
			appendString(&si, "lddqu ");
			appendVdq(&si);
			*si.buffer++ = ',';
			appendModRmUpper(&si, "xmmword");
		}
		else if (op == 0xf7)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX ? "maskmovdqu " : "maskmovq ");
			if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
			{
				appendVdq(&si);
				*si.buffer++ = ',';
				appendUdq(&si);
			}
			else
			{
				appendPq(&si);
				*si.buffer++ = ',';
				appendNq(&si);
			}
		}
		else if (op >= 0xd1 && op <= 0xfe)
		{
			const char* mnemonics[] = { "srlw", "srld", "srlq", "addq", "mullw", NMD_NULL, NMD_NULL, "subusb", "subusw", "minub", "and", "addusb", "addusw", "maxub", "andn", "avgb", "sraw", "srad", "avgw", "mulhuw", "mulhw", NMD_NULL, NMD_NULL, "subsb", "subsw", "minsw", "or", "addsb", "addsw", "maxsw", "xor", NMD_NULL, "sllw", "slld", "sllq", "muludq", "maddwd", "sadbw", NMD_NULL, "subb", "subw", "subd", "subq", "addb", "addw", "addd" };
			*si.buffer++ = 'p';
			appendString(&si, mnemonics[op - 0xd1]);
			*si.buffer++ = ' ';
			if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
			{
				appendVdq(&si);
				*si.buffer++ = ',';
				appendW(&si);
			}
			else
			{
				appendPq(&si);
				*si.buffer++ = ',';
				appendQq(&si);
			}
		}
		else
		{
			const char* str = NMD_NULL;
			switch (op)
			{
			case 0x05: str = "syscall"; break;
			case 0x06: str = "clts"; break;
			case 0x07: str = "sysret"; break;
			case 0x08: str = "invd"; break;
			case 0x09: str = "wbinvd"; break;
			case 0x0b: str = "ud2"; break;
			case 0x0e: str = "femms"; break;
			case 0x30: str = "wrmsr"; break;
			case 0x31: str = "rdtsc"; break;
			case 0x32: str = "rdmsr"; break;
			case 0x33: str = "rdpmc"; break;
			case 0x34: str = "sysenter"; break;
			case 0x35: str = "sysexit"; break;
			case 0x37: str = "getsec"; break;
			case 0x77: str = "emms"; break;
			case 0xa0: str = "push fs"; break;
			case 0xa1: str = "pop fs"; break;
			case 0xa2: str = "cpuid"; break;
			case 0xa8: str = "push gs"; break;
			case 0xa9: str = "pop gs"; break;
			case 0xaa: str = "rsm"; break;
			case 0xb9: str = "ud2"; break;
			case 0xff: str = "ud0"; break;
			default: return;
			}
			appendString(&si, str);
		}
	}
	else if (instruction->opcodeMap == NMD_X86_OPCODE_MAP_0F_38)
	{
		if ((NMD_R(op) == 2 || NMD_R(op) == 3) && NMD_C(op) <= 5)
		{
			const char* mnemonics[] = { "pmovsxbw", "pmovsxbd", "pmovsxbq", "pmovsxwd", "pmovsxwq", "pmovsxdq" };
			appendString(&si, mnemonics[NMD_C(op)]);
			if (NMD_R(op) == 3)
				*(si.buffer - 4) = 'z';
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			if (instruction->modrm.mod == 0b11)
				appendUdq(&si);
			else
				appendModRmUpper(&si, NMD_C(op) == 5 ? "qword" : (NMD_C(op) % 3 == 0 ? "qword" : (NMD_C(op) % 3 == 1 ? "dword" : "word")));
		}
		else if (op >= 0x80 && op <= 0x83)
		{
			appendString(&si, op == 0x80 ? "invept" : (op == 0x81 ? "invvpid" : "invpcid"));
			*si.buffer++ = ' ';
			appendGy(&si);
			*si.buffer++ = ',';
			appendModRmUpper(&si, "xmmword");
		}
		else if (op >= 0xc8 && op <= 0xcd)
		{
			const char* mnemonics[] = { "sha1nexte", "sha1msg1", "sha1msg2", "sha256rnds2", "sha256msg1", "sha256msg2" };
			appendString(&si, mnemonics[op - 0xc8]);
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			appendW(&si);
		}
		else if (op == 0xf0 || op == 0xf1)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_REPEAT_NOT_ZERO_PREFIX ? "crc32" : "movbe");
			*si.buffer++ = ' ';
			if (op == 0xf0)
			{
				if (instruction->simdPrefix == NMD_X86_REPEAT_NOT_ZERO_PREFIX)
				{
					appendString(&si, reg32[instruction->modrm.reg]);
					*si.buffer++ = ',';
					appendEb(&si);
				}
				else
				{
					appendGv(&si);
					*si.buffer++ = ',';
					appendEv(&si);
				}
			}
			else
			{
				if (instruction->simdPrefix == NMD_X86_REPEAT_NOT_ZERO_PREFIX)
				{
					appendString(&si, reg32[instruction->modrm.reg]);
					*si.buffer++ = ',';
					if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
						appendEw(&si);
					else
						appendEy(&si);
				}
				else
				{
					appendEv(&si);
					*si.buffer++ = ',';
					appendGv(&si);
				}
			}
		}
		else if (op == 0xf6)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_REPEAT_PREFIX ? "adox " : "adcx ");
			appendGy(&si);
			*si.buffer++ = ',';
			appendEy(&si);
		}
		else
		{
			if (op == 0x40)
				appendString(&si, "pmulld");
			else if (op == 0x41)
				appendString(&si, "phminposuw");
			else if (op >= 0xdb && op <= 0xdf)
			{
				const char* mnemonics[] = { "aesimc", "aesenc", "aesenclast", "aesdec", "aesdeclast" };
				appendString(&si, mnemonics[op - 0xdb]);
			}
			else if (op == 0x37)
				appendString(&si, "pcmpgtq");
			else if (NMD_R(op) == 2)
			{
				const char* mnemonics[] = { "pmuldq", "pcmpeqq", "movntdqa", "packusdw" };
				appendString(&si, mnemonics[NMD_C(op) - 8]);
			}
			else if (NMD_R(op) == 3)
			{
				const char* mnemonics[] = { "pminsb", "pminsd", "pminuw", "pminud", "pmaxsb", "pmaxsd", "pmaxuw", "pmaxud" };
				appendString(&si, mnemonics[NMD_C(op) - 8]);
			}
			else if (op < 0x10)
			{
				const char* mnemonics[] = { "pshufb", "phaddw", "phaddd", "phaddsw", "pmaddubsw", "phsubw", "phsubd", "phsubsw", "psignb", "psignw", "psignd", "pmulhrsw", "permilpsv", "permilpdv", "testpsv", "testpdv" };
				appendString(&si, mnemonics[op]);
			}
			else if (op < 0x18)
				appendString(&si, op == 0x10 ? "pblendvb" : (op == 0x14 ? "blendvps" : (op == 0x15 ? "blendvpd" : "ptest")));
			else
			{
				appendString(&si, "pabs");
				*si.buffer++ = op == 0x1c ? 'b' : (op == 0x1d ? 'w' : 'd');
			}
			*si.buffer++ = ' ';
			if (instruction->simdPrefix == NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX)
			{
				appendVdq(&si);
				*si.buffer++ = ',';
				appendW(&si);
			}
			else
			{
				appendPq(&si);
				*si.buffer++ = ',';
				appendQq(&si);
			}
		}

	}
	else if (instruction->opcodeMap == NMD_X86_OPCODE_MAP_0F_3A)
	{
		if (NMD_R(op) == 1)
		{
			const char* mnemonics[] = { "pextrb", "pextrw", "pextrd", "extractps" };
			appendString(&si, mnemonics[op - 0x14]);
			*si.buffer++ = ' ';
			if (instruction->modrm.mod == 0b11)
				appendString(&si, (si.instruction->flags.operandSize64 ? reg64 : reg32)[instruction->modrm.rm]);
			else
			{
				if (op == 0x14)
					appendModRmUpper(&si, "byte");
				else if (op == 0x15)
					appendModRmUpper(&si, "word");
				else if (op == 0x16)
					appendEy(&si);
				else
					appendModRmUpper(&si, "dword");
			}
			*si.buffer++ = ',';
			appendVdq(&si);
		}
		else if (NMD_R(op) == 2)
		{
			appendString(&si, op == 0x20 ? "pinsrb" : (op == 0x21 ? "insertps" : "pinsrd"));
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			if (op == 0x20)
			{
				if (instruction->modrm.mod == 0b11)
					appendString(&si, reg32[instruction->modrm.rm]);
				else
					appendModRmUpper(&si, "byte");
			}
			else if (op == 0x21)
			{
				if (instruction->modrm.mod == 0b11)
					appendUdq(&si);
				else
					appendModRmUpper(&si, "dword");
			}
			else
				appendEy(&si);
		}
		else
		{
			if (op < 0x10)
			{
				const char* mnemonics[] = { "roundps", "roundpd", "roundss", "roundsd", "blendps", "blendpd", "pblendw", "palignr" };
				appendString(&si, mnemonics[op - 8]);
			}
			else if (NMD_R(op) == 4)
			{
				const char* mnemonics[] = { "dpps", "dppd", "mpsadbw", NMD_NULL, "pclmulqdq" };
				appendString(&si, mnemonics[NMD_C(op)]);
			}
			else if (NMD_R(op) == 6)
			{
				const char* mnemonics[] = { "pcmpestrm", "pcmpestri", "pcmpistrm", "pcmpistri" };
				appendString(&si, mnemonics[NMD_C(op)]);
			}
			else if (op > 0x80)
				appendString(&si, op == 0xcc ? "sha1rnds4" : "aeskeygenassist");
			*si.buffer++ = ' ';
			if (op == 0xf && !(instruction->prefixes & (NMD_X86_OPERAND_SIZE_OVERRIDE_PREFIX | NMD_X86_REPEAT_PREFIX | NMD_X86_REPEAT_NOT_ZERO_PREFIX)))
			{
				appendPq(&si);
				*si.buffer++ = ',';
				appendQq(&si);
			}
			else
			{
				appendVdq(&si);
				*si.buffer++ = ',';
				if (instruction->modrm.mod == 0b11)
					appendString(&si, "xmm"), * si.buffer++ = (char)(0x30 + instruction->modrm.rm);
				else
					appendModRmUpper(&si, op == 0xa ? "dword" : (op == 0xb ? "qword" : "xmmword"));
			}
		}
		*si.buffer++ = ',';
		appendNumber(&si, instruction->immediate);
	}

	size_t stringLength = si.buffer - buffer;
	if (formatFlags & NMD_X86_FORMAT_FLAGS_UPPERCASE)
	{
		for (size_t i = 0; i < stringLength; i++)
		{
			const char c = buffer[i];
			buffer[i] = (c > 0x60 && c <= 0x7A) ? c - 0x20 : c;
		}
	}

	if (formatFlags & NMD_X86_FORMAT_FLAGS_COMMA_SPACES)
	{
		for (size_t i = 0; i < stringLength; i++)
		{
			if (buffer[i] == ',')
			{
				for (size_t j = stringLength; j > i; j--)
					buffer[j] = buffer[j - 1];

				buffer[i + 1] = ' ';
				si.buffer++, stringLength++;
			}
		}
	}

	if (formatFlags & NMD_X86_FORMAT_FLAGS_OPERATOR_SPACES)
	{
		for (size_t i = 0; i < stringLength; i++)
		{
			if (buffer[i] == '+' || (buffer[i] == '-' && buffer[i - 1] != ' ' && buffer[i - 1] != '('))
			{
				for (size_t j = stringLength + 1; j > i; j--)
					buffer[j] = buffer[j - 2];

				buffer[i + 1] = buffer[i];
				buffer[i] = ' ';
				buffer[i + 2] = ' ';
				si.buffer += 2, stringLength += 2;
				i++;
			}
		}
	}

	*si.buffer = '\0';
}

#endif // NMD_ASSEMBLY_IMPLEMENTATION

#endif // NMD_ASSEMBLY_H