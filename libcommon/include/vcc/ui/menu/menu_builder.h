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
#include "vcc/ui/menu/menu_item_collection.h"
#include "vcc/ui/menu/detail/menu_item.h"
#include "vcc/detail/exports.h"
#include <Windows.h>
#include <vector>
#include <functional>


namespace vcc::ui::menu
{

	/// @brief A lightweight utility for building a list of menu items.
	///
	/// This class aids in the building of menus used in the _cartridge_ menu of the
	/// emulator. The items are built sequentially and when the building is done the
	/// sequence can be retrieved and used for additional processing of menu items.
	class menu_builder
	{
	public:

		/// @brief Type alias for a menu item.
		using item_type = ::vcc::ui::menu::detail::menu_item;
		/// @copydoc item_type::category_type
		using category_type = item_type::category_type;
		/// @copydoc item_type::item_id_type
		using item_id_type = item_type::item_id_type;
		/// @copydoc item_type::string_type
		using string_type = item_type::string_type;
		/// @brief Type alias for a collection of menu items.
		using item_collection_type = menu_item_collection;
		/// @brief TYpe alias for a container of items used during the builder process.
		using item_container_type = menu_item_collection::item_container_type;
		/// @copydoc item_type::icon_type
		using icon_type = HBITMAP;


	public:

		/// @brief Indicates if the menu builder is empty.
		/// @return `true` if the builder is empty and contains no menu items; `false` otherwise.
		LIBCOMMON_EXPORT bool empty() const;
		
		/// @brief Removes all menu items from the builder.
		/// 
		/// @return A reference to the builder.
		LIBCOMMON_EXPORT menu_builder& clear();

		/// @brief Add a sub-menu to the end of the root menu.
		/// 
		/// This adds a sub-menu to the root menu and makes it the active sub-menu
		/// that items will be added to when calling `add_submenu_separator` and
		/// `add_submenu_item`.
		/// 
		/// @param text The sub-menu text.
		/// @param icon An option icon to display next to the menu item.
		/// 
		/// @return A reference to the builder.
		LIBCOMMON_EXPORT menu_builder& add_root_submenu(string_type text, icon_type icon = nullptr);

		/// @brief Add a separator to the end of the root menu.
		/// 
		/// @return A reference to the builder.
		LIBCOMMON_EXPORT menu_builder& add_root_separator();

		/// @brief Add a selectable menu item to the end of the root menu.
		/// 
		/// @param id The identifier to send when the menu item is clicked.
		/// @param text The menu item text.
		/// @param icon An optional item to display next to the menu item.
		/// @param disabled Flag indicating if the menu item is disabled. If this flag is
		/// `true` the item should be not be selectable by the user and not dispatched.
		/// 
		/// @return A reference to the builder.
		LIBCOMMON_EXPORT menu_builder& add_root_item(
			item_id_type id,
			string_type text,
			icon_type icon = nullptr,
			bool disabled = false);

		/// @brief Add a separator to the end of the current sub-menu.
		/// 
		/// @return A reference to the builder.
		LIBCOMMON_EXPORT menu_builder& add_submenu_separator();

		/// @brief Add a selectable menu item to the end of the current sub-menu.
		/// 
		/// @param id The identifier to send when the menu item is clicked.
		/// @param text The menu item text.
		/// @param icon An optional item to display next to the menu item.
		/// @param disabled Flag indicating if the menu item is disabled. If this flag is
		/// `true` the item should be not be selectable by the user and not dispatched.
		/// 
		/// @return A reference to the builder.
		LIBCOMMON_EXPORT menu_builder& add_submenu_item(
			item_id_type id,
			string_type text,
			icon_type icon = nullptr,
			bool disabled = false);

		/// @brief Adds a collection of items.
		/// 
		/// Adds a collection of items to the end of the list of items already added to
		/// the builder. When the items are appended to the current list, their identifiers
		/// are adjusted by adding the value passed in `menu_id_offset` to them.
		/// 
		/// @param items The items to add. 
		/// @param menu_id_offset The value to adjust the menu id by when added to the builder.
		/// 
		/// @return A reference to the builder.
		LIBCOMMON_EXPORT menu_builder& add_items(
			const item_collection_type& items,
			size_t menu_id_offset = 0);

		/// @brief Remove all items stored in the builder and return them as in a
		/// collection.
		/// 
		/// @return A collection of all items added to the builder.
		LIBCOMMON_EXPORT item_collection_type release_items();


	protected:

		/// @brief Add a menu item to the end of the current list of items.
		/// 
		/// @param category The category of the menu item specifying its type and which
		/// menu it should be added to.
		/// @param id The identifier to send when the menu item is clicked.
		/// @param text The menu item text.
		/// @param icon An optional item to display next to the menu item.
		/// @param disabled Flag indicating if the menu item is disabled. If this flag is
		/// `true` the item should be not be selectable by the user and not dispatched.
		LIBCOMMON_EXPORT void add_item(
			category_type category,
			item_id_type id,
			string_type text,
			icon_type icon = nullptr,
			bool disabled = false);


	private:

		/// @brief The list of menu items.
		item_container_type items_;
	};

	/// @brief Convert an lvalue reference to a menu builder to an rvalue.
	/// 
	/// @param builder The builder to convert.
	/// 
	/// @return The reference passed in `builder` as an `rvalue`.
	inline menu_builder&& move(menu_builder& builder)
	{
		return static_cast<menu_builder&&>(builder);
	}

}
