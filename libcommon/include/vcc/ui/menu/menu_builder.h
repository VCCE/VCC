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
#include <vector>
#include <functional>


namespace vcc::ui::menu
{

	class menu_builder
	{
	public:

		using item_type = ::vcc::ui::menu::detail::menu_item;
		using category_type = item_type::category_type;
		using item_id_type = item_type::item_id_type;
		using string_type = item_type::string_type;
		using item_collection_type = menu_item_collection;
		using item_container_type = menu_item_collection::item_container_type;


	public:

		LIBCOMMON_EXPORT bool empty() const;
		
		LIBCOMMON_EXPORT menu_builder& clear();

		LIBCOMMON_EXPORT menu_builder& add_root_submenu(string_type text);
		LIBCOMMON_EXPORT menu_builder& add_root_separator();
		LIBCOMMON_EXPORT menu_builder& add_root_item(item_id_type id, string_type text);
		LIBCOMMON_EXPORT menu_builder& add_submenu_item(item_id_type id, string_type text);
		LIBCOMMON_EXPORT menu_builder& add_items(const item_collection_type& items, size_t menu_id_offset = 0);

		LIBCOMMON_EXPORT item_collection_type release_items();


	protected:

		LIBCOMMON_EXPORT void add_item(
			category_type category,
			item_id_type id,
			string_type text);

	private:


		item_container_type items_;
	};

	inline menu_builder&& move(menu_builder& builder)
	{
		return static_cast<menu_builder&&>(builder);
	}

}
