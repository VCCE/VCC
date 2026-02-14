/*
    Copyright 2015 by Joseph Forgione
    This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <vcc/bus/cartridge_menuitem.h> // for menu item type

// A menu ID is either a small unsigned integer less than or equal to 50 used
// to identify cart menu functions with control id in the range 5000 - 5050.
// Menu ID's map directly to control id's by adding 5000. The ControlId()
// constexpr should be used to get the control id from a menu id. There are three
// "special" message IDs as follows:

constexpr auto MID_BEGIN  = 0;  // Clears the menu, optionally reserving some entries.
constexpr auto MID_FINISH = 1;  // Draws the menu
constexpr auto MID_ENTRY  = 2;  // A menu item with no control

//  Menu IDs other than the above special ones must be converted to control IDs
//  using the ControlId() constexpr. These reference controls that are activated
//  when the menu item is selected.

//  The MPI adds 50 times the slot number (1-4) to the ID when a cartridge is loaded in
//  an mpi slot, then subtracts this value when the item is activated before using
//  it to call the control in the cartridge dll. This allows dynamic cartridge menus to
//  work properly regardless of which slot they are in.

constexpr auto MID_CONTROL = 5000;

// constexpr to convert menu id number to control id
constexpr int ControlId(int id) { return MID_CONTROL + id; };

struct CartMenuItem {
	std::string name;
	unsigned int menu_id;
	MenuItemType type;
};

class CartridgeMenu {

private:
	std::vector<CartMenuItem> menu_ {};
	HWND hMainWin_;
	HMENU hMenuBar_;
	std::string MenuBarTitle_;
	int MenuPosition_;
	unsigned int reserve_;

public:
	CartridgeMenu();

//  Title is menu bar title, position is location menu will be placed on the menu bar.
	void init(const char * title="Cartridge", int position=3);

//  Reserve cartmenu items. The reserve value allows the 'Cartridge' menu item and
//  it's submenu to remain intact when DLL's append items to the menu. The reserve
//  value is only used whwn MID_BEGIN items are encountered.
	void reserve(const unsigned int count);

//  Add menu entry. name is the item text, menu_id and type per above. Reserve is the
//  number of menu items to kept before MID_BEGIN.
	void add(const char * name, unsigned int menu_id, MenuItemType type);

//  Draw refreshes the menu
    HMENU draw();
};

// Make CartridgeMenu Object global;
extern CartridgeMenu CartMenu;
