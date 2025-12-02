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
#include "vcc/ui/menu/detail/menu_category.h"
#include <string>
#include <Windows.h>


namespace vcc::ui::menu::detail
{

	/// @brief Describes the common properties of a menu item.
	struct menu_item
	{
		/// @brief Type alias for a menu item category.
		using category_type = ::vcc::ui::menu::detail::menu_category;
		/// @brief Type alias for a menu item id.
		using item_id_type = unsigned int;
		/// @brief Type alias for a variable length string.
		using string_type = std::string;
		/// @brief Type alias for a system bitmap handle.
		using icon_type = HBITMAP;

		/// @brief The item text
		string_type text;
		/// @brief The menu id.
		item_id_type id = 0;
		/// @brief The item type.
		category_type type = category_type::root_seperator;
		/// @brief An optional icon to display next to the menu text.
		icon_type icon = nullptr;
		/// @brief Flag indicating if the item is disabled.
		bool disabled = false;
	};

}
