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
#include "ramdisk_cartridge_driver.h"
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_port_bus.h"
#include <Windows.h>


class ramdisk_cartridge : public ::vcc::bus::cartridge
{
public:

	using driver_type = ramdisk_cartridge_driver;


public:

	explicit ramdisk_cartridge(HINSTANCE module_instance);

	name_type name() const override;
	[[nodiscard]] driver_type& driver() override;

	void start() override;


private:

	const HINSTANCE module_instance_;
	driver_type driver_;
};
