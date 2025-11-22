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
#include "orchestra90cc_cartridge.h"
#include "resource.h"
#include "vcc/utils/winapi.h"


orchestra90cc_cartridge::orchestra90cc_cartridge(
	std::shared_ptr<expansion_port_host_type> host,
	std::unique_ptr<expansion_port_bus_type> bus,
	HINSTANCE module_instance)
	:
	host_(move(host)),
	bus_(move(bus)),
	module_instance_(module_instance)
{
}


orchestra90cc_cartridge::name_type orchestra90cc_cartridge::name() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME);
}

orchestra90cc_cartridge::catalog_id_type orchestra90cc_cartridge::catalog_id() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_CATNUMBER);
}

orchestra90cc_cartridge::description_type orchestra90cc_cartridge::description() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_DESCRIPTION);
}

orchestra90cc_cartridge::device_type& orchestra90cc_cartridge::device()
{
	return *this;
}


void orchestra90cc_cartridge::start()
{
	if (rom_image_.load(host_->system_rom_path() + default_rom_filename_))
	{
		bus_->set_cartridge_select_line(true);
	}
}


void orchestra90cc_cartridge::write_port(unsigned char port_id, unsigned char value)
{
	switch (port_id)
	{
	case mmio_ports::right_channel:
		right_channel_buffer_ = value;
		break;

	case mmio_ports::left_channel:
		left_channel_buffer_ = value;
		break;
	}
}

unsigned char orchestra90cc_cartridge::read_memory_byte(size_type memory_address)
{
	return rom_image_.read_memory_byte(memory_address);
}


unsigned short orchestra90cc_cartridge::sample_audio()
{
	return (left_channel_buffer_ << 8) | right_channel_buffer_;
}
