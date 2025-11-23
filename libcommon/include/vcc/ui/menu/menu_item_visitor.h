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

	class LIBCOMMON_EXPORT menu_item_visitor
	{
	public:

		using item_id_type = ::vcc::ui::menu::detail::menu_item::item_id_type;
		using string_type = ::vcc::ui::menu::detail::menu_item::string_type;
		using icon_type = HBITMAP;


	public:

		virtual ~menu_item_visitor() = default;

		virtual void root_submenu(const string_type& text, icon_type icon) = 0;
		virtual void root_separator() = 0;
		virtual void root_item(item_id_type id, const string_type& text, icon_type icon, bool disabled) = 0;
		virtual void submenu_separator() = 0;
		virtual void submenu_item(item_id_type id, const string_type& text, icon_type icon, bool disabled) = 0;
	};

}