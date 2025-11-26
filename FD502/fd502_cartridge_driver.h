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
#include "vcc/bus/cartridge_driver.h"
#include "vcc/bus/expansion_port_bus.h"
#include "vcc/bus/expansion_port_ui.h"
#include "vcc/bus/expansion_port_host.h"
#include "vcc/devices/rom/banked_rom_image.h"
#include <memory>


class fd502_cartridge_driver : public ::vcc::bus::cartridge_driver
{
public:

	using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
	using expansion_port_ui_type = ::vcc::bus::expansion_port_ui;
	using expansion_port_host_type = ::vcc::bus::expansion_port_host;
	using path_type = std::string;
	using rom_image_type = ::vcc::devices::rom::banked_rom_image;


public:

	fd502_cartridge_driver(
		std::shared_ptr<expansion_port_host_type> host,
		std::shared_ptr<expansion_port_bus_type> bus);

	void start() /*override*/;
	void stop() /*override*/;

	unsigned char read_memory_byte(size_type memory_address) override;

	void write_port(unsigned char port_id, unsigned char value) override;
	unsigned char read_port(unsigned char port_id) override;
	void update(float delta) override;

private:


	const std::shared_ptr<expansion_port_host_type> host_;
	const std::shared_ptr<expansion_port_bus_type> bus_;
};


