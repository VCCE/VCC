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
#include <Windows.h>


class menu_populator : public ::vcc::ui::menu::menu_item_visitor
{
public:

	menu_populator(HMENU root_menu, item_id_type base_menu_id)
		:
		base_menu_id_(base_menu_id),
		root_menu_(root_menu)
	{ }

	void root_submenu(const string_type& text) override
	{
		MENUITEMINFO item_info = {};

		current_submenu_ = CreatePopupMenu();

		item_info.cbSize = sizeof(item_info);
		item_info.fMask = MIIM_TYPE | MIIM_SUBMENU;
		item_info.fType = MFT_STRING;
		item_info.hSubMenu = current_submenu_;
		item_info.dwTypeData = const_cast<LPSTR>(text.c_str());
		item_info.cch = text.size();

		InsertMenuItem(root_menu_, GetMenuItemCount(root_menu_), TRUE, &item_info);
	}

	void root_separator() override
	{
		MENUITEMINFO item_info = {};

		item_info.cbSize = sizeof(item_info);
		item_info.fMask = MIIM_TYPE;
		item_info.fType = MF_SEPARATOR;

		InsertMenuItem(root_menu_, GetMenuItemCount(root_menu_), TRUE, &item_info);
	}

	void root_item(item_id_type id, const string_type& text) override
	{
		MENUITEMINFO item_info = {};

		item_info.cbSize = sizeof(item_info);
		item_info.fMask = MIIM_TYPE | MIIM_ID;
		item_info.fType = MFT_STRING;
		item_info.wID = id + base_menu_id_;
		item_info.dwTypeData = const_cast<LPSTR>(text.c_str());
		item_info.cch = text.size();

		InsertMenuItem(root_menu_, GetMenuItemCount(root_menu_), TRUE, &item_info);

	}
	void submenu_item(item_id_type id, const string_type& text) override
	{
		if (current_submenu_ == nullptr)
		{
			throw std::runtime_error("Cannot add sub menu item. No sub menu to add to");
		}

		MENUITEMINFO item_info = {};

		item_info.cbSize = sizeof(item_info);
		item_info.fMask = MIIM_TYPE | MIIM_ID;
		item_info.fType = MFT_STRING;
		item_info.wID = id + base_menu_id_;
		item_info.dwTypeData = const_cast<LPSTR>(text.c_str());
		item_info.cch = text.size();

		InsertMenuItem(current_submenu_, GetMenuItemCount(current_submenu_), TRUE, &item_info);
	}


private:

	const item_id_type base_menu_id_;
	HMENU root_menu_ = nullptr;
	HMENU current_submenu_ = nullptr;

};
