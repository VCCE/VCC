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
