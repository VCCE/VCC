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
	std::shared_ptr<expansion_port_bus_type> bus,
	HINSTANCE module_instance)
	:
	host_(move(host)),
	bus_(bus),
	module_instance_(module_instance),
	driver_(bus)
{}


orchestra90cc_cartridge::name_type orchestra90cc_cartridge::name() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME);
}

orchestra90cc_cartridge::driver_type& orchestra90cc_cartridge::driver()
{
	return driver_;
}


void orchestra90cc_cartridge::start()
{
	driver_.start(host_->system_rom_path() + default_rom_filename_);
}
