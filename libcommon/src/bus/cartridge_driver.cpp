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
#include "vcc/bus/cartridge_driver.h"


namespace vcc::bus
{

	void cartridge_driver::reset()
	{}

	void cartridge_driver::update([[maybe_unused]] float delta)
	{}

	void cartridge_driver::write_port(
		[[maybe_unused]] unsigned char port_id,
		[[maybe_unused]] unsigned char value)
	{}

	unsigned char cartridge_driver::read_port([[maybe_unused]] unsigned char port_id)
	{ 
		return {};
	}

	unsigned char cartridge_driver::read_memory_byte([[maybe_unused]] size_type memory_address)
	{
		return {};
	}

	unsigned short cartridge_driver::sample_audio()
	{
		return {};
	}

}
