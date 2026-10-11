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

enum MenuItemType
{
	MIT_Head,       // Menu header, should have slave items
	MIT_Slave,      // Slave items with associated control in header submenu.
	MIT_StandAlone, // Menu item with an associated control
	MIT_Seperator,  // Draws a horizontal line to seperate groups of menu items
};

// Define a C API safe struct for DLL ItemList export
// PakGetMenuItem(menu_item_entry* items, size_t* count)
struct menu_item_entry
{
	char name[256];
	size_t menu_id;
	MenuItemType type;
};

// Sanity check maximum menu items
constexpr auto MAX_MENU_ITEMS = 200u;

// MenuItem ID <-> Command ID mapping
constexpr auto MID_CONTROL = 5000;
constexpr int ControlId(int id) { return MID_CONTROL + id; }

