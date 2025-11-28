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
#include "multipak_expansion_port_bus.h"
#include <stdexcept>


namespace vcc::cartridges::multipak
{

	multipak_expansion_port_bus::multipak_expansion_port_bus(
		std::shared_ptr<::vcc::bus::expansion_port_bus> bus,
		slot_id_type slot_id,
		driver_type& driver)
		:
		bus_(bus),
		slot_id_(slot_id),
		driver_(driver)
	{
		if (bus_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Expansion Bus adapter. Parent bus is null.");
		}
	}

}
