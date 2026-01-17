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

// TODO: move this to libcommon

#include "CartridgeMenu.h"
#include <vcc/util/logger.h>
#include "defines.h"

CartridgeMenu::CartridgeMenu() {};

//  Init. Title is menu bar title. Position is where the menu is placed on title bar
void CartridgeMenu::init(const char * title,int position) {
	MenuBarTitle_ = title;
	MenuPosition_ = position;
	reserve_ = 0;
}

// Menu add requests come from both the pakinterface and carts via config calls;
// pakinterface entries should always be first, followed by carts in slot 4 to 1 in
// that order. pakinterface can set the reserve to keep it's menu items from being
// overwritten from the mpi scan of the slots.
// Set the number of entries to reserve for cartridge menu
void CartridgeMenu::reserve(const unsigned int count) {
	reserve_ = count;
};

//  Add menu entry.
void CartridgeMenu::add(const char * name, unsigned int menu_id, MenuItemType type) {
	switch (menu_id) {
	case MID_BEGIN:
		if (menu_.size() > reserve_) menu_.resize(reserve_);
	    break;
	case MID_FINISH:
		draw();
		break;
	default:
		// Some older DLLs used message type MIT_Head with an empty name for a seperator.
		// To support those here is a check for MIT_Head with an empty name.
		if (type == MIT_Head && strcmp(name,"") == 0) type = MIT_Seperator;
		menu_.push_back({name,menu_id,type});
		break;
	}
}

// Draw the menu
HMENU CartridgeMenu::draw() {

	// Fullscreen toggle changes the WindowHandle
	if (hMenuBar_ == nullptr || hMainWin_ != EmuState.WindowHandle) {
		hMainWin_ = EmuState.WindowHandle;
		hMenuBar_ = GetMenu(hMainWin_);
	}

	// Erase the Existing Cartridge Menu.  Vcc.rc defines a dummy Cartridge menu to
	// preserve it's place.
	DeleteMenu(hMenuBar_,MenuPosition_,MF_BYPOSITION);

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
	Mii.dwTypeData = const_cast<LPSTR>(MenuBarTitle_.c_str());
	Mii.cch = MenuBarTitle_.size();
	InsertMenuItem(hMenuBar_,MenuPosition_,TRUE,&Mii);

	// Create sub menus in order
	unsigned int pos = 0u;
	for (CartMenuItem item : menu_) {
		DLOG_C("%4d %d '%s'\n",item.menu_id,item.type,item.name.c_str());
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
			Mii.hSubMenu = hMenuBar_;
			Mii.dwTypeData = "";
			Mii.cch=0;
			Mii.fType = MF_SEPARATOR;
			InsertMenuItem(hMenu0,pos,TRUE,&Mii);
			pos++;
			break;
		case MIT_StandAlone:
			Mii.fMask = MIIM_TYPE | MIIM_ID;
		    Mii.wID = item.menu_id;
			Mii.hSubMenu = hMenuBar_;
			Mii.dwTypeData = const_cast<LPSTR>(item.name.c_str());
			Mii.cch=item.name.size();
			Mii.fType = MFT_STRING;
			InsertMenuItem(hMenu0,pos,TRUE,&Mii);
			pos++;
			break;
		}
	}
	DrawMenuBar(hMainWin_);
	return hMenu0;
}

