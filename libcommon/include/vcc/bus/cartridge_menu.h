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

#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <vcc/util/logger.h>
#include <vcc/bus/cartridge_menuitem.h>

//-----------------------------------------------------------------------
// cartridge_menu objects hold cartridge menu lists.
//
// Every cartridge DLL that uses menu items holds a cartridge_menu
// list for it's items. When it modifies what is in it's list it posts
// a menu changed message to Vcc main.
//
// Menu building and drawing is controlled by the PakInterface and
// is initiated by VCC startup or by a menu changed message from
// one of the catridge DLL's.
//
// PakInterface builds the master cartridge_menu starting with it's
// own items.  It then calls PakGetMenuItems to get and append items
// from the DLL in slot 0 (AKA the boot or side slot).
//
// If the MPI is in slot 0 it responds with it's own list plus the
// lists from each cartridge it has loaded, each via PakGetMenuItems
// calls to them.
//-----------------------------------------------------------------------

namespace VCC::Bus {

// menu_id is an unsigned int between 0 and 49 that can be used by
// a cart to identify menus it adds. Multipak will bias the Menu ID
// by 50 times SlotId to as mechanism to determine which slot a
// message is for.  Control ID is used in windows messages to detect
// menu button pushes. 5000 is added to menu_id to generate these
// control messages. 

//-----------------------------------------------------
// Message ID 5000 - 5249 are reserved for cartridge menus
// Message ID 5251 - 5299 are reseverd for cartridge messaging
//-----------------------------------------------------

// Menu ID <-> Command ID mapping
constexpr auto MID_CONTROL = 5000;
constexpr int ControlId(int id) { return MID_CONTROL + id; }

// A single menu entry
struct CartMenuItem {
    std::string name;
    size_t menu_id;
    MenuItemType type;
};

class cartridge_menu {

private:
    std::vector<CartMenuItem> menu_;

public:

	// Constructor
	cartridge_menu() {}

    // Clear all items
    void clear() { menu_.clear(); }

    // Reserve capacity (for deterministic growth)
    void reserve(size_t count) { menu_.reserve(count); }

    // Add a single item
    void add(const std::string& name, unsigned int menu_id, MenuItemType type)
    {
        menu_.push_back({ name, menu_id, type });
    }

	CartMenuItem& operator[](size_t i)
	{ 
		return menu_[i];
	}

    // Number of items in list
    size_t size() const { return menu_.size(); }

	// debug logging
	void log()
	{
    	for (size_t i = 0; i < menu_.size(); ++i) {
      		PrintLogC("%s %d %d\n", menu_[i].name.c_str(),menu_[i].menu_id,menu_[i].type);
    	}
	}

    // Draw the menu (implemented in catridge_menu.cpp)
	HMENU draw(HWND hWnd,
			int position=3,
			const std::string& title="Cartridge");
};

// create global instance for Vcc.cpp and pakinterface to use.
extern cartridge_menu gCartMenu;

} // namespace VCC::Bus
