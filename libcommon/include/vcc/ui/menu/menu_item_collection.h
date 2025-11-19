#pragma once
#include <vcc/ui/menu/menu_item_visitor.h>
#include <vcc/ui/menu/detail/menu_item.h>
#include <vcc/detail/exports.h>
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