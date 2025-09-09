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
#include "OpDecoder.h"
#include "tcc1014mmu.h"
#include "DebuggerUtils.h"
#include "defines.h"

namespace VCC { namespace Debugger
{

	void OpDecoder::Reset(long maxSamples)
	{
		TraceCaptured_.clear();
		TraceCaptured_.reserve(maxSamples);
		TotalCycles_ = 0;
		OpCodeTables_ = std::make_unique<OpCodeTables>();
	}

	void OpDecoder::CaptureBefore(long cycleTime, const CPUState& state)
	{
		BeforeCycles_ = cycleTime;
		CurrentTrace_.event = TraceEvent::Instruction;
		CurrentTrace_.startState = state;
		CurrentTrace_.pc = state.PC;
		CurrentTrace_.bytes.clear();
		CurrentTrace_.instruction.clear();
		CurrentTrace_.operand.clear();

		DecodeInstruction(state, CurrentTrace_);
	}

	void OpDecoder::CaptureAfter(long cycleTime, const CPUState& state)
	{
		AfterCycles_ = cycleTime;
		CurrentTrace_.cycleTime = TotalCycles_;
		CurrentTrace_.endState = state;
		CurrentTrace_.execCycles = AfterCycles_ - BeforeCycles_;
		TotalCycles_ += CurrentTrace_.execCycles;
		TraceCaptured_.push_back(CurrentTrace_);
	}

	void OpDecoder::CaptureInterrupt(TraceEvent evt, IRQType irq, long cycleTime, const CPUState& state)
	{
		CPUTrace interrupt;
		interrupt.event = evt;
		interrupt.cycleTime = TotalCycles_;
		CurrentTrace_.pc = state.PC;
		interrupt.startState = state;
		interrupt.endState = state;
		interrupt.decodeCycles = cycleTime;
		interrupt.execCycles = cycleTime;
		TotalCycles_ += interrupt.execCycles;
		DecodeInterrupt(evt, irq, interrupt.instruction);
		TraceCaptured_.push_back(interrupt);
	}

	void OpDecoder::CaptureScreenEvent(TraceEvent evt, double cycles)
	{
		int execCycles = (int)floor(cycles);
		CPUTrace screen;
		DecodeScreen(evt, screen.instruction);
		screen.event = evt;
		screen.cycleTime = TotalCycles_;
		screen.pc = 0;
		screen.startState = { 0 };
		screen.endState = { 0 };
		screen.decodeCycles = execCycles;
		screen.execCycles = execCycles;
		screen.frame = CurrentFrame_;
		screen.line = CurrentLine_;
//		TotalCycles_ += screen.execCycles;
		TraceCaptured_.push_back(screen);
	}

	void OpDecoder::CaptureEmulatorCycle(TraceEvent evt, int state, double lineNS, double irqNS, double soundNS, double cycles, double drift)
	{
		CPUTrace emulator;
		emulator.event = evt;
		emulator.cycleTime = 0;
		emulator.startState = { 0 };
		emulator.endState = { 0 };
		emulator.decodeCycles = 0;
		emulator.execCycles = 0;
		emulator.frame = 0;
		emulator.line = 0;
		emulator.emulationState = state;
		emulator.emulator.push_back(lineNS);
		emulator.emulator.push_back(irqNS);
		emulator.emulator.push_back(soundNS);
		emulator.emulator.push_back(cycles);
		emulator.emulator.push_back(drift);
		emulator.emulator.push_back(cycles + -drift);
		TraceCaptured_.push_back(emulator);
	}

	const std::vector<VCC::CPUTrace>& OpDecoder::GetTrace() const
	{
		return TraceCaptured_;
	}

	size_t OpDecoder::GetSampleCount() const
	{
		return TraceCaptured_.size();
	}

	OpDecoder::IRQType OpDecoder::ToIRQType(unsigned char irq) const
	{
		switch (irq)
		{
		case INT_IRQ:
			return IRQType::InterruptRequest;
		case INT_FIRQ:
			return IRQType::FastInterruptRequest;
		case INT_NMI:
			return IRQType::NonMaskableInterrupt;
		}
		return IRQType();
	}

	bool OpDecoder::DecodeInterrupt(TraceEvent evt, IRQType irq, std::string& interrupt) const
	{
		switch (irq)
		{
		case IRQType::InterruptRequest:
			interrupt += "[IRQ]";
			break;
		case IRQType::FastInterruptRequest:
			interrupt += "[FIRQ]";
			break;
		case IRQType::NonMaskableInterrupt:
			interrupt += "[NMI]";
			break;
		}

		switch (evt)
		{
		case TraceEvent::IRQRequested:
			interrupt += " :: Request";
			break;
		case TraceEvent::IRQMasked:
			interrupt += " :: Ignored";
			break;
		case TraceEvent::IRQServicing:
			interrupt += " :: Service";
			break;
		case TraceEvent::IRQExecuting:
			interrupt += " :: Execute";
			break;
		}

		return true;
	}

	bool OpDecoder::DecodeScreen(TraceEvent evt, std::string& screen)
	{
		switch (evt)
		{
		case TraceEvent::ScreenTopBorder:
			screen = "[Top Border]";
			break;
		case TraceEvent::ScreenRender:
			screen = "[Render]";
			break;
		case TraceEvent::ScreenBottomBorder:
			screen = "[Bot Border]";
			break;
		case TraceEvent::ScreenVSYNCLow:
			screen = "[VSync Low ";
			screen += std::to_string(CurrentFrame_);
			screen += "]";
			CurrentLine_ = 0;
			TotalCycles_ = 0;
			break;
		case TraceEvent::ScreenVSYNCHigh:
			screen = "[VSync High ";
			screen += std::to_string(CurrentFrame_++);
			screen += "]";
			break;
		case TraceEvent::ScreenHSYNCLow:
			screen = "[HSync Low ";
			screen += std::to_string(CurrentLine_);
			screen += "]";
			break;
		case TraceEvent::ScreenHSYNCHigh:
			screen = "[HSync High ";
			screen += std::to_string(CurrentLine_++);
			screen += "]";
			break;
		}

		return true;
	}

	bool OpDecoder::DecodeInstruction(const CPUState& state, CPUTrace& trace)
	{
		// Get the address of the next Op Code.
		unsigned short PC = state.PC;

		// Get the Op Code.
		unsigned char Op1 = DbgRead8(state.phyAddr,state.block,PC);

		// Save it.
		trace.bytes.push_back(Op1);

		// Decode it.
		OpCodeTables::OpCodeInfo Opcode = OpCodeTables_->Page1OpCodes[Op1];

		// Page 2 Opcode?
		if (Opcode.mode == OpCodeTables::AddressingMode::OpPage2)
		{
			return DecodePage2Instruction(++PC, state, trace);
		}

		// Page 3 Opcode?
		if (Opcode.mode == OpCodeTables::AddressingMode::OpPage3)
		{
			return DecodePage3Instruction(++PC, state, trace);
		}

		// Page 1 Opcode =================
		trace.instruction = Opcode.name;

		// Calculate cycles based on state.
		OpCodeTables_->ProcessHeuristics(Opcode, state, trace);

		return true;
	}

	bool OpDecoder::DecodePage2Instruction(unsigned short PC, const CPUState& state, CPUTrace& trace)
	{
		// Get the extended Op Code.
		unsigned char Op2 = DbgRead8(state.phyAddr,state.block,PC);

		// Save it.
		trace.bytes.push_back(Op2);

		// Decode it.
		OpCodeTables::OpCodeInfo Opcode = OpCodeTables_->Page2OpCodes[Op2];

		// Page 2 Opcode =================
		trace.instruction = Opcode.name;

		// Calculate cycles based on state.
		OpCodeTables_->ProcessHeuristics(Opcode, state, trace);

		return true;
	}
	
	bool OpDecoder::DecodePage3Instruction(unsigned short PC, const CPUState& state, CPUTrace& trace)
	{
		// Get the extended Op Code.
		unsigned char Op3 = DbgRead8(state.phyAddr,state.block,PC);

		// Save it.
		trace.bytes.push_back(Op3);

		// Decode it.
		OpCodeTables::OpCodeInfo Opcode = OpCodeTables_->Page3OpCodes[Op3];

		// Page 3 Opcode =================
		trace.instruction = Opcode.name;

		// Calculate cycles based on state.
		OpCodeTables_->ProcessHeuristics(Opcode, state, trace);

		return true;
	}

} }
