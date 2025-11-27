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
#include "orchestra90cc_cartridge_driver.h"
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_port_bus.h"
#include "vcc/bus/expansion_port_host.h"
#include <memory>
#include <Windows.h>


namespace vcc::cartridges::orchestra90cc
{

	/// @brief Orchestra-90cc Stereo Pak plugin.
	///
	/// This plugin provides an implementation of the Tandy/Radio Shack Orchestra-90cc Stereo
	/// Pal and access to the cartridge device driver.
	class orchestra90cc_cartridge : public ::vcc::bus::cartridge
	{
	public:

		/// @brief Type alias for the component acting as the expansion bus the cartridge plugin
		/// is connected to.
		using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
		/// @brief Type alias for the component providing global system services to the
		/// cartridge plugin.
		using expansion_port_host_type = ::vcc::bus::expansion_port_host;
		/// @brief Type alias for the component emulating the ORchestra-90cc hardware.
		using driver_type = orchestra90cc_cartridge_driver;


	public:

		/// @brief Construct the cartridge.
		/// 
		/// @param host A pointer to the host interface.
		/// @param bus A pointer to the bus interface.
		/// @param module_instance A handle to the instance of the module containing the
		/// cartridge resources.
		/// 
		/// @throws std::invalid_argument if `host` is null.
		/// @throws std::invalid_argument if `module_instance` is null.
		orchestra90cc_cartridge(
			std::shared_ptr<expansion_port_host_type> host,
			std::unique_ptr<expansion_port_bus_type> bus,
			HINSTANCE module_instance);

		/// @inheritdoc
		name_type name() const override;

		/// @inheritdoc
		[[nodiscard]] driver_type& driver() override;

		/// @brief Initializes the driver to default values and starts its execution.
		void start() override;


	private:

		/// @brief The filename of the ROM to load. This ROM file is expected to be available
		/// in the emulators system ROMs collection.
		static const inline std::string rom_filename_ = "orch90.rom";

		/// @brief The expansion port host.
		const std::shared_ptr<expansion_port_host_type> host_;
		/// @brief The handle to the module instance containing the cartridges resources.
		const HINSTANCE module_instance_;
		/// @brief The driver emulating the Orchestra hardware.
		driver_type driver_;
	};

}
