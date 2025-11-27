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
#include "gmc_cartridge_driver.h"
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_port_host.h"
#include "vcc/bus/expansion_port_ui.h"
#include "vcc/bus/expansion_port_bus.h"
#include <Windows.h>
#include <memory>


namespace vcc::cartridges::gmc
{

	/// @brief Game Master Cartridge plugin.
	///
	/// This plugin provides an implementation of the Game Master Cartridge, access to the
	/// cartridge device driver, and user interface functionality for access to settings and
	/// other plugin control features.
	class gmc_cartridge : public ::vcc::bus::cartridge
	{
	public:

		/// @brief Type alias for the component acting as the expansion bus the cartridge plugin
		/// is connected to.
		using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
		/// @brief Type alias for the component providing global system services to the
		/// cartridge plugin.
		using expansion_port_host_type = ::vcc::bus::expansion_port_host;
		/// @brief Type alias for the component emulating the Game Master Cartridge hardware.
		using driver_type = ::vcc::cartridges::gmc::gmc_cartridge_driver;


	public:

		/// @brief Construct the cartridge.
		/// 
		/// @param host A pointer to the host interface.
		/// @param bus A pointer to the bus interface.
		/// @param module_instance A handle to the instance of the module containing the
		/// GMC resources.
		gmc_cartridge(
			std::shared_ptr<expansion_port_host_type> host,
			std::unique_ptr<expansion_port_bus_type> bus,
			HINSTANCE module_instance);

		/// @inheritdoc
		[[nodiscard]] name_type name() const override;

		/// @inheritdoc
		[[nodiscard]] driver_type& driver() override;

		/// @inheritdoc
		void start() override;

		/// @inheritdoc
		[[nodiscard]] menu_item_collection_type get_menu_items() const override;

		/// @inheritdoc
		void menu_item_clicked(unsigned char menu_item_id) override;


	private:

		/// @brief Defines the identifiers for the options available to the user in the
		/// emulator's cartridge menu.
		struct menu_item_ids
		{
			//FIXME-CHET: The first id should be 0 but its not handled properly by the menu system.

			/// @brief Requests the Game Master Cartridge select a new ROM image to use.
			static const unsigned int select_rom = 1;
			/// @brief Requests the Game Master Cartridge remove the current ROM image.
			static const unsigned int remove_rom = 2;
		};

		/// @brief Defines the configuration section, value keys, and default values for the
		/// cartridge configuration.
		struct configuration
		{
			/// @brief The section where all settings for the GMC Cartridge are stored.
			static const inline std::string section = "Cartridges.GameMasterCartridge";

			/// @brief Defines the key names the settings values are stored in.
			struct keys
			{
				/// @brief The key used to store the path of the ROM image to load and use.
				static const inline std::string rom_filename = "rom.path";
			};
		};


	private:

		/// @brief The expansion port host.
		const std::shared_ptr<expansion_port_host_type> host_;
		/// @brief The handle to the module instance containing the cartridges resources.
		const HINSTANCE module_instance_;
		/// @brief The driver emulating the Game Master Cartridge hardware.
		driver_type driver_;
	};

}
