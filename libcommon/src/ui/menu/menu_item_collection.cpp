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
#include "vcc/ui/menu/menu_item_collection.h"
#include "vcc/ui/menu/menu_builder.h"


namespace vcc::ui::menu
{

	menu_item_collection::menu_item_collection(item_container_type items)
		: items_(move(items))
	{}

	bool menu_item_collection::empty() const
	{
		return items_.empty();
	}

	void menu_item_collection::accept(visitor_type& visitor) const
	{
		using category_type = ::vcc::ui::menu::detail::menu_category;

		for (const auto& item : items_)
		{
			switch (item.type)
			{
			case category_type::root_sub_menu:
				visitor.root_submenu(item.name, item.icon);
				break;

			case category_type::root_seperator:
				visitor.root_separator();
				break;

			case category_type::sub_menu_separator:
				visitor.submenu_separator();
				break;

			case category_type::sub_menu_item:
				visitor.submenu_item(item.menu_id, item.name, item.icon, item.disabled);
				break;

			case category_type::root_menu_item:
				visitor.root_item(item.menu_id, item.name, item.icon, item.disabled);
				break;
			}
		}
	}

}