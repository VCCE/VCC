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
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_port_bus.h"
#include <Windows.h>
#include <memory>
#include <array>
#include "ramdisk_device.h"


class ramdisk_cartridge : public ::vcc::bus::cartridge
{
public:

	using device_type = ramdisk_device;


public:

	explicit ramdisk_cartridge(HINSTANCE module_instance);

	name_type name() const override;
	catalog_id_type catalog_id() const override;
	description_type description() const override;
	[[nodiscard]] device_type& device() override;

	void start() override;


private:

	const HINSTANCE module_instance_;
	device_type device_;
};
