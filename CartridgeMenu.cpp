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

#include "CartridgeMenu.h"
#include "logger.h"
#include "defines.h"

CartridgeMenu::CartridgeMenu() {};

//  Init. Title is menu bar title. Position is where the menu is placed on title bar
void CartridgeMenu::init(const char * title,int position) {
	MenuBarTitle = title;
	MenuPosition = position;
}

//  Add menu entry.
void CartridgeMenu::add(const char * name, unsigned int menu_id, MenuItemType type, unsigned int reserve) {
	switch (menu_id) {
	case MID_BEGIN:
		if (menu.size() > reserve) menu.resize(reserve);
	    break;
	case MID_FINISH:
		draw();
		break;
	default:
		menu.push_back({name,menu_id,type});
		break;
	}
}

// Draw the menu
HMENU CartridgeMenu::draw() {

	// Fullscreen toggle changes the WindowHandle
	if (hMenuBar == nullptr || hMainWin != EmuState.WindowHandle) {
		hMainWin = EmuState.WindowHandle;
		hMenuBar = GetMenu(hMainWin);
	}

	// Erase the Existing Cartridge Menu.  Vcc.rc defines a dummy Cartridge menu to
	// preserve it's place.
	DeleteMenu(hMenuBar,MenuPosition,MF_BYPOSITION);

	// Create first sub menu item
	HMENU hMenu = CreatePopupMenu();
	HMENU hMenu0 = hMenu;

	MENUITEMINFO Mii{};
	memset(&Mii,0,sizeof(MENUITEMINFO));

	// Create title bar item
	Mii.cbSize= sizeof(MENUITEMINFO);
	Mii.fMask = MIIM_TYPE | MIIM_SUBMENU | MIIM_ID;  // setting the submenu id
	Mii.fType = MFT_STRING;                          // Type is a string
	Mii.hSubMenu = hMenu;
	Mii.dwTypeData = const_cast<LPSTR>(MenuBarTitle.c_str());
	Mii.cch = MenuBarTitle.size();
	InsertMenuItem(hMenuBar,MenuPosition,TRUE,&Mii);

	// Create sub menus in order
	unsigned int pos = 0u;
	for (MenuItem item : menu) {
		switch (item.type) {
		case MIT_Head:
			hMenu = CreatePopupMenu();
			Mii.fMask = MIIM_TYPE | MIIM_SUBMENU | MIIM_ID;
			Mii.fType = MFT_STRING;
			Mii.hSubMenu = hMenu;
			Mii.dwTypeData = const_cast<LPSTR>(item.name.c_str());
			InsertMenuItem(hMenu0,pos,TRUE,&Mii);
			pos++;
			break;
		case MIT_Slave:
			Mii.fMask = MIIM_TYPE | MIIM_ID;
			Mii.fType = MFT_STRING;
			Mii.wID = item.menu_id;
			Mii.hSubMenu = hMenu;
			Mii.dwTypeData = const_cast<LPSTR>(item.name.c_str());
			Mii.cch=item.name.size();
			InsertMenuItem(hMenu,0,FALSE,&Mii);
			break;
		case MIT_Seperator:
			Mii.fMask = MIIM_TYPE | MIIM_ID;
			Mii.hSubMenu = hMenuBar;
			Mii.dwTypeData = "";
			Mii.cch=0;
			Mii.fType = MF_SEPARATOR;
			InsertMenuItem(hMenu0,pos,TRUE,&Mii);
			pos++;
			break;
		case MIT_StandAlone:
			Mii.fMask = MIIM_TYPE | MIIM_ID;
		    Mii.wID = item.menu_id;
			Mii.hSubMenu = hMenuBar;
			Mii.dwTypeData = const_cast<LPSTR>(item.name.c_str());
			Mii.cch=item.name.size();
			Mii.fType = MFT_STRING;
			InsertMenuItem(hMenu0,pos,TRUE,&Mii);
			pos++;
			break;
		}
	}
	DrawMenuBar(hMainWin);
	return hMenu0;
}

