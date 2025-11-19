#pragma once
#include <vcc/ui/menu/detail/menu_category.h>
#include <string>


namespace vcc::ui::menu::detail
{

	struct menu_item
	{
		using category_type = ::vcc::ui::menu::detail::menu_category;
		using item_id_type = unsigned int;
		using string_type = ::std::string;

		string_type name;
		item_id_type menu_id;
		category_type type;
	};

}
