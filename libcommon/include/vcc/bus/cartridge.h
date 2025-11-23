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
#include "vcc/bus/cartridge_device.h"
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

		/// @brief Specifies the type used to store a size of length.
		using size_type = std::size_t;
		/// @brief Specifies the type used to store names.
		using name_type = std::string;
		/// @brief Specifies the type used to store catalog identifiers.
		using catalog_id_type = std::string;
		/// @brief Specifies the type used to store cartridge descriptions.
		using description_type = std::string;
		/// @brief Specifies the type used to store a collection of menu items.
		using menu_item_collection_type = ::vcc::ui::menu::menu_item_collection;
		/// @bridge Specifies the type of devices supported by this cartridge.
		using device_type = ::vcc::bus::cartridge_device;


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
		/// This function may be invoked prior to calling `start` to initialize the device
		/// and after `stop` has been called to terminate the device.
		///
		/// @return A string containing the cartridge name.
		virtual [[nodiscard]] name_type name() const = 0;

		/// @brief Retrieves an optional catalog identifier of the cartridge.
		/// 
		/// Retrieves an optional catalog identifier of the cartridge. Catalog identifiers
		/// are arbitrary and generally represent the part number of the cartridge included
		/// in catalogs and magazine advertisements. Cartridges are not requires to provide
		/// a catalog identifier and may return an empty string.
		/// 
		/// This function may be invoked prior to calling `start` to initialize the device
		/// and after `stop` has been called to terminate the device.
		///
		/// @return A string containing the catalog identifier. If the cartridge does not
		/// include a catalog id an empty string is returned.
		virtual [[nodiscard]] catalog_id_type catalog_id() const = 0;

		/// @brief Retrieves an optional description of the cartridge.
		/// 
		/// This function may be invoked prior to calling `start` to initialize the device
		/// and after `stop` has been called to terminate the device.
		///
		/// @return A string containing the description of the cartridge. If the cartridge
		/// does not include a description an empty string is returned.
		virtual [[nodiscard]] description_type description() const = 0;

		virtual [[nodiscard]] device_type& device() = 0;

		/// @brief Initialize the device.
		///
		/// Initialize the cartridge device to a default state. If this called more than
		/// once without stopping the device an exception is thrown.
		/// 
		/// @todo Add exception information. Need custom exceptions first.
		virtual void start();

		/// @brief Terminate the device.
		///
		/// Stops all device operations and releases all resources. This function must be
		/// called prior to the cartridge being destroyed to ensure that all resources are
		/// gracefully released. If this function is called before the cartridge is
		/// initialized or after it has been terminated an exception is thrown.
		/// 
		/// @todo Add exception information. Need custom exceptions first.
		virtual void stop();

		/// @brief Retrieve the cartridge status.
		/// 
		/// Retrieves the status of the cartridge as a human readable string.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated an exception is thrown.
		/// 
		/// @todo Add exception information. Need custom exceptions first.
		/// @todo Maybe rename `buffer_size` to `buffer_length`.
		/// 
		/// @param text_buffer The text buffer to generate the status string in.
		/// @param buffer_size The length of the text buffer.
		virtual void status(char* text_buffer, size_type buffer_size);

		/// @brief Inform the cartridge a menu item has been clicked.
		/// 
		/// Inform the cartridge a menu item associated with it has been clicked.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated an exception is thrown.
		/// 
		/// @todo Add exception information. Need custom exceptions first.
		/// 
		/// @param menu_item_id The identifier of the menu item.
		virtual void menu_item_clicked(unsigned char menu_item_id);

		/// @brief Get the list of menu items for this cartridge.
		/// 
		/// Retrieves the list of menu and submenu items that can be used to control the
		/// cartridge by invoking the `menu_item_clicked` member.
		/// 
		/// @return A collection of menu items for this cartridge.
		virtual menu_item_collection_type get_menu_items() const;
	};

}
