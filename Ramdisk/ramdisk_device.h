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
#include "vcc/bus/cartridge_device.h"
#include "vcc/bus/expansion_port_bus.h"
#include <array>


class ramdisk_device : public ::vcc::bus::cartridge_device
{
public:

	using address_type = std::size_t;
	using buffer_type = std::array<unsigned char, 1024u * 512u>;


public:

	ramdisk_device() = default;

	void start();
	void reset() override;

	void write_port(unsigned char port_id, unsigned char value) override;
	unsigned char read_port(unsigned char port_id) override;


private:

	void initialize_device_state();


private:

	struct mmio_ports
	{
		static const unsigned char address_low = 0x40;
		static const unsigned char address_middle = 0x41;
		static const unsigned char address_high = 0x42;
		static const unsigned char data = 0x43;
	};

	address_type current_address_ = 0;
	address_type address_byte0 = 0;
	address_type address_byte1 = 0;
	address_type address_byte2 = 0;
	buffer_type ram_ = {};
};
