////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute itand/or
//	modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your
//	option) any later version.
//	
//	VCC (Virtual Color Computer) is distributed in the hope that it will be
//	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
//	Public License for more details.
//	
//	You should have received a copy of the GNU General Public License along with
//	VCC (Virtual Color Computer). If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "multipak_cartridge.h"

class cartridge_slot_adapter : public ::VCC::Core::cartridge_callbacks
{
public:

	cartridge_slot_adapter
		(
			size_t slot_id, 
			::VCC::Core::cartridge_callbacks& mpi_callbacks,
			multipak_cartridge& multipak
		)
		:
		slot_id_(slot_id),
		mpi_callbacks_(mpi_callbacks),
		multipak_(multipak)
	{}

	path_type configuration_path() const override
	{
		return mpi_callbacks_.configuration_path();
	}

	void write_memory_byte(unsigned char value, unsigned short address) override
	{
		mpi_callbacks_.write_memory_byte(value, address);
	}

	unsigned char read_memory_byte(unsigned short address) override
	{
		return mpi_callbacks_.read_memory_byte(address);
	}

	void assert_cartridge_line(bool line_state) override
	{
		multipak_.assert_cartridge_line(slot_id_, line_state);
	}

	void assert_interrupt(Interrupt interrupt, InterruptSource interrupt_source) override
	{
		mpi_callbacks_.assert_interrupt(interrupt, interrupt_source);
	}


private:

	const size_t slot_id_;
	::VCC::Core::cartridge_callbacks& mpi_callbacks_;
	multipak_cartridge& multipak_;
};
