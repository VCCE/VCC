#include <vcc/ui/menu/menu_item_collection.h>
#include <vcc/ui/menu/menu_builder.h>


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
				visitor.root_submenu(item.name);
				break;

			case category_type::root_seperator:
				visitor.root_separator();
				break;

			case category_type::sub_menu_item:
				visitor.submenu_item(item.menu_id, item.name);
				break;

			case category_type::root_menu_item:
				visitor.root_item(item.menu_id, item.name);
				break;
			}
		}
	}

}