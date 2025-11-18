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
#include "vcc/bus/expansion_port_host.h"
#include "vcc/bus/expansion_port_ui.h"
#include "vcc/bus/expansion_port_bus.h"
#include <memory>

namespace vcc::bus
{

	/// @brief The type returned by the cartridge factory function.
	using cartridge_factory_result = std::unique_ptr<::vcc::bus::cartridge>;

	/// @brief The function type used to create instances of a cartridge.
	///
	/// @param host The host system that will manage the cartridge.
	/// @param ui The user interface manager that allows interaction between the cartridge
	/// and the user.
	/// @param bus The expansion bus of the emulated system.
	/// 
	/// @return An instance of a cartridge.
	using cartridge_factory_prototype = cartridge_factory_result(*)(
		std::unique_ptr<::vcc::bus::expansion_port_host> host,
		std::unique_ptr<::vcc::bus::expansion_port_ui> ui,
		std::unique_ptr<::vcc::bus::expansion_port_bus> bus);

	/// @brief The function type used to retrieve the factory function of a cartridge.
	using create_cartridge_factory_prototype = cartridge_factory_prototype(*)();

}
