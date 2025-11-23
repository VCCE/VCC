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
#include "ramdisk_device.h"


void ramdisk_device::start()
{
	initialize_device_state();
	std::fill(ram_.begin(), ram_.end(), buffer_type::value_type(0xffu));
}

void ramdisk_device::reset()
{
	initialize_device_state();
}

void ramdisk_device::write_port(unsigned char port_id, unsigned char value)
{
	switch (port_id)
	{
	case mmio_ports::address_low:
		address_byte0 = value;
		break;

	case mmio_ports::address_middle:
		address_byte1 = value;
		break;

	case mmio_ports::address_high:
		address_byte2 = (value & 0x7);
		break;

	case mmio_ports::data:
		ram_[current_address_] = value;
		return;

	default:
		return;
	}

	current_address_ = (address_byte2 << 16) | (address_byte1 << 8) | address_byte0;
}

unsigned char ramdisk_device::read_port(unsigned char port_id)
{
	if (port_id == mmio_ports::data)
	{
		return ram_[current_address_];
	}

	return 0;
}


void ramdisk_device::initialize_device_state()
{
	current_address_ = 0;
	address_byte0 = 0;
	address_byte1 = 0;
	address_byte2 = 0;
}
