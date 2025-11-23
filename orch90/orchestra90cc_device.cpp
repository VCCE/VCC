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
#include "orchestra90cc_device.h"
#include "resource.h"
#include "vcc/utils/winapi.h"


orchestra90cc_device::orchestra90cc_device(std::shared_ptr<expansion_port_bus_type> bus)
	: bus_(move(bus))
{}


void orchestra90cc_device::start(const path_type& rom_filename)
{
	if (!rom_filename.empty() && rom_image_.load(rom_filename))
	{
		bus_->set_cartridge_select_line(true);
	}
}


void orchestra90cc_device::write_port(unsigned char port_id, unsigned char value)
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

unsigned char orchestra90cc_device::read_memory_byte(size_type memory_address)
{
	return rom_image_.read_memory_byte(memory_address);
}


unsigned short orchestra90cc_device::sample_audio()
{
	return (left_channel_buffer_ << 8) | right_channel_buffer_;
}
