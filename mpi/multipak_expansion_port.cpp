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
#include "multipak_expansion_port.h"
#include "vcc/bus/cartridges/empty_cartridge.h"


multipak_expansion_port& multipak_expansion_port::operator=(multipak_expansion_port&& other) noexcept
{
	if (this != &other)
	{
		expansion_port::operator=(std::move(other));

		line_state_ = other.line_state_;

		other.line_state_ = false;
	}

	return *this;
}


multipak_expansion_port::label_type multipak_expansion_port::label() const
{
	return name() + "  " + catalog_id();
}
