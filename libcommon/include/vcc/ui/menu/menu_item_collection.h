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

	/// @brief A read only lightweight collection of menu items.
	///
	/// A read only lightweight collection of menu items that can be processed
	/// through a visitor.
	class menu_item_collection
	{
	public:

		/// @brief Type alias for a menu item.
		using item_type = ::vcc::ui::menu::detail::menu_item;
		/// @brief Type alias for a the container menu items are stored in.
		using item_container_type = std::vector<item_type>;
		/// @brief Type alias for the visitors we accept.
		using visitor_type = menu_item_visitor;


	public:

		LIBCOMMON_EXPORT menu_item_collection() = default;

		/// @brief Construct a Menu Item Collection.
		/// 
		/// @param items The list of items to initialize the collection with.
		LIBCOMMON_EXPORT explicit menu_item_collection(item_container_type items);

		/// @brief Indicates if the collection is empty.
		/// 
		/// @return `true` if the collection is empty; `false` otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT bool empty() const;

		/// @brief Accept a visitor.
		/// 
		/// When the visitor is accepted it will be called for each of the menu items
		/// in the collection, in the same order they are stored.
		/// 
		/// @param visitor The visitor to accept.
		LIBCOMMON_EXPORT void accept(visitor_type& visitor) const;


	private:

		/// @brief The list of menu items in the collection.
		item_container_type items_;
	};

	/// @brief Convert an lvalue reference to a menu item collection to an rvalue.
	/// 
	/// @param collection The item collection to convert.
	/// 
	/// @return The reference passed in `collection` as an `rvalue`.
	inline menu_item_collection&& move(menu_item_collection& collection)
	{
		return static_cast<menu_item_collection&&>(collection);
	}

}