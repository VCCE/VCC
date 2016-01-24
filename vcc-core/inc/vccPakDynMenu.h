#ifndef __VCCPAKDYNMENU_H__
#define __VCCPAKDYNMENU_H__

/****************************************************************************/
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
/****************************************************************************/
/*
	definitions for dynamic menus
*/
/****************************************************************************/

#include "xTypes.h"

// TODO: remove the need for this to be included here
#include <Windows.h>

/****************************************************************************/

#define MAX_MENUS		64		///< Maximum menus (for all Paks)
#define MAX_MENU_TEXT	256		///< Maximum text size for a menu item

//
// Defines the start and end IDs for the dynamic menus
//
#define ID_BASEMENU		6000	// some arbitrary number
#define ID_SDYNAMENU	5000	// start	
#define ID_EDYNAMENU	7000	// end

/**
	Dynamic menu types
*/
typedef enum dynmenutype_t
{
	DMENU_TYPE_NONE = -1,
	DMENU_TYPE_HEAD = 0,
	DMENU_TYPE_SLAVE,
	DMENU_TYPE_SEPARATOR,
	DMENU_TYPE_STANDALONE
} dynmenutype_t;

/**
	Dynamic menu definition
*/
typedef struct vccdynmenu_t  {
	HMENU			hMenu;
	char			name[MAX_MENU_TEXT];
	int				id;
	dynmenutype_t	type;
} vccdynmenu_t;

/*
	Dynamic menu commands
*/
#define VCC_DYNMENU_FLUSH		0
#define VCC_DYNMENU_REFRESH		1

/*
	Common menu actions
*/
#define VCC_DYNMENU_ACTION_CONFIG	0
#define VCC_DYNMENU_ACTION_LOAD		1
#define VCC_DYNMENU_ACTION_UNLOAD	2
#define VCC_DYNMENU_ACTION_COUNT	3

#endif // __VCCPAKDYNMENU_H__



