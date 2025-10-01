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

// A menu ID is either a small unsigned integer less than 20 used to identify
// cart menu functions or a control id in the range 5000 - 5019.  Menu ID's map
// directly to control id's by adding 5000. The ControlId() constexpr should be
// used to get the control id from a menu id. There are three "special" message
// IDs as follows:

constexpr auto MID_BEGIN  = 0;  // Clears the menu, optionally reserving some entries.
constexpr auto MID_FINISH = 1;  // Draws the menu
constexpr auto MID_ENTRY  = 2;  // A menu item with no control ID

//  Menu IDs other than the above special ones must be converted to control IDs
//  using the ControlId() constexpr. These reference controls that are activated
//  when the menu item is selected.

//  The MPI adds 20 times the slot number (1-4) to the ID when a cartridge is loaded in
//  an mpi slot, then subtracts this value when the item is activated before using
//  it to call the control in the cartridge dll. This allows module configs to work
//  properly regardless of which slot they are in.

constexpr auto MID_CONTROL = 5000;

// constexpr to convert menu id number to control id
constexpr int ControlId(int id) { return MID_CONTROL + id; }; 

// Menu item types, one of the following.
enum MenuItemType
{
	MIT_Head,       // Menu header with no associated control, may have slave items
	MIT_Slave,      // Slave items with associated control in header submenu.
	MIT_StandAlone, // Menu item with an associated control
	MIT_Seperator,  // Draws a horizontal line to seperate groups of menu items
};

class CartridgeMenu {

private:
	struct MenuItem {
		std::string name;
		unsigned int menu_id;
		MenuItemType type;
	};
	std::vector<MenuItem> menu {};
	HWND hMainWin;
	HMENU hMenuBar;
	std::string MenuBarTitle;
	int MenuPosition;

public:
	CartridgeMenu();

//  Title is menu bar title, position is location menu will be placed on the menu bar.
	void init(const char * title="Cartridge", int position=3);

//  Add menu entry. name is the item text, menu_id and type per above. Reserve is the
//  number of menu items to kept before MID_BEGIN.  A reserve value of 2 allows the
//  'Cartridge' menu item and it's submenu to remain intact when DLL's append items to
//  the menu. The reserve value is only used whwn MID_BEGIN items are encountered.
	void add(const char * name, unsigned int menu_id, MenuItemType type, unsigned int reserve=2);

//  Draw refreshes the menu
    HMENU draw();
};

// Make CartridgeMenu Object global;
extern CartridgeMenu CartMenu;
