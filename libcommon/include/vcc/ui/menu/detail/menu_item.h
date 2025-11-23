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

	struct menu_item
	{
		using category_type = ::vcc::ui::menu::detail::menu_category;
		using item_id_type = unsigned int;
		using string_type = ::std::string;
		using icon_type = HBITMAP;

		string_type name;
		item_id_type menu_id = 0;
		category_type type = category_type::root_seperator;
		icon_type icon = nullptr;
		bool disabled = false;
	};

}
