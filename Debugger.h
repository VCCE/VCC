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
//		Debugger Interface - Part of the Debugger package for VCC
//		Author: Chet Simpson
#pragma once
#include "DebuggerUtils.h"
#include "MachineDefs.h"
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <memory>
#include <atomic>
#include <Windows.h>
#include "OpDecoder.h"


namespace VCC { namespace Debugger
{

	struct Client
	{
		virtual ~Client() = default;

		virtual void OnReset() = 0;
		virtual void OnUpdate() = 0;
	};

	class Debugger
	{
	public:

		using callback_type = std::function<void(HWND)>;
		using breakpointsbuffer_type = std::vector<unsigned short>;
		using triggerbuffer_type = std::vector<unsigned short>;
		using tracebuffer_type = std::vector<CPUTrace>;

	public:

		void Reset();

		void RegisterClient(HWND window, std::unique_ptr<Client> client);
		void RemoveClient(HWND window);

		void SetBreakpoints(breakpointsbuffer_type breakpoints);

		CPUState GetProcessorStateCopy() const;

		bool IsHalted() const;
		bool IsHalted(unsigned short& pc) const;
		bool IsStepping() const;
		
		void Halt();

		void QueueRun();
		void QueueStep();
		void QueueHalt();
	    void ToggleRun();

		void QueueWrite(unsigned short addr, unsigned char value);

		bool IsTracing() const;
		bool IsTracingEnabled() const;

		void TraceStart();
		void TraceStop();
		void TraceCaptureBefore(long cycles, CPUState state);
		void TraceCaptureAfter(long cycles, CPUState state);
		void TraceCaptureInterruptRequest(unsigned char irq, long cycleTime, CPUState state);
		void TraceCaptureInterruptMasked(unsigned char irq, long cycleTime, CPUState state);
		void TraceCaptureInterruptServicing(unsigned char irq, long cycleTime, CPUState state);
		void TraceCaptureInterruptExecuting(unsigned char irq, long cycleTime, CPUState state);
		void TraceCaptureScreenEvent(TraceEvent evt, double cycles);
		void TraceEmulatorCycle(TraceEvent evt, int state, double lineNS, double irqNS, double soundNS, double cycles, double drift);

		void SetTraceEnable();
		void SetTraceDisable();
		void ResetTrace();
		void SetTraceMaxSamples(long samples);
		void SetTraceStartTriggers(triggerbuffer_type startTriggers);
		void SetTraceStopTriggers(triggerbuffer_type stopTriggers);
		void SetTraceOptions(bool screen, bool emulation);

		long GetTraceSamples() const;
		void LockTrace() const;
		void UnlockTrace() const;
		const tracebuffer_type& GetTraceResult() const;
		void SetTraceMark(int mark, long sample);
		void ClearTraceMarks();
		void GetTraceMarkSamples(tracebuffer_type& result) const;

		void Update();

		// VCC HALT and BREAK instuctions status and enable. A BREAK instruction
		// can be assembled into code using the lwasm assembler. The HALT
		// instruction is used by Vcc as a breakpoint mechanism (called
		// haltpoints to avoid conflict with the mechanism used by the source
		// code debugger)  BREAK instruction is page two opcodes 0x113E and the
		// HALT instruction is opcode 0x15.
		bool Break_Enabled();
		void Enable_Break(bool);
		bool Halt_Enabled();
		void Enable_Halt(bool);

	protected:

		enum class ExecutionMode
		{
			Halt,
			Run,
			Step
		};

		bool HasPendingCommandNoLock() const;
		ExecutionMode ConsumePendingCommandNoLock();
		ExecutionMode ProcessNewModeNoLock(ExecutionMode cpuCmd) const;

		struct PendingWrite
		{
			unsigned short addr;
			unsigned char value;
			PendingWrite(unsigned short addr, unsigned char value) : addr(addr), value(value) {};
			PendingWrite() : addr(0),value(0) {};
		};

		bool HasPendingWriteNoLock() const;
		PendingWrite ConsumePendingWriteNoLock();
		void ProcessWrite(PendingWrite memWrite);

		void CheckStopTrace(CPUState state);

	private:

		mutable CriticalSection			Section_;
		bool							HasPendingCommand_;
		ExecutionMode					PendingCommand_;
		breakpointsbuffer_type			Breakpoints_;
		bool							BreakpointsChanged_ = false;
		CPUState						ProcessorState_;
		std::map<HWND, std::unique_ptr<Client>>	RegisteredClients_;
		std::atomic<ExecutionMode>		ExecutionMode_ = ExecutionMode::Run;
		//
		bool							HasPendingWrite_ = false;
		PendingWrite					PendingWrite_;
		//
		std::unique_ptr<OpDecoder>		Decoder_;
		std::atomic<bool>				TraceEnabled_ = false;
		std::atomic<bool>				TraceRunning_ = false;
		size_t							TraceMaxSamples_ = 500000;
		long							TraceSamplesCollected_ = 0;
		bool							TraceEmulation_ = false;
		bool							TraceScreen_ = false;
		triggerbuffer_type				TraceStartTriggers_;
		triggerbuffer_type				TraceStopTriggers_;
		bool							TraceTriggerChanged_ = false;
		tracebuffer_type				TraceCaptured_;
		std::map<int, long>				TraceMarks_;
		bool							Break_Enabled_TF = false;
		bool							Halt_Enabled_TF = false;
	};
} }
