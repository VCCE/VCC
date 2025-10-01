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

CartridgeMenu::CartridgeMenu() {};

//  Init. hWin is main window id. Title is menu bar title. Position is where 
//  the menu is placed on title bar
void CartridgeMenu::init(HWND hWin,const char * title,int position) {
PrintLogC("Init %d '%s' %d\n",hWin,title,position);
	hMainWin = hWin;
	BarTitle = title;
	menu_position = position;
}

//  Add menu entry.  cart number: 0 main cart, 1-4 mpi slot carts
void CartridgeMenu::add(const char * name, int menu_id, MenuItemType type) {

PrintLogC("ADD '%s' %d %d\n",name,menu_id,type);

	if (menu_id == MID_BEGIN) {
		if (menu.size() > 2) menu.resize(2);
//		menu.clear();
	} else if (menu_id == MID_FINISH) {
		draw();
	} else {
		menu.push_back({name,menu_id,type});
	}
}

// Draw the menu
HMENU CartridgeMenu::draw() {


	if (hMenuBar == nullptr)  // Can hMainWin change?
		hMenuBar = GetMenu(hMainWin);
	else
		DeleteMenu(hMenuBar,menu_position,MF_BYPOSITION);

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
	Mii.dwTypeData = const_cast<LPSTR>(BarTitle.c_str());
	Mii.cch = BarTitle.size();
	InsertMenuItem(hMenuBar,menu_position,TRUE,&Mii);

	// Create sub menus
	for (size_t i = 0; i < menu.size(); i++) {
PrintLogC("Draw %d '%s' %d %d\n",i,menu[i].name.c_str(), menu[i].menu_id,menu[i].type);
		switch (menu[i].type) {
		case MIT_Head:
			hMenu = CreatePopupMenu();
			Mii.fMask = MIIM_TYPE | MIIM_SUBMENU | MIIM_ID;
			Mii.fType = MFT_STRING;
			Mii.hSubMenu = hMenu;
			Mii.dwTypeData = const_cast<LPSTR>(menu[i].name.c_str());
			InsertMenuItem(hMenu0,0,FALSE,&Mii);
			break;
		case MIT_Slave:
			Mii.fMask = MIIM_TYPE | MIIM_ID;
			Mii.fType = MFT_STRING;
			Mii.wID = menu[i].menu_id;
			Mii.hSubMenu = hMenu;
			Mii.dwTypeData = const_cast<LPSTR>(menu[i].name.c_str());
			Mii.cch=menu[i].name.size();
			InsertMenuItem(hMenu,0,FALSE,&Mii);
			break;
		case MIT_Seperator:
			Mii.fMask = MIIM_TYPE | MIIM_ID;
			Mii.hSubMenu = hMenuBar;
			Mii.dwTypeData = "";
			Mii.cch=0;
			Mii.fType = MF_SEPARATOR;
			InsertMenuItem(hMenu0,0,FALSE,&Mii);
			break;
		case MIT_StandAlone:
			Mii.fMask = MIIM_TYPE | MIIM_ID;
		    Mii.wID = menu[i].menu_id;
			Mii.hSubMenu = hMenuBar;
			Mii.dwTypeData = const_cast<LPSTR>(menu[i].name.c_str());
			Mii.cch=menu[i].name.size();
			Mii.fType = MFT_STRING;
			InsertMenuItem(hMenu0,0,FALSE,&Mii);
			break;
		}
	}
	DrawMenuBar(hMainWin);
	return hMenu0;
}
								 
//  Delete menu items
void CartridgeMenu::del() {
	menu.clear();
}

