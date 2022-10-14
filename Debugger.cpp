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
#include "MachineDefs.h"
#include <sstream>
#include <iomanip>
#include "tcc1014mmu.h"	//  FIXME: May want this decoupled from Debugger for Memory Write


namespace VCC { namespace Debugger
{

	void Debugger::Reset()
	{
		SectionLocker lock(Section_);

		ExecutionMode_ = ExecutionMode::Run;
		PendingCommand_ = ExecutionMode::Halt;
		HasPendingCommand_ = false;
		ProcessorState_ = CPUState();

		for (const auto& callback : RegisteredClients_)
		{
			callback.second->OnReset();
		}
	}



	void Debugger::RegisterClient(HWND window, std::unique_ptr<Client> client)
	{
		SectionLocker lock(Section_);

		if (!client)
		{
			throw std::invalid_argument("client cannot be null");
		}

		if (RegisteredClients_.find(window) != RegisteredClients_.end())
		{
			throw std::invalid_argument("Udpate client is already registered with the debugger");
		}

		RegisteredClients_[window] = move(client);
	}
	
	
	void Debugger::RemoveClient(HWND window)
	{
		SectionLocker lock(Section_);

		if (RegisteredClients_.find(window) == RegisteredClients_.end())
		{
			throw std::invalid_argument("window is not registered with the debugger");
		}

		RegisteredClients_.erase(window);
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


	bool Debugger::IsHalted(unsigned short& returnPc) const
	{
		SectionLocker lock(Section_);

		if (ExecutionMode_ != ExecutionMode::Halt)
		{
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


	bool Debugger::HasPendingCommandNoLock() const
	{
		return HasPendingCommand_;
	}


	Debugger::ExecutionMode Debugger::ConsumePendingCommandNoLock()
	{
		if (!HasPendingCommandNoLock())
		{
			throw std::runtime_error("No pending command to consume");
		}

		HasPendingCommand_ = false;
		return PendingCommand_;
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

		PendingCommand_ = ExecutionMode::Run;
		HasPendingCommand_ = true;
	}

	void Debugger::QueueStep()
	{
		SectionLocker lock(Section_);

		PendingCommand_ = ExecutionMode::Step;
		HasPendingCommand_ = true;
	}

	void Debugger::QueueHalt()
	{
		SectionLocker lock(Section_);

		PendingCommand_ = ExecutionMode::Halt;
		HasPendingCommand_ = true;
	}




	void Debugger::Update()
	{
		{
			SectionLocker lock(Section_);

			ProcessorState_ = CPUGetState();

			if (BreakpointsChanged_)
			{
				CPUSetBreakpoints(Breakpoints_);
				BreakpointsChanged_ = false;
			}

			if (HasPendingCommandNoLock())
			{
				ExecutionMode_ = ProcessNewModeNoLock(ConsumePendingCommandNoLock());
			}
			
			if (HasPendingWriteNoLock())
			{
				ProcessWrite(ConsumePendingWriteNoLock());
			}

		}

		for (const auto& callback : RegisteredClients_)
		{
			callback.second->OnUpdate();
		}

	}


	Debugger::ExecutionMode Debugger::ProcessNewModeNoLock(ExecutionMode cpuCmd) const
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


	void Debugger::QueueWrite(unsigned short addr, unsigned char value)
	{
		SectionLocker lock(Section_);

		PendingWrite_ = PendingWrite(addr, value);
		HasPendingWrite_ = true;
	}


	bool Debugger::HasPendingWriteNoLock() const
	{
		return HasPendingWrite_;
	}

	Debugger::PendingWrite Debugger::ConsumePendingWriteNoLock()
	{
		if (!HasPendingWriteNoLock())
		{
			throw std::runtime_error("No pending memory writes to consume");
		}

		HasPendingWrite_ = false;
		return PendingWrite_;
	}

	void Debugger::ProcessWrite(PendingWrite memWrite)
	{
		MemWrite8(memWrite.value, memWrite.addr);
	}


} }


