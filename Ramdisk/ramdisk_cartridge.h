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
#pragma once
#include "ramdisk_cartridge_driver.h"
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_port_bus.h"
#include <Windows.h>


namespace vcc::cartridges::rambuffer
{

	/// @brief RAM Disk/Buffer Cartridge
	/// 
	/// This plugin provides an implementation of a cartridge containing a large amount
	/// of RAM suitable for use as a RAM or print spooler. 
	///
	/// @todo Determine original of hardware and if it was a commercial product, from an
	/// article in The Rainbow, or a _fantasy_ device.
	/// @todo Add _battery backup_ feature in the same vein as Vidicom's SolidDrive and have
	/// the data stored in the RAM buffers persisted to a user selectable file.
	/// @todo Consider renaming since it's just a buffer and provides no actual disk
	/// like functionality.
	class ramdisk_cartridge : public ::vcc::bus::cartridge
	{
	public:

		/// @brief Type alias for the cartridge driver used by the plugin.
		using driver_type = ::vcc::cartridges::rambuffer::ramdisk_cartridge_driver;


	public:

		/// @brief Construct the cartridge.
		/// 
		/// @param module_instance A handle to the instance of the module containing the
		/// plugins resources.
		/// 
		/// @throws std::invalid_argument if `module_instance` is null.
		explicit ramdisk_cartridge(HINSTANCE module_instance);

		/// @inheritdoc
		name_type name() const override;

		/// @inheritdoc
		[[nodiscard]] driver_type& driver() override;

		/// @brief Initializes the device logic and memory to default values.
		void start() override;


	private:

		/// @brief The handle to the module instance containing the cartridges resources.
		const HINSTANCE module_instance_;
		/// @brief The driver emulating the RAM buffer hardware.
		driver_type driver_;
	};

}
