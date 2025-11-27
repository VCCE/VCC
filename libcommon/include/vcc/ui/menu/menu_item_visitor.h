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
#include "vcc/ui/menu/detail/menu_item.h"
#include "vcc/detail/exports.h"
#include <Windows.h>


namespace vcc::ui::menu
{

	/// @brief Defines the interface of a menu item visitor.
	class LIBCOMMON_EXPORT menu_item_visitor
	{
	public:

		/// @copydoc ::vcc::ui::menu::detail::menu_item::item_id_type
		using item_id_type = ::vcc::ui::menu::detail::menu_item::item_id_type;
		/// @copydoc ::vcc::ui::menu::detail::menu_item::string_type
		using string_type = ::vcc::ui::menu::detail::menu_item::string_type;
		/// @copydoc ::vcc::ui::menu::detail::menu_item::icon_type
		using icon_type = ::vcc::ui::menu::detail::menu_item::icon_type;


	public:

		virtual ~menu_item_visitor() = default;

		/// @brief Visit a root menu sub-menu.
		/// 
		/// @param text The sub-menu text.
		/// @param icon An option icon to display next to the menu item.
		virtual void root_submenu(const string_type& text, icon_type icon) = 0;

		/// @brief Visit a separator item in the root menu.
		virtual void root_separator() = 0;

		/// @brief Visit a selectable menu item in the root menu.
		/// 
		/// @param id The identifier to send when the menu item is clicked.
		/// @param text The menu item text.
		/// @param icon An optional item to display next to the menu item.
		/// @param disabled Flag indicating if the menu item is disabled.
		virtual void root_item(
			item_id_type id,
			const string_type& text,
			icon_type icon,
			bool disabled) = 0;

		/// @brief Visit a separator item in the current sub-menu.
		virtual void submenu_separator() = 0;

		/// @brief Visit a selectable menu item in the current sub-menu.
		/// 
		/// @param id The identifier to send when the menu item is clicked.
		/// @param text The menu item text.
		/// @param icon An optional item to display next to the menu item.
		/// @param disabled Flag indicating if the menu item is disabled.
		virtual void submenu_item(
			item_id_type id,
			const string_type& text,
			icon_type icon,
			bool disabled) = 0;
	};

}