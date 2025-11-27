////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute it and/or
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
#include <stdexcept>


namespace vcc::cartridges::rambuffer
{

	ramdisk_cartridge::ramdisk_cartridge(HINSTANCE module_instance)
		: module_instance_(module_instance)
	{
		if (module_instance_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct RAM Buffer Cartridge. The module handle is null.");
		}
	}


	ramdisk_cartridge::name_type ramdisk_cartridge::name() const
	{
		return ::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME);
	}

	ramdisk_cartridge::driver_type& ramdisk_cartridge::driver()
	{
		return driver_;
	}

	void ramdisk_cartridge::start()
	{
		driver_.start();
	}

}
