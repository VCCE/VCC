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

namespace vcc::ui::menu::detail
{

	/// @brief Represents the different types of menu items.
	enum class menu_category
	{
		/// @brief Menu header with no associated control, may have slave items
		root_sub_menu,
		/// @brief Draws a horizontal line to separate groups of menu items
		root_seperator,
		/// @brief Menu item with an associated control
		root_menu_item,

		sub_menu_separator,
		/// @brief Slave items with associated control in header sub-menu.
		sub_menu_item
	};

}
