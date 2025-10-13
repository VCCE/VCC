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
#include <vcc/core/cartridges/null_cartridge.h>


namespace vcc { namespace modules { namespace mpi
{

	cartridge_slot::cartridge_slot()
		: cartridge_(std::make_unique<::vcc::core::cartridges::null_cartridge>())
	{}

	cartridge_slot::cartridge_slot(path_type path, handle_type handle, cartridge_ptr_type cartridge)
		:
		path_(move(path)),
		handle_(move(handle)),
		cartridge_(cartridge)
	{}


	cartridge_slot::label_type cartridge_slot::label() const
	{
		std::string text;

		text += cartridge_->name();
		text += "  ";
		text += cartridge_->catalog_id();

		return text;
	}

} } }
