//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
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
//		OpDecoder Interface - Part of the Debugger package for VCC
//		Author: Mike Rojas
#include "OpCodeTables.h"
#include <stdexcept>
#include "tcc1014mmu.h"
#include <bitset>
#include "DebuggerUtils.h"
#include <sstream>
#include <iomanip>

namespace VCC { namespace Debugger
{
	bool OpCodeTables::ProcessHeuristics(OpCodeInfo& opcode, const CPUState& state, CPUTrace& trace)
	{
		bool valid = true;
		trace.decodeCycles = state.IsNative6309 ? opcode.num6309cycles : opcode.num6809cycles;

		switch (opcode.modifer)
		{
		case Heuristics::None:
		case Heuristics::Fixed:
			valid = ProcessNoAdjust(opcode, state, trace);
			break;
		case Heuristics::IndexedModeAdjust:
			valid = ProcessIndexModeAdjust(opcode, state, trace);
			break;
		case Heuristics::InterruptAdjust:
			valid = ProcessInterruptAdjust(opcode, state, trace);
			break;
		case Heuristics::StackAdjust:
			valid = ProcessStackAdjust(opcode, state, trace);
			break;
		case Heuristics::LongBranchAdjust:
			valid = ProcessLongBranchAdjust(opcode, state, trace);
			break;
		case Heuristics::TFMAdjust:
			valid = ProcessTFMAdjust(opcode, state, trace);
			break;
		case Heuristics::DIVAdjust:
			valid = ProcessDIVAdjust(opcode, state, trace);
			break;
		case Heuristics::WaitForSYNCAdjust:
			valid = ProcessWaitForSYNCAdjust(opcode, state, trace);
			break;
		default:
			valid = false;
			throw std::runtime_error("Unhandled opcode heuristic");
		}

		return valid;
	}

	bool OpCodeTables::ProcessNoAdjust(const OpCodeInfo& opcode, const CPUState& state, CPUTrace& trace)
	{
		// Get the current PC.
		unsigned short PC = state.PC;

		// Skip past opcode bytes
		PC += opcode.oplen;

		// Number of bytes for the operand.
		int operandLen = opcode.numbytes - opcode.oplen;
		for (int n = 0; n < operandLen; n++)
		{
			trace.bytes.push_back(DbgRead8(state.phyAddr,state.block,PC++));
		}

		int operand = 0;

		switch (operandLen)
		{
		case 0:
			break;
		case 1:
			operand = trace.bytes[opcode.oplen];
			break;
		case 2:
			operand = (trace.bytes[opcode.oplen] << 8) + trace.bytes[opcode.oplen + 1];
			break;
		}

		switch (opcode.mode)
		{
		case Inherent:
			break;
		case Direct:
			trace.operand = "<$" + ToHexString(operand, 2);
			break;
		case Indexed:
			break;
		case Relative:
			trace.operand = ToRelativeAddressString(operand, opcode.oplen, operandLen);
			break;
		case Extended:
			trace.operand = "$" + ToHexString(operand, 4);
			break;
		case Immediate:
		{
			// TFR and EXG - use inter-register operand
			if (opcode.opcode == 0x1F || opcode.opcode == 0x1E)
			{
				// Get source and destination registers 
				unsigned char src = operand >> 4;
				unsigned char dst = operand & 0b00001111;

				trace.operand = ToInterRegister(src) + "," + ToInterRegister(dst);
			}
			else
			{ 
				trace.operand = "#$" + ToHexString(operand, 2);
			}
		}
			break;
		case LongRelative:
			break;
		}

		return true;
	}

	bool OpCodeTables::ProcessIndexModeAdjust(OpCodeInfo& opcode, const CPUState& state, CPUTrace& trace)
	{
		// Get the current PC.
		unsigned short PC = state.PC;

		// Get the post byte.
		PC += opcode.oplen;
		unsigned char postbyte = DbgRead8(state.phyAddr,state.block,PC);

		trace.bytes.push_back(postbyte);

		// Determine various indexing modes.
		IndexModeInfo mode;
		if (!GetIndexMode(postbyte, mode, trace.operand))
		{
			// Index mode is not valid.
			return false;
		}

		// One byte offset?
		if (mode.numbytes == 1)
		{
			unsigned char b = DbgRead8(state.phyAddr,state.block,++PC);
			trace.bytes.push_back(b);
			std::int32_t offset = b;
			offset = (offset << 24) >> 24;
			replace(trace.operand, "n", std::to_string(offset));
		}

		// Two byte offset?
		if (mode.numbytes == 2)
		{
			unsigned char b1 = DbgRead8(state.phyAddr,state.block,++PC);
			trace.bytes.push_back(b1);
			unsigned char b2 = DbgRead8(state.phyAddr,state.block,++PC);
			trace.bytes.push_back(b2);
			std::int32_t offset = (b1 << 8) + b2;
			// Extended Indirect?
			if (trace.operand == "[n]")
			{
				replace(trace.operand, "n", "$" + ToHexString(offset, 4));
			}
			else
			{
				offset = (offset << 16) >> 16;
				replace(trace.operand, "n", std::to_string(offset));
			}
		}

		// Adjust for number of cycles.
		int additionalCycles = (state.IsNative6309) ? mode.num6309cycles : mode.num6809cycles;
		trace.decodeCycles = AdjustCycles(opcode, additionalCycles, mode.numbytes, state.IsNative6309);

		return true;
	}

	bool OpCodeTables::GetIndexMode(unsigned char postbyte, IndexModeInfo& mode, std::string& operand)
	{
		// Convert to key.
		std::string key = std::bitset<8>(postbyte).to_string();

		// MSB = 0?  5 bit offset.
		if (key[0] == '0')
		{
			mode = IndexingModes["0RRnnnnn"];
			operand = mode.form;

			// Determine the register.
			replace(operand, "R", ToRegister(postbyte));

			// Convert 5 bits to sign extended offset (-16 to +15)
			std::int32_t offset = std::bitset<5>(key.substr(3, 5)).to_ulong();
			offset = (offset << 27) >> 27;
			replace(operand, "n", std::to_string(offset));

			return true;
		}

		// Try exact match.
		if (IndexingModes.find(key) != IndexingModes.end())
		{
			mode = IndexingModes[key];
			operand = mode.form;
			return true;
		}

		// Try PCR modes.
		key[1] = 'X';
		key[2] = 'X';
		if (IndexingModes.find(key) != IndexingModes.end())
		{
			mode = IndexingModes[key];
			operand = mode.form;
			return true;
		}

		// Try register modes.
		key[1] = 'R';
		key[2] = 'R';
		if (IndexingModes.find(key) != IndexingModes.end())
		{
			mode = IndexingModes[key];
			operand = mode.form;
			replace(operand, "R", ToRegister(postbyte));
			return true;
		}

		// Invalid mode.
		return false;
	}

	bool OpCodeTables::ProcessInterruptAdjust(OpCodeInfo& opcode, const CPUState& state, CPUTrace& trace)
	{
		trace.decodeCycles = AdjustCycles(opcode, 0, 0, state.IsNative6309);
		return true;
	}

	bool OpCodeTables::ProcessStackAdjust(OpCodeInfo& opcode, const CPUState& state, CPUTrace& trace)
	{
		// Get the current PC.
		unsigned short PC = state.PC;

		// Get the post byte.
		unsigned char postbyte = DbgRead8(state.phyAddr,state.block,++PC);
		trace.bytes.push_back(postbyte);

		// Count the number of registers set in post byte.
		int additionalCycles = 0;
		int regIndex = 0;
		std::array<std::string, 8> registers = { "CC", "A", "B", "DP", "X", "Y", "U/S", "PC" };
		std::array<int, 8> registerSize = { 1, 1, 1, 1, 2, 2, 2, 2 };
		registers[6] = opcode.opcode & 0b00000010 ? "S" : "U";
		while (postbyte) 
		{
			int bit = postbyte & 1;
			if (bit)
			{
				if (trace.operand.size() > 0)
				{
					trace.operand += ",";
				}
				trace.operand += registers[regIndex];
				// One additional cycle per byte pushed/pulled.
				additionalCycles += registerSize[regIndex];
			}
			postbyte >>= 1;
			regIndex++;
		}

		// Adjust the number of cycles.
		trace.decodeCycles = AdjustCycles(opcode, additionalCycles, 0, state.IsNative6309);
		return true;
	}

	bool OpCodeTables::ProcessLongBranchAdjust(OpCodeInfo& opcode, const CPUState& state, CPUTrace& trace)
	{
		// Get the current PC.
		unsigned short PC = state.PC;

		// Skip past opcode bytes
		PC += opcode.oplen;

		// Number of bytes for the operand.
		int operandLen = opcode.numbytes - opcode.oplen;
		for (int n = 0; n < operandLen; n++)
		{
			trace.bytes.push_back(DbgRead8(state.phyAddr,state.block,PC++));
		}

		// All long branches are relative.
		int operand = (trace.bytes[opcode.oplen] << 8) + trace.bytes[opcode.oplen + 1];
		trace.operand = ToRelativeAddressString(operand, opcode.oplen, operandLen);

		// No adjustment needed on 6309 
		if (state.IsNative6309)
		{
			trace.decodeCycles = AdjustCycles(opcode, 0, 0, state.IsNative6309);
			return true;
		}

		// We need to determine if the branch will be taken
		bool willBranch = false;
		bool C = (state.CC & 0b00000001) == 1;
		bool V = (state.CC & 0b00000010) == 1;	//	FIXME-CHET: This will never be true because the result can never be 1
		bool Z = (state.CC & 0b00000100) == 1;	//	FIXME-CHET: This will never be true because the result can never be 1
		bool N = (state.CC & 0b00001000) == 1;	//	FIXME-CHET: This will never be true because the result can never be 1

		// Will the branch be taken?
		switch (opcode.opcode)
		{
		case 0x24:	// LBCC, LBHS
			willBranch = !C;				// IF CC.C = 0 then PC <- PC + IMM
			break;
		case 0x25:	// LBCS, LBLO
			willBranch = C;					// IF CC.C = 1 then PC <- PC + IMM
			break;
		case 0x28:	// LBVC
			willBranch = !V;				// IF CC.V = 0 then PC <- PC + IMM
			break;
		case 0x29:	// LBVS
			willBranch = V;					// IF CC.V = 1 then PC <- PC + IMM
			break;
		case 0x26:	// LBNE
			willBranch = !Z;				// IF CC.Z = 0 then PC <- PC + IMM
			break;
		case 0x27:	// LBEQ
			willBranch = Z;					// IF CC.Z = 1 then PC <- PC + IMM
			break;
		case 0x2A:	// LBPL
			willBranch = !N;				// IF CC.N = 0 then PC <- PC + IMM
			break;
		case 0x2B:	// LBMI
			willBranch = N;					// IF CC.N = 1 then PC <- PC + IMM
			break;
		case 0x2C:	// LBGE
			willBranch = N == V;			// IF CC.N = CC.V then PC <- PC + IMM
			break;
		case 0x2F:	// LBLE
			willBranch = (N != V) || Z;		// IF (CC.N <> CC.V) OR (CC.Z = 1) then PC <- PC + IMM
			break;
		case 0x2E:	// LBGT
			willBranch = (N == V) && !Z;	// IF (CC.N = CC.V) AND (CC.Z = 0) then PC <- PC + IMM
			break;
		case 0x2D:	// LBLT
			willBranch = N != V;			// IF CC.N <> CC.V then PC <- PC + IMM
			break;
		case 0x22:	// LBHI
			willBranch = !Z && !C;			// IF (CC.Z = 0) AND (CC.C = 0) then PC <- PC + IMM
			break;
		case 0x23:	// LBLS
			willBranch = Z || C;			// IF (CC.Z = 1) OR (CC.C = 1) then PC <- PC + IMM
			break;
		}

		// Additional cycle if branch is taken.
		trace.decodeCycles = AdjustCycles(opcode, willBranch ? 1 : 0, 0, state.IsNative6309);
		return true;
	}

	bool OpCodeTables::ProcessTFMAdjust(OpCodeInfo& opcode, const CPUState& state, CPUTrace& trace)
	{
		// Get the current PC.
		unsigned short PC = state.PC;

		// Get the post byte.
		unsigned char postbyte = DbgRead8(state.phyAddr,state.block,++PC);
		trace.bytes.push_back(postbyte);

		// Get source and destination registers 
		unsigned char src = postbyte >> 4;
		unsigned char dst = postbyte & 0b00001111;

		trace.operand = ToInterRegister(src) + "," + ToInterRegister(dst);

		// Assume invalid.
		int additionalCycles = 0;

		// Only D, X, Y, U, S are valid.
		if (src < 0b0101 && dst < 0b0101)
		{
			// Get number of bytes that will be transferred.
			unsigned short W = (state.E << 8) + state.F;

			// Calculate total cycles to complete.
			additionalCycles = 3 * W;
		}

		// Adjust the number of cycles.
		trace.decodeCycles = AdjustCycles(opcode, additionalCycles, 0, state.IsNative6309);
		return true;
	}

	bool OpCodeTables::ProcessDIVAdjust(OpCodeInfo& opcode, const CPUState& state, CPUTrace& trace)
	{
		// Two rules:
		//	1) DIVD executes in l fewer cycle if a two's-complement overflow occurs. 
		//	2) Range error means fewer cycles: (13 fewer for DIVD, 21 fewer for DIVQ)

		// Get the current PC.
		unsigned short PC = state.PC;

		// Determine width.
		bool wide = (opcode.opcode == 0x8E);

		// Get the dividend
		int dividend = (state.A << 8) + state.B;

		// Get the divisor
		int divisor;
		if (wide)
		{
			unsigned char b1 = DbgRead8(state.phyAddr,state.block,++PC);
			trace.bytes.push_back(b1);
			unsigned char b2 = DbgRead8(state.phyAddr,state.block,++PC);
			trace.bytes.push_back(b2);
			divisor = (b1 << 8) + b2;
		}
		else
		{
			unsigned char b1 = DbgRead8(state.phyAddr,state.block,++PC);
			trace.bytes.push_back(b1);
			divisor = b1;
		}

		// Calculate the quotient
		int quotient = dividend / divisor;

		int additionalCycles = 0;

		// Range Error?
		if (!wide && ((quotient > 255) || (quotient < -256)) || 
			(wide && ((quotient > 65535) || (quotient < -65536))))
		{
			additionalCycles = wide ? -21: -13;
		}
		// 2-complement Overflow?
		else if (!wide && ((quotient > 127) || (quotient < -128)) ||
			(wide && ((quotient > 32767) || (quotient < -32768))))
		{
			additionalCycles = -1;
		}

		trace.decodeCycles = AdjustCycles(opcode, additionalCycles, 0, state.IsNative6309);
		return true;
	}

	bool OpCodeTables::ProcessWaitForSYNCAdjust(OpCodeInfo& opcode, const CPUState& state, CPUTrace& trace)
	{
		trace.decodeCycles = AdjustCycles(opcode, 0, 0, state.IsNative6309);
		return true;
	}

	std::string OpCodeTables::ToInterRegister(unsigned char reg)
	{
		std::vector<std::string> regs = { "D", "X", "Y", "U", "S", "PC", "W", "V", "A", "B", "CC", "DP", "0", "0", "E", "F" };
		return regs[reg];
	}

	std::string OpCodeTables::ToRegister(unsigned char postbyte)
	{
		int reg = (postbyte & 0b01100000) >> 5;
		switch (reg)
		{
		case 0b00:
			return "X";
		case 0b01:
			return "Y";
		case 0b10:
			return "U";
		case 0b11:
			return "S";
		}
		return "?";
	}

	std::string OpCodeTables::ToRelativeAddressString(int value, int oplen, int operandlen)
	{
		std::ostringstream fmt;

		int length = operandlen * 2;
		int relativeOffset = length + value;
		switch (operandlen)
		{
			case 0:
				if (relativeOffset > 15) relativeOffset -= 16;
				break;
			case 1:
				if (relativeOffset > 127) relativeOffset -= 256;
				break;
			case 2:
				if (relativeOffset > 32767) relativeOffset -= 65536;
				break;
		}

		fmt << "*";
		if (relativeOffset < 0)
		{
			relativeOffset *= -1;
			fmt << "-$" << std::hex << std::setw(length) << std::uppercase << std::setfill('0') << relativeOffset;
		}
		else if (relativeOffset > 0)
		{
			fmt << "+$" << std::hex << std::setw(length) << std::uppercase << std::setfill('0') << relativeOffset;
		}

		return fmt.str();
	}

	int OpCodeTables::AdjustCycles(OpCodeInfo& opcode, int cycles, int bytes, bool IsNative6309)
	{
		if (IsNative6309)
		{
			opcode.num6309cycles += cycles;
			cycles = opcode.num6309cycles;
		}
		else
		{
			opcode.num6809cycles += cycles;
			cycles = opcode.num6809cycles;
		}
		opcode.numbytes += bytes;
		return cycles;
	}
} }
