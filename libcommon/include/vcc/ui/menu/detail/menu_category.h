#pragma once

namespace vcc::ui::menu::detail
{

	/// @brief Represents the different types of menu items.
	enum class menu_category
	{
		/// @brief Menu header with no associated control, may have slave items
		root_sub_menu,
		/// @brief Draws a horizontal line to separate groups of menu items
		root_seperator,
		/// @brief Menu item with an associated control
		root_menu_item,
		/// @brief Slave items with associated control in header sub-menu.
		sub_menu_item
	};

}
