//	
//		VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
//		it under the terms of the GNU General Public License as published by
//		the Free Software Foundation, either version 3 of the License, or
//		(at your option) any later version.
//	
//		VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
//		but WITHOUT ANY WARRANTY; without even the implied warranty of
//		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//		GNU General Public License for more details.
//	
//		You should have received a copy of the GNU General Public License
//		along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
//	
//		OpCodeTables for 6x09 microprocessors - Part of the Debugger package for VCC
//		Author: Mike Rojas
//
//		TODO: Let's add a capture for 6309 illegal opcodes and correct cycle behavior for 6809 illegal opcodes.
//
#pragma once
#pragma warning(push)
#pragma warning(disable : 26812)
#include <string>
#include <map>
#include <array>
#include "MachineDefs.h"

namespace VCC { namespace Debugger
{

	class OpCodeTables
	{
	public:
		OpCodeTables() {};

		// 6x09 Address Modes
		enum AddressingMode
		{
			Illegal,
			Inherent,
			Direct,
			Indexed,
			Relative,
			Extended,
			Immediate,
			LongRelative,
			OpPage2,
			OpPage3,
		};

		// 6x09 Heuristics
		// We classify op codes into special modifiers that are used to adjust the number of bytes and clock cycles
		enum Heuristics
		{
			None,					// Used for Illegal Op Codes
			Fixed,					// Base Bytes and Cycles are the final values
			WaitForSYNCAdjust,		// Adjust for Interrupt Delay SYNC Style
			IndexedModeAdjust,		// Adjust for various indexing modes, offsets, autoincrement/decrement
			StackAdjust,			// Adjust for number of registers pushed or pulled
			InterruptAdjust,		// Adjust for interrupt state via CC.E
			LongBranchAdjust,		// Adjust for long branch condition
			TFMAdjust,				// Adjust for number of bytes transferred
			DIVAdjust,				// Adjust for overflow condition
		};

		// OpCode page entry
		struct OpCodeInfo
		{
			unsigned char opcode;		// 8-bit opcode value
			std::string name;			// Opcode name
			AddressingMode mode;		// Addressing mode
			int num6809cycles;			// Base number of cycles (6809 only and 6309 in emulation)
			int num6309cycles;			// Base number of cycles (6309 native only)
			int numbytes;				// Base number of bytes
			int oplen;					// Number of bytes for opcode
			bool only6309;				// Valid only if running a 6309, in other words, invalid on a 6809
			Heuristics modifer;			// Modifer used to adjust base number of bytes and base clock cycles
		};

		// Page 1 operations xx codes
		std::array<OpCodeTables::OpCodeInfo, 256> Page1OpCodes =
		{
			0x00,	"NEG",		Direct,			6,		5,		2,		1,	false,	Fixed,
			0x01,	"OIM",		Direct,			6,		6,		3,		1,	true,	Fixed,						// Illegal on 6809
			0x02,	"AIM",		Direct,			6,		6,		3,		1,	true,	Fixed,						// Illegal on 6809
			0x03,	"COM",		Direct,			6,		5,		2,		1,	false,	Fixed,
			0x04,	"LSR",		Direct,			6,		5,		2,		1,	false,	Fixed,
			0x05,	"EIM",		Direct,			6,		6,		3,		1,	true,	Fixed,						// Illegal on 6809
			0x06,	"ROR",		Direct,			6,		5,		2,		1,	false,	Fixed,
			0x07,	"ASR",		Direct,			6,		5,		2,		1,	false,	Fixed,
			0x08,	"ASL",		Direct,			6,		5,		2,		1,	false,	Fixed,
			0x09,	"ROL",		Direct,			6,		5,		2,		1,	false,	Fixed,
			0x0A,	"DEC",		Direct,			6,		5,		2,		1,	false,	Fixed,
			0x0B,	"TIM",		Direct,			6,		6,		3,		1,	true,	Fixed,						// Illegal on 6809
			0x0C,	"INC",		Direct,			6,		5,		2,		1,	false,	Fixed,
			0x0D,	"TST",		Direct,			6,		4,		2,		1,	false,	Fixed,
			0x0E,	"JMP",		Direct,			3,		2,		2,		1,	false,	Fixed,
			0x0F,	"CLR",		Direct,			6,		5,		2,		1,	false,	Fixed,
																			
			0x10,	"page2",	OpPage2,		0,		0,		2,		1,	false,	Fixed,
			0x11,	"page3",	OpPage3,		0,		0,		2,		1,	false,	Fixed,
			0x12,	"NOP",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x13,	"SYNC",		Inherent,		4,		3,		1,		1,	false,	WaitForSYNCAdjust,
			0x14,	"SEXW",		Inherent,		4,		4,		1,		1,	true,	Fixed,						// Illegal on 6809
			0x15,	"HALT",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x16,	"LBRA",		LongRelative,	5,		4,		3,		1,	false,	LongBranchAdjust,
			0x17,	"LBSR",		LongRelative,	9,		7,		3,		1,	false,	LongBranchAdjust,
			0x18,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x19,	"DAA",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x1A,	"ORCC",		Immediate,		3,		3,		2,		1,	false,	Fixed,
			0x1B,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x1C,	"ANDCC",	Immediate,		3,		3,		2,		1,	false,	Fixed,
			0x1D,	"SEX",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x1E,	"EXG",		Immediate,		8,		5,		2,		1,	false,	Fixed,
			0x1F,	"TFR",		Immediate,		6,		4,		2,		1,	false,	Fixed,
																				
			0x20,	"BRA",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x21,	"BRN",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x22,	"BHI",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x23,	"BLS",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x24,	"BCC",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x25,	"BCS",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x26,	"BNE",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x27,	"BEQ",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x28,	"BVC",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x29,	"BVS",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x2A,	"BPL",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x2B,	"BMI",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x2C,	"BGE",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x2D,	"BLT",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x2E,	"BGT",		Relative,		3,		3,		2,		1,	false,	Fixed,
			0x2F,	"BLE",		Relative,		3,		3,		2,		1,	false,	Fixed,
																				
			0x30,	"LEAX",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0x31,	"LEAY",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0x32,	"LEAS",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0x33,	"LEAU",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0x34,	"PSHS",		Inherent,		5,		4,		2,		1,	false,	StackAdjust,
			0x35,	"PULS",		Inherent,		5,		4,		2,		1,	false,	StackAdjust,
			0x36,	"PSHU",		Inherent,		5,		4,		2,		1,	false,	StackAdjust,
			0x37,	"PULU",		Inherent,		5,		4,		2,		1,	false,	StackAdjust,
			0x38,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x39,	"RTS",		Inherent,		5,		4,		1,		1,	false,	Fixed,
			0x3A,	"ABX",		Inherent,		3,		1,		1,		1,	false,	Fixed,
			0x3B,	"RTI",		Inherent,		6,		6,		1,		1,	false,	InterruptAdjust,
			0x3C,	"CWAI",		Immediate,		22,		20,		2,		1,	false,	Fixed,
			0x3D,	"MUL",		Inherent,		11,		10,		1,		1,	false,	Fixed,
			0x3E,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x3F,	"SWI",		Inherent,		19,		21,		1,		1,	false,	Fixed,
																				
			0x40,	"NEGA",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x41,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x42,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x43,	"COMA",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x44,	"LSRA",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x45,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x46,	"RORA",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x47,	"ASRA",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x48,	"LSLA",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x49,	"ROLA",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x4A,	"DECA",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x4B,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x4C,	"INCA",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x4D,	"TSTA",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x4E,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x4F,	"CLRA",		Inherent,		2,		1,		1,		1,	false,	Fixed,
																				
			0x50,	"NEGB",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x51,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x52,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x53,	"COMB",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x54,	"LSRB",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x55,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x56,	"RORB",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x57,	"ASRB",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x58,	"LSLB",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x59,	"ROLB",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x5A,	"DECB",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x5B,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x5C,	"INCB",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x5D,	"TSTB",		Inherent,		2,		1,		1,		1,	false,	Fixed,
			0x5E,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x5F,	"CLRB",		Inherent,		2,		1,		1,		1,	false,	Fixed,
																				
			0x60,	"NEG",		Indexed,		6,		6,		2,		1,	false,	IndexedModeAdjust,
			0x61,	"OIM",		Indexed,		7,		7,		3,		1,	true,	IndexedModeAdjust,			// Illegal on 6809
			0x62,	"AIM",		Indexed,		7,		7,		3,		1,	true,	IndexedModeAdjust,			// Illegal on 6809
			0x63,	"COM",		Indexed,		6,		6,		2,		1,	false,	IndexedModeAdjust,
			0x64,	"LSR",		Indexed,		6,		6,		2,		1,	false,	IndexedModeAdjust,
			0x65,	"EIM",		Indexed,		7,		7,		3,		1,	true,	IndexedModeAdjust,			// Illegal on 6809
			0x66,	"ROR",		Indexed,		6,		6,		2,		1,	false,	IndexedModeAdjust,
			0x67,	"ASR",		Indexed,		6,		6,		2,		1,	false,	IndexedModeAdjust,
			0x68,	"LSL",		Indexed,		6,		6,		2,		1,	false,	IndexedModeAdjust,
			0x69,	"ROL",		Indexed,		6,		6,		2,		1,	false,	IndexedModeAdjust,
			0x6A,	"DEC",		Indexed,		6,		6,		2,		1,	false,	IndexedModeAdjust,
			0x6B,	"TIM",		Indexed,		7,		7,		3,		1,	true,	IndexedModeAdjust,			// Illegal on 6809
			0x6C,	"INC",		Indexed,		6,		6,		2,		1,	false,	IndexedModeAdjust,
			0x6D,	"TST",		Indexed,		6,		5,		2,		1,	false,	IndexedModeAdjust,
			0x6E,	"JMP",		Indexed,		3,		3,		2,		1,	false,	IndexedModeAdjust,
			0x6F,	"CLR",		Indexed,		6,		6,		2,		1,	false,	IndexedModeAdjust,
																				
			0x70,	"NEG",		Extended,		7,		6,		3,		1,	false,	Fixed,
			0x71,	"OIM",		Extended,		7,		7,		4,		1,	true,	Fixed,						// Illegal on 6809
			0x72,	"AIM",		Extended,		7,		7,		4,		1,	true,	Fixed,						// Illegal on 6809
			0x73,	"COM",		Extended,		7,		6,		3,		1,	false,	Fixed,
			0x74,	"LSR",		Extended,		7,		6,		3,		1,	false,	Fixed,
			0x75,	"EIM",		Extended,		7,		7,		4,		1,	true,	Fixed,						// Illegal on 6809
			0x76,	"ROR",		Extended,		7,		6,		3,		1,	false,	Fixed,
			0x77,	"ASR",		Extended,		7,		6,		3,		1,	false,	Fixed,
			0x78,	"LSL",		Extended,		7,		6,		3,		1,	false,	Fixed,
			0x79,	"ROL",		Extended,		7,		6,		3,		1,	false,	Fixed,
			0x7A,	"DEC",		Extended,		7,		6,		3,		1,	false,	Fixed,
			0x7B,	"TIM",		Extended,		7,		7,		4,		1,	true,	Fixed,						// Illegal on 6809
			0x7C,	"INC",		Extended,		7,		6,		3,		1,	false,	Fixed,
			0x7D,	"TST",		Extended,		7,		5,		3,		1,	false,	Fixed,
			0x7E,	"JMP",		Extended,		4,		3,		3,		1,	false,	Fixed,
			0x7F,	"CLR",		Extended,		7,		6,		3,		1,	false,	Fixed,
																				
			0x80,	"SUBA",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0x81,	"CMPA",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0x82,	"SBCA",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0x83,	"SUBD",		Immediate,		4,		3,		3,		1,	false,	Fixed,
			0x84,	"ANDA",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0x85,	"BITA",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0x86,	"LDA",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0x87,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0x88,	"EORA",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0x89,	"ADCA",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0x8A,	"ORA",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0x8B,	"ADDA",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0x8C,	"CMPX",		Immediate,		4,		3,		3,		1,	false,	Fixed,
			0x8D,	"BSR",		Relative,		7,		6,		2,		1,	false,	Fixed,
			0x8E,	"LDX",		Immediate,		3,		3,		3,		1,	false,	Fixed,
			0x8F,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
																				
			0x90,	"SUBA",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0x91,	"CMPA",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0x92,	"SBCA",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0x93,	"SUBD",		Direct,			6,		4,		2,		1,	false,	Fixed,
			0x94,	"ANDA",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0x95,	"BITA",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0x96,	"LDA",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0x97,	"STA",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0x98,	"EORA",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0x99,	"ADCA",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0x9A,	"ORA",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0x9B,	"ADDA",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0x9C,	"CMPX",		Direct,			6,		4,		2,		1,	false,	Fixed,
			0x9D,	"JSR",		Direct,			7,		6,		2,		1,	false,	Fixed,
			0x9E,	"LDX",		Direct,			5,		4,		2,		1,	false,	Fixed,
			0x9F,	"STX",		Direct,			5,		4,		2,		1,	false,	Fixed,
																				
			0xA0,	"SUBA",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xA1,	"CMPA",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xA2,	"SBCA",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xA3,	"SUBD",		Indexed,		6,		5,		2,		1,	false,	IndexedModeAdjust,
			0xA4,	"ANDA",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xA5,	"BITA",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xA6,	"LDA",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xA7,	"STA",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xA8,	"EORA",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xA9,	"ADCA",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xAA,	"ORA",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xAB,	"ADDA",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xAC,	"CMPX",		Indexed,		6,		5,		2,		1,	false,	IndexedModeAdjust,
			0xAD,	"JSR",		Indexed,		7,		6,		2,		1,	false,	IndexedModeAdjust,
			0xAE,	"LDX",		Indexed,		5,		2,		2,		1,	false,	IndexedModeAdjust,
			0xAF,	"STX",		Indexed,		5,		2,		2,		1,	false,	IndexedModeAdjust,
																				
			0xB0,	"SUBA",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xB1,	"CMPA",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xB2,	"SBCA",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xB3,	"SUBD",		Extended,		7,		5,		3,		1,	false,	Fixed,
			0xB4,	"ANDA",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xB5,	"BITA",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xB6,	"LDA",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xB7,	"STA",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xB8,	"EORA",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xB9,	"ADCA",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xBA,	"ORA",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xBB,	"ADDA",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xBC,	"CMPX",		Extended,		7,		5,		3,		1,	false,	Fixed,
			0xBD,	"JSR",		Extended,		8,		7,		3,		1,	false,	Fixed,
			0xBE,	"LDX",		Extended,		6,		5,		3,		1,	false,	Fixed,
			0xBF,	"STX",		Extended,		6,		5,		3,		1,	false,	Fixed,
																				
			0xC0,	"SUBB",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0xC1,	"CMPB",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0xC2,	"SBCB",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0xC3,	"ADDD",		Immediate,		4,		3,		3,		1,	false,	Fixed,
			0xC4,	"ANDB",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0xC5,	"BITB",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0xC6,	"LDB",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0xC7,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
			0xC8,	"EORB",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0xC9,	"ADCB",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0xCA,	"ORB",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0xCB,	"ADDB",		Immediate,		2,		2,		2,		1,	false,	Fixed,
			0xCC,	"LDD",		Immediate,		3,		3,		3,		1,	false,	Fixed,
			0xCD,	"LDQ",		Immediate,		5,		5,		5,		1,	false,	Fixed,
			0xCE,	"LDU",		Immediate,		3,		3,		3,		1,	false,	Fixed,
			0xCF,	"-",		Illegal,		0,		0,		0,		1,	false,	None,
																				
			0xD0,	"SUBB",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0xD1,	"CMPB",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0xD2,	"SBCB",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0xD3,	"ADDD",		Direct,			6,		4,		2,		1,	false,	Fixed,
			0xD4,	"ANDB",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0xD5,	"BITB",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0xD6,	"LDB",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0xD7,	"STB",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0xD8,	"EORB",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0xD9,	"ADCB",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0xDA,	"ORB",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0xDB,	"ADDB",		Direct,			4,		3,		2,		1,	false,	Fixed,
			0xDC,	"LDD",		Direct,			5,		4,		2,		1,	false,	Fixed,
			0xDD,	"STD",		Direct,			5,		4,		2,		1,	false,	Fixed,
			0xDE,	"LDU",		Direct,			5,		4,		2,		1,	false,	Fixed,
			0xDF,	"STU",		Direct,			5,		4,		2,		1,	false,	Fixed,
																				
			0xE0,	"SUBB",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xE1,	"CMPB",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xE2,	"SBCB",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xE3,	"ADDD",		Indexed,		6,		5,		2,		1,	false,	IndexedModeAdjust,
			0xE4,	"ANDB",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xE5,	"BITB",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xE6,	"LDB",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xE7,	"STB",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xE8,	"EORB",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xE9,	"ADCB",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xEA,	"ORB",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xEB,	"ADDB",		Indexed,		4,		4,		2,		1,	false,	IndexedModeAdjust,
			0xEC,	"LDD",		Indexed,		5,		5,		2,		1,	false,	IndexedModeAdjust,
			0xED,	"STD",		Indexed,		5,		5,		2,		1,	false,	IndexedModeAdjust,
			0xEE,	"LDU",		Indexed,		5,		5,		2,		1,	false,	IndexedModeAdjust,
			0xEF,	"STU",		Indexed,		5,		5,		2,		1,	false,	IndexedModeAdjust,
																			
			0xF0,	"SUBB",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xF1,	"CMPB",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xF2,	"SBCB",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xF3,	"ADDD",		Extended,		7,		5,		3,		1,	false,	Fixed,
			0xF4,	"ANDB",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xF5,	"BITB",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xF6,	"LDB",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xF7,	"STB",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xF8,	"EORB",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xF9,	"ADCB",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xFA,	"ORB",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xFB,	"ADDB",		Extended,		5,		4,		3,		1,	false,	Fixed,
			0xFC,	"LDD",		Extended,		6,		5,		3,		1,	false,	Fixed,
			0xFD,	"STD",		Extended,		6,		5,		3,		1,	false,	Fixed,
			0xFE,	"LDU",		Extended,		6,		5,		3,		1,	false,	Fixed,
			0xFF,	"STU",		Extended,		6,		5,		3,		1,	false,	Fixed,
		};

		// Page 2 operations 10xx codes
		std::array<OpCodeTables::OpCodeInfo, 256> Page2OpCodes =
		{
			0x00,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x01,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x02,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x03,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x04,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x05,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x06,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x07,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x08,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x09,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x0A,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x0B,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x0C,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x0D,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x0E,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x0F,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
																				
			0x10,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x11,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x12,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x13,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x14,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x15,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x16,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x17,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x18,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x19,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x1A,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x1B,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x1C,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x1D,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x1E,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x1F,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
																				
			0x20,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x21,	"LBRN",		LongRelative,	5,		5,		4,		2,	false,	Fixed,
			0x22,	"LBHI",		LongRelative,	5,		6,		4,		2,	false,	LongBranchAdjust,
			0x23,	"LBLS",		LongRelative,	5,		6,		4,		2,	false,	LongBranchAdjust,
			0x24,	"LBCC",		LongRelative,	5,		6,		4,		2,	false,	LongBranchAdjust,
			0x25,	"LBCS",		LongRelative,	5,		6,		4,		2,	false,	LongBranchAdjust,
			0x26,	"LBNE",		LongRelative,	5,		6,		4,		2,	false,	LongBranchAdjust,
			0x27,	"LBEQ",		LongRelative,	5,		6,		4,		2,	false,	LongBranchAdjust,
			0x28,	"LBVC",		LongRelative,	5,		6,		4,		2,	false,	LongBranchAdjust,
			0x29,	"LBVS",		LongRelative,	5,		6,		4,		2,	false,	LongBranchAdjust,
			0x2A,	"LBPL",		LongRelative,	5,		6,		4,		2,	false,	LongBranchAdjust,
			0x2B,	"LBMI",		LongRelative,	5,		6,		4,		2,	false,	LongBranchAdjust,
			0x2C,	"LBGE",		LongRelative,	5,		6,		4,		2,	false,	LongBranchAdjust,
			0x2D,	"LBLT",		LongRelative,	5,		6,		4,		2,	false,	LongBranchAdjust,
			0x2E,	"LBGT",		LongRelative,	5,		6,		4,		2,	false,	LongBranchAdjust,
			0x2F,	"LBLE",		LongRelative,	5,		6,		4,		2,	false,	LongBranchAdjust,
																				
			0x30,	"ADDR",		Immediate,		4,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x31,	"ADCR",		Immediate,		4,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x32,	"SUBR",		Immediate,		4,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x33,	"SBCR",		Immediate,		4,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x34,	"ANDR",		Immediate,		4,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x35,	"ORR",		Immediate,		4,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x36,	"EORR",		Immediate,		4,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x37,	"CMPR",		Immediate,		4,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x38,	"PSHSW",	Inherent,		6,		6,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x39,	"PULSW",	Inherent,		6,		6,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x3A,	"PSHUW",	Inherent,		6,		6,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x3B,	"PULUW",	Inherent,		6,		6,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x3C,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x3D,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x3E,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x3F,	"SWI2",		Inherent,		20,		22,		2,		2,	false,	Fixed,
																				
			0x40,	"NEGD",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x41,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x42,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x43,	"COMD",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809					
			0x44,	"LSRD",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x45,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x46,	"RORD",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x47,	"ASRD",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x48,	"LSLD",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x49,	"ROLD",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x4A,	"DECD",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x4B,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x4C,	"INCD",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x4D,	"TSTD",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x4E,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x4F,	"CLRD",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
																				
			0x50,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x51,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x52,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x53,	"COMW",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x54,	"LSRW",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x55,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x56,	"RORW",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x57,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x58,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x59,	"ROLW",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x5A,	"DECW",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x5B,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x5C,	"INCW",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x5D,	"TSTW",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x5E,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x5F,	"CLRW",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
																				
			0x60,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x61,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x62,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x63,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x64,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x65,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x66,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x67,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x68,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x69,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x6A,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x6B,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x6C,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x6D,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x6E,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x6F,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
																				
			0x70,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x71,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x72,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x73,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x74,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x75,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x76,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x77,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x78,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x79,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x7A,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x7B,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x7C,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x7D,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x7E,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x7F,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
																				
			0x80,	"SUBW",		Immediate,		5,		4,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x81,	"CMPW",		Immediate,		5,		4,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x82,	"SBCD",		Immediate,		5,		4,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x83,	"CMPD",		Immediate,		5,		4,		4,		2,	false,	Fixed,
			0x84,	"ANDD",		Immediate,		5,		4,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x85,	"BITD",		Immediate,		5,		4,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x86,	"LDW",		Immediate,		4,		4,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x87,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x88,	"EORD",		Immediate,		5,		4,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x89,	"ADCD",		Immediate,		5,		4,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x8A,	"ORD",		Immediate,		5,		4,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x8B,	"ADDW",		Immediate,		5,		4,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x8C,	"CMPY",		Immediate,		5,		4,		4,		2,	false,	Fixed,
			0x8D,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x8E,	"LDY",		Immediate,		4,		4,		4,		2,	false,	Fixed,
			0x8F,	"-",		Illegal,		0,		0,		0,		2,	false,	Fixed,
																				
			0x90,	"SUBW",		Direct,			7,		5,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x91,	"CMPW",		Direct,			7,		5,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x92,	"SBCD",		Direct,			7,		5,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x93,	"CMPD",		Direct,			7,		5,		3,		2,	false,	Fixed,
			0x94,	"ANDD",		Direct,			7,		5,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x95,	"BITD",		Direct,			7,		5,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x96,	"LDW",		Direct,			6,		5,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x97,	"STW",		Direct,			6,		5,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x98,	"EORD",		Direct,			7,		5,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x99,	"ADCD",		Direct,			7,		5,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x9A,	"ORD",		Direct,			7,		5,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x9B,	"ADDW",		Direct,			7,		5,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x9C,	"CMPY",		Direct,			7,		5,		3,		2,	false,	Fixed,
			0x9D,	"-",		Illegal,		0,		0,		0,		2,	false,	Fixed,
			0x9E,	"LDY",		Direct,			6,		5,		3,		2,	false,	Fixed,
			0x9F,	"STY",		Direct,			6,		5,		3,		2,	false,	Fixed,
																				
			0xA0,	"SUBW",		Indexed,		7,		6,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xA1,	"CMPW",		Indexed,		7,		6,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xA2,	"SBCD",		Indexed,		7,		5,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xA3,	"CMPD",		Indexed,		7,		6,		3,		2,	false,	IndexedModeAdjust,
			0xA4,	"ANDD",		Indexed,		7,		6,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xA5,	"BITD",		Indexed,		7,		6,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xA6,	"LDW",		Indexed,		6,		6,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xA7,	"STW",		Indexed,		6,		6,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xA8,	"EORD",		Indexed,		7,		6,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xA9,	"ADCD",		Indexed,		7,		6,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xAA,	"ORD",		Indexed,		7,		6,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xAB,	"ADDW",		Indexed,		7,		6,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xAC,	"CMPY",		Indexed,		7,		6,		3,		2,	false,	IndexedModeAdjust,
			0xAD,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xAE,	"LDY",		Indexed,		6,		6,		3,		2,	false,	IndexedModeAdjust,
			0xAF,	"STY",		Indexed,		6,		6,		3,		2,	false,	IndexedModeAdjust,
																				
			0xB0,	"SUBW",		Extended,		8,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xB1,	"CMPW",		Extended,		8,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xB2,	"SBCD",		Extended,		8,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xB3,	"CMPD",		Extended,		8,		6,		4,		2,	false,	Fixed,
			0xB4,	"ANDD",		Extended,		8,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xB5,	"BITD",		Extended,		8,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xB6,	"LDW",		Extended,		7,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xB7,	"STW",		Extended,		7,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xB8,	"EORD",		Extended,		8,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xB9,	"ADCD",		Extended,		8,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xBA,	"ORD",		Extended,		8,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xBB,	"ADDW",		Extended,		8,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xBC,	"CMPY",		Extended,		8,		6,		4,		2,	false,	Fixed,
			0xBD,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xBE,	"LDY",		Extended,		7,		6,		4,		2,	false,	Fixed,
			0xBF,	"STY",		Extended,		7,		6,		4,		2,	false,	Fixed,
																				
			0xC0,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC1,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC2,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC3,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC4,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC5,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC6,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC7,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC8,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC9,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xCA,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xCB,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xCC,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xCD,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xCE,	"LDS",		Immediate,		4,		4,		4,		2,	false,	Fixed,
			0xCF,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
																				
			0xD0,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xD1,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xD2,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xD3,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xD4,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xD5,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xD6,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xD7,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xD8,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xD9,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xDA,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xDB,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xDC,	"LDQ",		Direct,			8,		7,		3,		2,	true,	Fixed,						// Illegal on 6809
			0xDD,	"STQ",		Direct,			8,		7,		3,		2,	true,	Fixed,						// Illegal on 6809
			0xDE,	"LDS",		Direct,			6,		5,		3,		2,	false,	Fixed,
			0xDF,	"STS",		Direct,			6,		5,		3,		2,	false,	Fixed,
																				
			0xE0,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xE1,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xE2,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xE3,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xE4,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xE5,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xE6,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xE7,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xE8,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xE9,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xEA,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xEB,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xEC,	"LDQ",		Indexed,		0,		8,		3,		2,	false,	IndexedModeAdjust,
			0xED,	"STQ",		Indexed,		0,		8,		3,		2,	false,	IndexedModeAdjust,
			0xEE,	"LDS",		Indexed,		6,		6,		3,		2,	false,	IndexedModeAdjust,
			0xEF,	"STS",		Indexed,		6,		6,		3,		2,	false,	IndexedModeAdjust,
																			
			0xF0,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xF1,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xF2,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xF3,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xF4,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xF5,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xF6,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xF7,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xF8,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xF9,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xFA,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xFB,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xFC,	"LDQ",		Extended,		0,		8,		4,		2,	false,	Fixed,
			0xFD,	"STQ",		Extended,		0,		8,		4,		2,	false,	Fixed,
			0xFE,	"LDS",		Extended,		7,		6,		4,		2,	false,	Fixed,
			0xFF,	"STS",		Extended,		7,		6,		4,		2,	false,	Fixed,
		};

		// Page 3 operations 11xx codes
		std::array<OpCodeTables::OpCodeInfo, 256> Page3OpCodes =
		{
			0x00,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x01,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x02,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x03,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x04,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x05,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x06,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x07,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x08,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x09,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x0A,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x0B,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x0C,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x0D,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x0E,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x0F,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
																				
			0x10,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x11,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x12,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x13,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x14,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x15,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x16,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x17,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x18,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x19,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x1A,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x1B,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x1C,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x1D,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x1E,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x1F,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
																				
			0x20,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x21,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x22,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x23,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x24,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x25,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x26,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x27,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x28,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x29,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x2A,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x2B,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x2C,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x2D,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x2E,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x2F,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
																				
			0x30,	"BAND",		Direct,			7,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x31,	"BIAND",	Direct,			7,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x32,	"BOR",		Direct,			7,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x33,	"BIOR",		Direct,			7,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x34,	"BEOR",		Direct,			7,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x35,	"BIEOR",	Direct,			7,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x36,	"LDBT",		Direct,			7,		6,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x37,	"STBT",		Direct,			8,		7,		4,		2,	true,	Fixed,						// Illegal on 6809
			0x38,	"TFM",		Immediate,		6,		6,		3,		2,	true,	TFMAdjust,					// Illegal on 6809
			0x39,	"TFM",		Immediate,		6,		6,		3,		2,	true,	TFMAdjust,					// Illegal on 6809
			0x3A,	"TFM",		Immediate,		6,		6,		3,		2,	true,	TFMAdjust,					// Illegal on 6809
			0x3B,	"TFM",		Immediate,		6,		6,		3,		2,	true,	TFMAdjust,					// Illegal on 6809
			0x3C,	"BITMD",	Immediate,		4,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x3D,	"LDMD",		Immediate,		5,		5,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x3E,	"BREAK",	Inherent,		4,		2,		2,		2,	false,	Fixed,
			0x3F,	"SWI3",		Inherent,		20,		22,		2,		2,	false,	Fixed,
																				
			0x40,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x41,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x42,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x43,	"COME",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x44,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x45,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x46,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x47,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x48,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x49,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x4A,	"DECE",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x4B,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x4C,	"INCE",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x4D,	"TSTE",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x4E,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x4F,	"CLRE",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
																				
			0x50,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x51,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x52,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x53,	"COMF",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x54,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x55,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x56,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x57,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x58,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x59,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x5A,	"DECF",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x5B,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x5C,	"INCF",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x5D,	"TSTF",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
			0x5E,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x5F,	"CLRF",		Inherent,		3,		2,		2,		2,	true,	Fixed,						// Illegal on 6809
																				
			0x60,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x61,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x62,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x63,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x64,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x65,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x66,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x67,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x68,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x69,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x6A,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x6B,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x6C,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x6D,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x6E,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x6F,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
																				
			0x70,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x71,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x72,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x73,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x74,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x75,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x76,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x77,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x78,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x79,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x7A,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x7B,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x7C,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x7D,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x7E,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x7F,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
																				
			0x80,	"SUBE",		Immediate,		3,		3,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x81,	"CMPE",		Immediate,		3,		3,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x82,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x83,	"CMPU",		Immediate,		5,		4,		4,		2,	false,	Fixed,
			0x84,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x85,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x86,	"LDE",		Immediate,		3,		3,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x87,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x88,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x89,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x8A,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x8B,	"ADDE",		Immediate,		3,		3,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x8C,	"CMPS",		Immediate,		5,		4,		4,		2,	false,	Fixed,
			0x8D,	"DIVD",		Immediate,		25,		25,		3,		2,	true,	DIVAdjust,					// Illegal on 6809
			0x8E,	"DIVQ",		Immediate,		34,		34,		4,		2,	true,	DIVAdjust,					// Illegal on 6809
			0x8F,	"MULD",		Immediate,		28,		28,		4,		2,	true,	Fixed,						// Illegal on 6809
																				
			0x90,	"SUBE",		Direct,			5,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x91,	"CMPE",		Direct,			5,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x92,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x93,	"CMPU",		Direct,			7,		5,		3,		2,	false,	Fixed,
			0x94,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x95,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x96,	"LDE",		Direct,			5,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x97,	"STE",		Direct,			5,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x98,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x99,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x9A,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0x9B,	"ADDE",		Direct,			5,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0x9C,	"CMPS",		Direct,			7,		5,		3,		2,	false,	Fixed,
			0x9D,	"DIVD",		Direct,			27,		26,		3,		2,	true,	DIVAdjust,					// Illegal on 6809
			0x9E,	"DIVQ",		Direct,			36,		35,		3,		2,	true,	DIVAdjust,					// Illegal on 6809
			0x9F,	"MULD",		Direct,			30,		29,		3,		2,	true,	Fixed,						// Illegal on 6809
																				
			0xA0,	"SUBE",		Indexed,		5,		5,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xA1,	"CMPE",		Indexed,		5,		5,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xA2,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xA3,	"CMPU",		Indexed,		7,		6,		3,		2,	true,	IndexedModeAdjust,
			0xA4,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xA5,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xA6,	"LDE",		Indexed,		5,		5,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xA7,	"STE",		Indexed,		5,		5,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xA8,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xA9,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xAA,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xAB,	"ADDE",		Indexed,		5,		5,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xAC,	"CMPS",		Indexed,		7,		6,		3,		2,	false,	IndexedModeAdjust,
			0xAD,	"DIVD",		Indexed,		27,		27,		3,		2,	true,	DIVAdjust,					// Illegal on 6809
			0xAE,	"DIVQ",		Indexed,		36,		36,		3,		2,	true,	DIVAdjust,					// Illegal on 6809
			0xAF,	"MULD",		Indexed,		30,		30,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
																				
			0xB0,	"SUBE",		Extended,		6,		5,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xB1,	"CMPE",		Extended,		6,		5,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xB2,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xB3,	"CMPU",		Extended,		8,		6,		4,		2,	false,	Fixed,
			0xB4,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xB5,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xB6,	"LDE",		Extended,		6,		5,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xB7,	"STE",		Extended,		6,		5,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xB8,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xB9,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xBA,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xBB,	"ADDE",		Extended,		6,		5,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xBC,	"CMPS",		Extended,		8,		6,		4,		2,	false,	Fixed,
			0xBD,	"DIVD",		Extended,		28,		27,		4,		2,	true,	DIVAdjust,					// Illegal on 6809
			0xBE,	"DIVQ",		Extended,		37,		36,		4,		2,	true,	DIVAdjust,					// Illegal on 6809
			0xBF,	"MULD",		Extended,		31,		30,		4,		2,	true,	Fixed,						// Illegal on 6809 (Possible Adjust Needed?)
																				
			0xC0,	"SUBF",		Immediate,		3,		3,		3,		2,	true,	Fixed,						// Illegal on 6809
			0xC1,	"CMPF",		Immediate,		3,		3,		3,		2,	true,	Fixed,						// Illegal on 6809
			0xC2,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC3,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC4,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC5,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC6,	"LDF",		Immediate,		3,		3,		3,		2,	true,	Fixed,						// Illegal on 6809
			0xC7,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC8,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xC9,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xCA,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xCB,	"ADDF",		Immediate,		3,		3,		3,		2,	true,	Fixed,						// Illegal on 6809
			0xCC,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xCD,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xCE,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xCF,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
																				
			0xD0,	"SUBF",		Direct,			5,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0xD1,	"CMPF",		Direct,			5,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0xD2,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xD3,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xD4,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xD5,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xD6,	"LDF",		Direct,			5,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0xD7,	"STF",		Direct,			5,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0xD8,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xD9,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xDA,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xDB,	"ADDF",		Direct,			5,		4,		3,		2,	true,	Fixed,						// Illegal on 6809
			0xDC,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xDD,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xDE,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xDF,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
																			
			0xE0,	"SUBF",		Indexed,		5,		5,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xE1,	"CMPF",		Indexed,		5,		5,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xE2,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xE3,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xE4,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xE5,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xE6,	"LDF",		Indexed,		5,		5,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xE7,	"STF",		Indexed,		5,		5,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xE8,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xE9,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xEA,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xEB,	"ADDF",		Indexed,		5,		5,		3,		2,	true,	IndexedModeAdjust,			// Illegal on 6809
			0xEC,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xED,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xEE,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xEF,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
																		
			0xF0,	"SUBF",		Extended,		6,		5,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xF1,	"CMPF",		Extended,		6,		5,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xF2,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xF3,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xF4,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xF5,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xF6,	"LDF",		Extended,		6,		5,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xF7,	"STF",		Extended,		6,		5,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xF8,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xF9,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xFA,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xFB,	"ADDF",		Extended,		6,		5,		4,		2,	true,	Fixed,						// Illegal on 6809
			0xFC,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xFD,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xFE,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
			0xFF,	"-",		Illegal,		0,		0,		0,		2,	false,	None,
		};

		// Indexed Mode PostByte formats
		struct IndexModeInfo
		{
			std::string postbyte;		// 8-bit postbyte key
			std::string form;			// Assembler form
			int num6809cycles;			// Additional number of cycles (6809 only and 6309 in emulation)
			int num6309cycles;			// Additional number of cycles (6309 native only)
			int numbytes;				// Additional number of bytes
			bool only6309;				// Valid only if running a 6309, in other words, invalid on a 6809
		};

		std::map<std::string, IndexModeInfo> IndexingModes = 
		{ 
			{ "1RR00100", {"1RR00100", ",R",		0,	0,	0,	false } },
			{ "0RRnnnnn", {"0RRnnnnn", "n,R",		1,	1,	0,	false } },
			{ "1RR01000", {"1RR01000", "n,R",		1,	1,	1,	false } },
			{ "1RR01001", {"1RR01001", "n,R",		4,	3,	2,	false } },
			{ "10001111", {"10001111", ",W",		0,	0,	0,	true } },			// Illegal on 6809
			{ "10101111", {"10101111", "n,W",		2,	2,	2,	true } },			// Illegal on 6809
			{ "1RR00110", {"1RR00110", "A,R",		1,	1,	0,	false } },
			{ "1RR00101", {"1RR00101", "B,R",		1,	1,	0,	false } },
			{ "1RR01011", {"1RR01011", "D,R",		4,	2,	0,	false } },
			{ "1RR00111", {"1RR00111", "E,R",		1,	1,	0,	true } },			// Illegal on 6809
			{ "1RR01010", {"1RR01010", "F,R",		1,	1,	0,	true } },			// Illegal on 6809
			{ "1RR01110", {"1RR01110", "W,R",		1,	1,	0,	true } },			// Illegal on 6809
			{ "1RR00000", {"1RR00000", ",R+",		2,	1,	0,	false } },
			{ "1RR00001", {"1RR00001", ",R++",		3,	2,	0,	false } },
			{ "1RR00010", {"1RR00010", ",-R",		2,	1,	0,	false } },
			{ "1RR00011", {"1RR00011", ",--R",		3,	2,	0,	false } },
			{ "11001111", {"11001111", ",W++",		1,	1,	0,	true } },			// Illegal on 6809
			{ "11101111", {"11101111", ",--W",		1,	1,	0,	true } },			// Illegal on 6809
			{ "1XX01100", {"1XX01100", "n,PCR",		1,	1,	1,	false } },
			{ "1XX01101", {"1XX01101", "n,PCR",		5,	3,	2,	false } },
			{ "1RR10100", {"1RR10100", "[,R]",		3,	3,	0,	false } },
			{ "1RR11000", {"1RR11000", "[n,R]",		4,	4,	1,	false } },
			{ "1RR11001", {"1RR11001", "[n,R]",		7,	6,	2,	false } },
			{ "10010000", {"10010000", "[,W]",		3,	3,	0,	true } },			// Illegal on 6809
			{ "10110000", {"10110000", "[n,W]",		5,	5,	2,	true } },			// Illegal on 6809
			{ "1RR10110", {"1RR10110", "[A,R]",		4,	4,	0,	false } },
			{ "1RR10101", {"1RR10101", "[B,R]",		4,	4,	0,	false } },
			{ "1RR11011", {"1RR11011", "[D,R]",		7,	5,	0,	false } },
			{ "1RR10111", {"1RR10111", "[E,R]",		4,	4,	0,	true } },			// Illegal on 6809
			{ "1RR11010", {"1RR11010", "[F,R]",		4,	4,	0,	true } },			// Illegal on 6809
			{ "1RR01110", {"1RR01110", "[W,R]",		4,	4,	0,	true } },			// Illegal on 6809
			{ "1RR10001", {"1RR10001", "[,R++]",	6,	5,	0,	false } },
			{ "1RR10011", {"1RR10011", "[,--R]",	6,	5,	0,	false } },
			{ "11010000", {"11010000", "[,W++]",	4,	4,	0,	true } },			// Illegal on 6809
			{ "11110000", {"11110000", "[,--W]",	4,	4,	0,	true } },			// Illegal on 6809
			{ "1XX11100", {"1XX11100", "[n,PCR]",	4,	4,	1,	false } },
			{ "1XX11101", {"1XX11101", "[n,PCR]",	8,	6,	2,	false } },
			{ "10011111", {"10011111", "[n]",		5,	4,	2,	false } },
		};

	public:

		// Calculate the exact number of cycles based on CPU state
		bool ProcessHeuristics(OpCodeInfo& opcode, CPUState state, CPUTrace& trace);

	protected:

		bool ProcessNoAdjust(OpCodeInfo& opcode, CPUState state, CPUTrace& trace);
		bool ProcessIndexModeAdjust(OpCodeInfo& opcode, CPUState state, CPUTrace& trace);
		bool ProcessInterruptAdjust(OpCodeInfo& opcode, CPUState state, CPUTrace& trace);
		bool ProcessStackAdjust(OpCodeInfo& opcode, CPUState state, CPUTrace& trace);
		bool ProcessLongBranchAdjust(OpCodeInfo& opcode, CPUState state, CPUTrace& trace);
		bool ProcessTFMAdjust(OpCodeInfo& opcode, CPUState state, CPUTrace& trace);
		bool ProcessDIVAdjust(OpCodeInfo& opcode, CPUState state, CPUTrace& trace);
		bool ProcessWaitForSYNCAdjust(OpCodeInfo& opcode, CPUState state, CPUTrace& trace);

		int AdjustCycles(OpCodeInfo& opcode, int cycles, int bytes, bool IsNative6309);

		bool GetIndexMode(unsigned char postbyte, IndexModeInfo& mode, std::string& operand);
		std::string ToRegister(unsigned char postbyte);
		std::string ToInterRegister(unsigned char reg);
		std::string ToRelativeAddressString(int value, int oplen, int operandlen);

	};
}}

#pragma warning(pop)
