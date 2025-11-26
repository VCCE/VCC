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
#include "vcc/devices/psg/sn76496.h"
#include "vcc/devices/rom/banked_rom_image.h"
#include "vcc/bus/cartridge_driver.h"
#include "vcc/bus/expansion_port_bus.h"
#include <memory>


class gmc_cartridge_driver :  public ::vcc::bus::cartridge_driver
{
public:

	using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
	using path_type = std::string;
	using rom_image_type = ::vcc::devices::rom::banked_rom_image;
	using psg_device_type = ::vcc::devices::psg::sn76489_device;


public:

	explicit gmc_cartridge_driver(std::shared_ptr<expansion_port_bus_type> bus);

	void start(const path_type& rom_filename);
	void reset() override;

	bool has_rom() const noexcept;
	path_type rom_filename() const;

	void load_rom(const path_type& filename, bool reset_on_load);

	unsigned char read_memory_byte(size_type memory_address) override;

	void write_port(unsigned char port_id, unsigned char value) override;
	unsigned char read_port(unsigned char port_id) override;

	sample_type sample_audio() override;


private:

	struct mmio_ports
	{
		static const unsigned char select_rom_bank = 0x40;
		static const unsigned char psg_io = 0x41;
	};

	const std::shared_ptr<expansion_port_bus_type> bus_;
	rom_image_type rom_image_;
	psg_device_type psg_;
};


