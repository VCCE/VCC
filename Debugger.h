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
#include <Windows.h>


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

		void QueueWrite(unsigned short addr, unsigned char value);

		void Update();


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
			PendingWrite() {};
		};

		bool HasPendingWriteNoLock() const;
		PendingWrite ConsumePendingWriteNoLock();
		void ProcessWrite(PendingWrite memWrite);

	private:

		mutable CriticalSection			Section_;
		bool							HasPendingCommand_;
		ExecutionMode					PendingCommand_;
		breakpointsbuffer_type			Breakpoints_;
		bool							BreakpointsChanged_ = false;
		CPUState						ProcessorState_;
		std::map<HWND, std::unique_ptr<Client>>	RegisteredClients_;
		ExecutionMode					ExecutionMode_ = ExecutionMode::Run;
		bool							HasPendingWrite_ = false;
		PendingWrite					PendingWrite_;
	};
} }