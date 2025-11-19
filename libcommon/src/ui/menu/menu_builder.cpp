#pragma once
#include <vcc/ui/menu/menu_builder.h>


namespace vcc::ui::menu
{

	namespace
	{

		class menu_copy_visitor : public ::vcc::ui::menu::menu_item_visitor
		{
		public:

			using size_type = std::size_t;


			explicit menu_copy_visitor(menu_builder& menu, size_type menu_id_offset)
				:
				menu_(menu),
				menu_id_offset_(menu_id_offset)
			{}

			void root_submenu(const string_type& text) override
			{
				menu_.add_root_submenu(text);
			}

			void root_separator() override
			{
				menu_.add_root_separator();
			}

			void root_item(item_id_type id, const string_type& text) override
			{
				menu_.add_root_item(id + menu_id_offset_, text);
			}

			void submenu_item(item_id_type id, const string_type& text) override
			{
				menu_.add_submenu_item(id + menu_id_offset_, text);
			}


		private:

			menu_builder& menu_;
			const size_type menu_id_offset_;
		};
	}

	bool menu_builder::empty() const
	{
		return items_.empty();
	}

	menu_builder& menu_builder::clear()
	{
		items_.clear();

		return *this;
	}

	menu_builder& menu_builder::add_root_submenu(string_type text)
	{
		add_item(category_type::root_sub_menu, 0, move(text));

		return *this;
	}
	
	menu_builder& menu_builder::add_root_separator()
	{
		add_item(category_type::root_seperator, 0, {});

		return *this;
	}
	
	menu_builder& menu_builder::add_root_item(item_id_type id, string_type text)
	{
		add_item(category_type::root_menu_item, id, move(text));

		return *this;
	}

	menu_builder& menu_builder::add_submenu_item(item_id_type id, string_type text)
	{
		add_item(category_type::sub_menu_item, id, move(text));

		return *this;
	}

	menu_builder& menu_builder::add_items(const item_collection_type& items, size_t menu_id_offset)
	{
		items.accept(menu_copy_visitor(*this, menu_id_offset));

		return *this;
	}

	menu_builder::item_collection_type menu_builder::release_items()
	{
		return menu_item_collection(move(items_));
	}

	void menu_builder::add_item(
		category_type category,
		item_id_type id,
		string_type text)
	{
		items_.push_back(item_type{ move(text), id, category });
	}

}
