#pragma once
#include <vcc/ui/menu/detail/menu_item.h>
#include <vcc/detail/exports.h>


namespace vcc::ui::menu
{

	class LIBCOMMON_EXPORT menu_item_visitor
	{
	public:

		using item_id_type = ::vcc::ui::menu::detail::menu_item::item_id_type;
		using string_type = ::vcc::ui::menu::detail::menu_item::string_type;


	public:

		virtual ~menu_item_visitor() = default;

		virtual void root_submenu(const string_type& text) = 0;
		virtual void root_separator() = 0;
		virtual void root_item(item_id_type id, const string_type& text) = 0;
		virtual void submenu_item(item_id_type id, const string_type& text) = 0;
	};

}