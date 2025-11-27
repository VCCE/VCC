////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute it and/or
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
#include "ramdisk_cartridge_driver.h"
#include <algorithm>


namespace vcc::cartridges::rambuffer
{

	void ramdisk_cartridge_driver::start()
	{
		initialize_device_state(true);
	}

	void ramdisk_cartridge_driver::reset()
	{
		initialize_device_state(false);
	}

	void ramdisk_cartridge_driver::write_port(unsigned char port_id, unsigned char value)
	{
		switch (port_id)
		{
		case mmio_ports::address_low:
			address_byte_low_ = value;
			break;

		case mmio_ports::address_middle:
			address_byte_middle_ = value;
			break;

		case mmio_ports::address_high:
			address_byte_high_ = (value & 0x7);
			break;

		case mmio_ports::data:
			ram_[current_address_] = value;
			return;

		default:
			return;
		}

		current_address_ = (address_byte_high_ << 16) | (address_byte_middle_ << 8) | address_byte_low_;
	}

	unsigned char ramdisk_cartridge_driver::read_port(unsigned char port_id)
	{
		if (port_id == mmio_ports::data)
		{
			return ram_[current_address_];
		}

		return 0;
	}


	void ramdisk_cartridge_driver::initialize_device_state(bool initialize_memory)
	{
		current_address_ = 0;
		address_byte_low_ = 0;
		address_byte_middle_ = 0;
		address_byte_high_ = 0;

		if (initialize_memory)
		{
			std::ranges::fill(ram_, buffer_type::value_type(0xffu));
		}
	}

}
