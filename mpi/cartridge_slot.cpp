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
#include "cartridge_slot.h"
#include "vcc/bus/cartridges/empty_cartridge.h"


namespace vcc::modules::mpi
{

	cartridge_slot::cartridge_slot(path_type path, handle_type handle, cartridge_ptr_type cartridge)
		:
		expansion_port(move(handle), move(cartridge)),
		path_(move(path))
	{}

	cartridge_slot& cartridge_slot::operator=(cartridge_slot&& other) noexcept
	{
		if (this != &other)
		{
			expansion_port::operator=(std::move(other));

			path_ = move(other.path_);
			line_state_ = other.line_state_;

			other.line_state_ = false;
		}

		return *this;
	}


	cartridge_slot::label_type cartridge_slot::label() const
	{
		return name() + "  " + catalog_id();
	}

}
