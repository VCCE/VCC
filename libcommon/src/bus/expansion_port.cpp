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
#include "vcc/bus/expansion_port.h"
#include "vcc/bus/cartridges/empty_cartridge.h"


namespace vcc::bus
{

	const expansion_port::cartridge_ptr_type
		expansion_port::default_empty_cartridge_(
			std::make_shared<::vcc::bus::cartridges::empty_cartridge>());


	expansion_port::expansion_port()
		:
		cartridge_(default_empty_cartridge_),
		device_(&cartridge_->device())
	{ }

	expansion_port::expansion_port(managed_handle_type handle, cartridge_ptr_type cartridge)
		:
		cartridge_(move(cartridge)),
		handle_(move(handle)),
		device_(&cartridge_->device())
	{ }

}
