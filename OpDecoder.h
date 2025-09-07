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
#pragma once
#include "Windows.h"
#include <string>
#include <array>
#include <memory>
#include "OpCodeTables.h"
#include "MachineDefs.h"

namespace VCC { namespace Debugger
{
	class OpDecoder
	{
	public:

		OpDecoder() 
		{
			Reset(500000);
		};

		// Interrupt Types on a 6x09 microprocessor
		enum class IRQType
		{
			InterruptRequest,
			FastInterruptRequest,
			NonMaskableInterrupt
		};

		void Reset(long maxSamples);
		void CaptureBefore(long cycleTime, CPUState state);
		void CaptureAfter(long cycleTime, CPUState state);
		void CaptureInterrupt(TraceEvent evt, IRQType irq, long cycleTime, CPUState state);
		void CaptureScreenEvent(TraceEvent evt, double cycles);
		void CaptureEmulatorCycle(TraceEvent evt, int state, double lineNS, double irqNS, double soundNS, double cycles, double drift);

		const std::vector<CPUTrace>& GetTrace() const;

		size_t GetSampleCount() const;

		IRQType ToIRQType(unsigned char irq);

		bool DecodeInstruction(CPUState state, CPUTrace& trace);

	protected:

		bool DecodeInterrupt(TraceEvent evt, IRQType irq, std::string& interrupt);
		bool DecodeScreen(TraceEvent evt, std::string& screen);
		bool DecodePage2Instruction(unsigned short PC, CPUState state, CPUTrace& trace);
		bool DecodePage3Instruction(unsigned short PC, CPUState state, CPUTrace& trace);

	private:

		std::unique_ptr<OpCodeTables>	OpCodeTables_;
		std::vector<CPUTrace>			TraceCaptured_;
		CPUTrace						CurrentTrace_;
		long							TotalCycles_ = 0;
		long							BeforeCycles_ = 0;
		long							AfterCycles_ = 0;
		int								CurrentFrame_ = 0;
		int								CurrentLine_ = 0;
	};

} }
