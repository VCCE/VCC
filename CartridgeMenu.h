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

// Menu IDs
//  MID_BEGIN  must be used to begin the menu
//  MID_FINISH must be used to finish the menu
//  5002 thru 5099 are standalone or slave entries.
//  6000 thru 6100 are dynamic entries.
constexpr auto MID_BEGIN     = 0;
constexpr auto MID_FINISH    = 1;
constexpr auto MID_ENTRY     = 6000;
constexpr auto MID_SDYNAMENU = 5000;
constexpr auto MID_EDYNAMENU = 5100;

// Menu item types
enum MenuItemType
{
	MIT_Head,
	MIT_Slave,
	MIT_StandAlone,
	MIT_Seperator,
};

class CartridgeMenu {

private:

	// MenuItem
	struct MenuItem {
		std::string name;
		int menu_id;
		MenuItemType type;
	};
	std::vector<MenuItem> menu {};
	HWND hMainWin;
	HMENU hMenuBar;
	std::string BarTitle;
	int menu_position;

public:

	CartridgeMenu();

//  Init. hWin is main window id. title is menu bar title position is the 
//  menubar position where the dynamic window is inserted.
	void init(HWND hWin,const char * title="Cartridge",int position=3);

//  Add menu entry.
	void add(const char * name, int menu_id, MenuItemType type);

//  Delete menu items
	void del();  

//  Draw refreshes the menu
    HMENU draw();
};

// Make CartridgeMenu Object global;
extern CartridgeMenu CartMenu;
