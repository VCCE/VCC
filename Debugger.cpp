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
#include "Debugger.h"
#include "DebuggerUtils.h"
#include "defines.h"	//	FIXME: cross dependency as Debugger is member of SystemState defined in defines.h
#include <sstream>
#include <iomanip>


namespace VCC { namespace Debugger
{

	void Debugger::Reset()
	{
		SectionLocker lock(Section_);

		ExecutionMode_ = ExecutionMode::Run;
		PendingCommand.reset();
		ProcessorState_ = CPUState();

		for (const auto& callback : ResetCallbacks_)
		{
			callback.second(callback.first);
		}
	}



	void Debugger::RegisterClient(HWND window, callback_type updateCallback, callback_type resetCallback)
	{
		SectionLocker lock(Section_);

		if (!updateCallback && !resetCallback)
		{
			throw std::invalid_argument("updateCallback and resetCallback cannot both be null");
		}

		if (updateCallback)
		{
			if (UpdateCallbacks_.find(window) != UpdateCallbacks_.end())
			{
				throw std::invalid_argument("Udpate client is already registered with the debugger");
			}

			UpdateCallbacks_[window] = updateCallback;
		}

		if (resetCallback)
		{
			if (ResetCallbacks_.find(window) != ResetCallbacks_.end())
			{
				throw std::invalid_argument("Reset client is already registered with the debugger");
			}

			ResetCallbacks_[window] = resetCallback;
		}
	}
	
	
	void Debugger::RemoveClient(HWND window)
	{
		SectionLocker lock(Section_);

		bool removed(false);
		if (UpdateCallbacks_.find(window) != UpdateCallbacks_.end())
		{
			UpdateCallbacks_.erase(window);
			removed = true;
		}

		if (ResetCallbacks_.find(window) != ResetCallbacks_.end())
		{
			ResetCallbacks_.erase(window);
			removed = true;
		}

		if (!removed)
		{
			throw std::invalid_argument("window is not registered with the debugger");
		}
	}


	void Debugger::Halt()
	{
		SectionLocker lock(Section_);

		ExecutionMode_ = ExecutionMode::Halt;
	}


	bool Debugger::IsHalted() const
	{
		SectionLocker lock(Section_);

		return ExecutionMode_ == ExecutionMode::Halt;
	}


	bool Debugger::IsHalted(tl::optional<unsigned short>& returnPc) const
	{
		SectionLocker lock(Section_);

		if (ExecutionMode_ != ExecutionMode::Halt)
		{
			returnPc.reset();
			return false;
		}

		returnPc = ProcessorState_.PC;

		return true;
	}


	bool Debugger::IsStepping() const
	{
		SectionLocker lock(Section_);

		return ExecutionMode_ == ExecutionMode::Step;
	}




	CPUState Debugger::GetProcessorStateCopy() const
	{
		SectionLocker lock(Section_);

		return ProcessorState_;
	}


	bool Debugger::HasPendingCommand() const
	{
		SectionLocker lock(Section_);

		return PendingCommand.has_value();
	}


	Debugger::ExecutionMode Debugger::ConsumePendingCommand()
	{
		SectionLocker lock(Section_);

		auto value = PendingCommand.value();
		PendingCommand.reset();

		return value;
	}


	void Debugger::SetBreakpoints(breakpointsbuffer_type breakpoints)
	{
		SectionLocker lock(Section_);

		Breakpoints_ = move(breakpoints);
		BreakpointsChanged_ = true;
	}




	void Debugger::QueueRun()
	{
		SectionLocker lock(Section_);

		PendingCommand = ExecutionMode::Run;
	}

	void Debugger::QueueStep()
	{
		SectionLocker lock(Section_);

		PendingCommand = ExecutionMode::Step;
	}

	void Debugger::QueueHalt()
	{
		SectionLocker lock(Section_);

		PendingCommand = ExecutionMode::Halt;
	}




	void Debugger::Update()
	{
		SectionLocker lock(Section_);

		ProcessorState_ = CPUGetState();

		if (BreakpointsChanged_)
		{
			CPUSetBreakpoints(Breakpoints_);
			BreakpointsChanged_ = false;
		}

		if (HasPendingCommand())
		{
			ExecutionMode_ = ProcessNewMode(ConsumePendingCommand());
		}

		if (!UpdateCallbacks_.empty())
		{
			for (const auto& callback : UpdateCallbacks_)
			{
				callback.second(callback.first);
			}
		}

	}


	Debugger::ExecutionMode Debugger::ProcessNewMode(ExecutionMode cpuCmd)
	{
		// Run -> Halt
		if (ExecutionMode_ == ExecutionMode::Run && cpuCmd == ExecutionMode::Halt)
		{
			return ExecutionMode::Halt;
		}
		// Halt -> Run
		if (ExecutionMode_ == ExecutionMode::Halt && cpuCmd == ExecutionMode::Run)
		{
			return ExecutionMode::Run;
		}
		// Halt -> Step
		if (ExecutionMode_ == ExecutionMode::Halt && cpuCmd == ExecutionMode::Step)
		{
			return ExecutionMode::Step;
		}

		return ExecutionMode_;
	}


} }


