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
#include "vcc/detail/exports.h"


// FIXME: this needs to come from the common library but is currently part of the
// main vcc app. Update this when it is migrated.
enum MenuItemType;

namespace vcc::bus
{

	class LIBCOMMON_EXPORT expansion_port_ui
	{
	public:

		/// @brief The type used to represent menu items.
		using menu_item_type = MenuItemType;


	public:

		virtual ~expansion_port_ui() = default;

		/// @brief Adds an item to the cartridges UI menu.
		/// 
		/// @param text The text to display.
		/// @param menu_id The identifier sent to the cartridge instance when the item is selected.
		/// @param menu_type The type of menu item to add.
		virtual void add_menu_item(const char* text, int menu_id, menu_item_type menu_type) = 0;
	};

}
