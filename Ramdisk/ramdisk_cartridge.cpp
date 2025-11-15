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
#include "ramdisk_cartridge.h"
#include "resource.h"
#include "vcc/utils/winapi.h"


extern HINSTANCE gModuleInstance;


ramdisk_cartridge::name_type ramdisk_cartridge::name() const
{
	return ::vcc::utils::load_string(gModuleInstance, IDS_MODULE_NAME);
}

ramdisk_cartridge::catalog_id_type ramdisk_cartridge::catalog_id() const
{
	return ::vcc::utils::load_string(gModuleInstance, IDS_CATNUMBER);
}

ramdisk_cartridge::description_type ramdisk_cartridge::description() const
{
	return ::vcc::utils::load_string(gModuleInstance, IDS_DESCRIPTION);
}


void ramdisk_cartridge::start()
{
	initialize_state(true);
}

void ramdisk_cartridge::reset()
{
	initialize_state(false);
}

void ramdisk_cartridge::write_port(unsigned char port_id, unsigned char value)
{
	switch (port_id)
	{
	case 0x40:
		address_byte0 = value;
		break;

	case 0x41:
		address_byte1 = value;
		break;

	case 0x42:
		address_byte2 = (value & 0x7);
		break;

	case 0x43:
		ram_[current_address_] = value;
		return;

	default:
		return;
	}

	current_address_ = (address_byte2 << 16) | (address_byte1 << 8) | address_byte0;
}

unsigned char ramdisk_cartridge::read_port(unsigned char port_id)
{
	if (port_id == 0x43)
	{
		return ram_[current_address_];
	}

	return 0;
}


void ramdisk_cartridge::initialize_state(bool initialize_memory)
{
	current_address_ = 0;
	address_byte0 = 0;
	address_byte1 = 0;
	address_byte2 = 0;
	if (initialize_memory)
	{
		std::fill(ram_.begin(), ram_.end(), 0xff);
	}
}
