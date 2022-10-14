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
#include <Windows.h>
#include <string>
#include "optional.hpp"
#include <string>
#include <vector>
#include <functional>
#include <map>


namespace VCC { namespace Debugger
{

	class Debugger
	{
	public:

		using callback_type = std::function<void(HWND)>;
		using breakpointsbuffer_type = std::vector<unsigned short>;


	public:

		void Reset();

		void RegisterClient(HWND window, callback_type updateCallback, callback_type resetCallback);
		void RemoveClient(HWND window);

		void SetBreakpoints(breakpointsbuffer_type breakpoints);

		CPUState GetProcessorStateCopy() const;

		bool IsHalted() const;
		bool IsHalted(tl::optional<unsigned short>& pc) const;
		bool IsStepping() const;
		
		void Halt();

		void QueueRun();
		void QueueStep();
		void QueueHalt();

		void Update();


	protected:

		enum class ExecutionMode
		{
			Halt,
			Run,
			Step
		};

		bool HasPendingCommand() const;
		ExecutionMode ConsumePendingCommand();
		ExecutionMode ProcessNewMode(ExecutionMode cpuCmd);


	private:

		mutable CriticalSection			Section_;
		tl::optional<ExecutionMode>	PendingCommand;
		breakpointsbuffer_type			Breakpoints_;
		bool							BreakpointsChanged_ = false;
		CPUState						ProcessorState_;
		std::map<HWND, callback_type>	UpdateCallbacks_;
		std::map<HWND, callback_type>	ResetCallbacks_;
		ExecutionMode					ExecutionMode_ = ExecutionMode::Run;
	};
} }