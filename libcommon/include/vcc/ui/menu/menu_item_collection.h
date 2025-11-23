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
#include "vcc/ui/menu/menu_item_visitor.h"
#include "vcc/ui/menu/detail/menu_item.h"
#include "vcc/detail/exports.h"
#include <vector>


namespace vcc::ui::menu
{

	class menu_item_collection
	{
	public:

		using item_type = ::vcc::ui::menu::detail::menu_item;
		using item_container_type = std::vector<item_type>;
		using visitor_type = menu_item_visitor;

		LIBCOMMON_EXPORT menu_item_collection() = default;

		LIBCOMMON_EXPORT explicit menu_item_collection(item_container_type items);

		LIBCOMMON_EXPORT bool empty() const;

		LIBCOMMON_EXPORT void accept(visitor_type& visitor) const;


	private:

		item_container_type items_;
	};

	inline menu_item_collection&& move(menu_item_collection& builder)
	{
		return static_cast<menu_item_collection&&>(builder);
	}

}