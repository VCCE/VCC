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
#include "orchestra90cc_device.h"
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_port_bus.h"
#include "vcc/bus/expansion_port_host.h"
#include <memory>
#include <Windows.h>


class orchestra90cc_cartridge : public ::vcc::bus::cartridge
{
public:

	using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
	using expansion_port_host_type = ::vcc::bus::expansion_port_host;
	using device_type = orchestra90cc_device;
	using rom_image_type = ::vcc::devices::rom::rom_image;


public:

	orchestra90cc_cartridge(
		std::shared_ptr<expansion_port_host_type> host,
		std::shared_ptr<expansion_port_bus_type> bus,
		HINSTANCE module_instance);

	/// @inheritdoc
	name_type name() const override;

	[[nodiscard]] device_type& device() override;


	void start() override;


private:

	static const inline std::string default_rom_filename_ = "orch90.rom";

	const std::shared_ptr<expansion_port_host_type> host_;
	const std::shared_ptr<expansion_port_bus_type> bus_;
	const HINSTANCE module_instance_;
	device_type device_;
};
