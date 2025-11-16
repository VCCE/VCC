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
#include "vcc/bus/expansion_port_host.h"


class multipak_expansion_slot_host : public ::vcc::bus::expansion_port_host
{
public:

	multipak_expansion_slot_host(
		size_t slot_id,
		::vcc::bus::expansion_port_host& host,
		multipak_cartridge& multipak)
		:
		slot_id_(slot_id),
		host_(host),
		multipak_(multipak)
	{}

	path_type configuration_path() const override
	{
		return host_.configuration_path();
	}

	path_type system_rom_path() const override
	{
		return host_.system_rom_path();
	}


private:

	const size_t slot_id_;
	::vcc::bus::expansion_port_host& host_;
	multipak_cartridge& multipak_;
};
