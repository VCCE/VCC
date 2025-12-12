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
#include "vcc/ui/menu/menu_item_collection.h"
#include "vcc/detail/exports.h"
#include <string>


namespace vcc::bus
{

	/// @brief Represents an interchangeable cartridge device.
	///
	/// This abstract class defines the fundamental operations of a cartridge device that
	/// can be connected to expansion ports such as a computer or video game console to
	/// provide programs and data on ROM, extra RAM, or additional hardware capabilities
	/// such as a floppy disk controller or programmable sound generator.
	class LIBCOMMON_EXPORT cartridge
	{
	public:

		/// @brief Specifies the type used to store a size or length.
		using size_type = std::size_t;
		/// @brief Specifies the type used to store names.
		using name_type = std::string;
		/// @brief Type alias for variable length status strings.
		using status_type = std::string;
		/// @brief Specifies the type used to store a collection of menu items.
		using menu_item_collection_type = ::vcc::ui::menu::menu_item_collection;
		/// @brief Specifies the type of devices supported by this cartridge.
		using driver_type = ::vcc::bus::cartridge_driver;
		/// @brief Type alias for menu item identifiers.
		using menu_item_id_type = UINT;


	public:

		/// @brief Construct the cartridge to a default state.
		cartridge() = default;
		cartridge(const cartridge&) = delete;
		cartridge(cartridge&&) = delete;

		/// @brief Release all resources held by the cartridge.
		virtual ~cartridge() = default;

		/// @brief Retrieves the name of the cartridge.
		/// 
		/// Retrieves the name of the cartridge.
		/// 
		/// @return A string containing the cartridge name.
		virtual [[nodiscard]] name_type name() const = 0;

		/// @brief Retrieves a reference to the device this cartridge controls.
		/// 
		/// Each cartridge controls an emulated hardware device that is accessed directly via
		/// the expansion port. The reference returned by this function can be used to access
		/// the functionality of the device through that interface
		/// 
		/// @return A reference to a cartridge device.
		virtual [[nodiscard]] driver_type& driver() = 0;

		/// @brief Initialize the plugin.
		///
		/// Initialize the cartridge device to a default state. If this called more than
		/// once without stopping the device the behavior is undefined.
		virtual void start();

		/// @brief Terminate the plugin.
		///
		/// Stops all device operations and releases all resources. This function must be
		/// called prior to the cartridge being destroyed to ensure that all resources are
		/// gracefully released. If this function is called before the cartridge is
		/// initialized or after it has been terminated the behavior is undefined.
		virtual void stop();

		/// @brief Retrieve the cartridge status.
		/// 
		/// Retrieves the status of the cartridge as a human readable string.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated the behavior is undefined.
		virtual status_type status() const;

		/// @brief Inform the cartridge a menu item has been clicked.
		/// 
		/// Inform the cartridge a menu item associated with it has been clicked.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated the behavior is undefined.
		/// 
		/// @todo rename this to `execute_menu_command` or something.
		/// 
		/// @param menu_item_id The identifier of the menu item.
		virtual void menu_item_clicked(menu_item_id_type menu_item_id);

		/// @brief Get the list of menu items for this cartridge.
		/// 
		/// Retrieves the list of menu and submenu items that can be used to control the
		/// cartridge by invoking the `menu_item_clicked` member.
		/// 
		/// @return A collection of menu items for this cartridge.
		virtual menu_item_collection_type get_menu_items() const;
	};

}
