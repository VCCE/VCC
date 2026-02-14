//#define USE_LOGGING
//======================================================================
// This file is part of VCC (Virtual Color Computer).
// Vcc is Copyright 2015 by Joseph Forgione
//
// VCC (Virtual Color Computer) is free software, you can redistribute
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//======================================================================

#include <vcc/bus/cartridge_menu.h>
#include <vcc/util/logger.h>

namespace VCC::Bus {

// Global cart menu for vcc.exe use
cartridge_menu gCartMenu;

// ------------------------------------------------------------
// Draw the menu.  DLL's should not call this
// ------------------------------------------------------------
HMENU cartridge_menu::draw(HWND hWnd, int position, const std::string& title)
{
	PrintLogC("Draw\n");
	auto hMenuBar = GetMenu(hWnd);

	// Erase the Existing Cartridge Menu.  Vcc.rc defines
	// a dummy Cartridge menu to preserve it's place.
	DeleteMenu(hMenuBar,position,MF_BYPOSITION);

	// Create first sub menu item
	HMENU hMenu = CreatePopupMenu();
	HMENU hMenu0 = hMenu;

	MENUITEMINFO Mii{};
	memset(&Mii,0,sizeof(MENUITEMINFO));
	Mii.cbSize= sizeof(MENUITEMINFO);

	// Create title bar item
	Mii.fMask = MIIM_TYPE | MIIM_SUBMENU | MIIM_ID;  // setting the submenu id
	Mii.fType = MFT_STRING;                          // Type is a string
	Mii.hSubMenu = hMenu;
	Mii.dwTypeData = const_cast<LPSTR>(title.c_str());
	Mii.cch = title.size();
	InsertMenuItem(hMenuBar,position,TRUE,&Mii);

	// Create sub menus in order
	unsigned int pos = 0u;
	for (CartMenuItem item : menu_) {
		DLOG_C("%4d %d '%s'\n",item.menu_id,item.type,item.name.c_str());
//		should this also be done inside the loop?
//		memset(&Mii,0,sizeof(MENUITEMINFO));
//		Mii.cbSize= sizeof(MENUITEMINFO);
		switch (item.type) {
		case MIT_Head:
			hMenu = CreatePopupMenu();
			Mii.fMask = MIIM_TYPE | MIIM_SUBMENU | MIIM_ID;
			Mii.fType = MFT_STRING;
			Mii.hSubMenu = hMenu;
			Mii.dwTypeData = const_cast<LPSTR>(item.name.c_str());
			Mii.cch = item.name.size();
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
			Mii.fMask = MIIM_TYPE;
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
	DrawMenuBar(hWnd);
	return hMenu0;
}

} // namespace VCC::Bus

