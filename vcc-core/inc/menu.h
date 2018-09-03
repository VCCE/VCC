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

#ifndef _menu_h_
#define _menu_h_

/*****************************************************************************/

#include "xTypes.h"

/*****************************************************************************/

typedef void *	hmenu_t;
typedef void *	hmenuitem_t;

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	XAPI hmenu_t		menuCreate(const char * pName);
	XAPI result_t		menuAddSubMenu(hmenu_t hMenu, hmenu_t hSubMenu);
	XAPI result_t		menuAddItem(hmenu_t hMenu, const char * pName, int pCommand);
	XAPI result_t		menuAddSeparator(hmenu_t hMenu);
	XAPI result_t		menuDestroy(hmenu_t hMenu);
	XAPI hmenu_t		menuFind(hmenu_t hMenu, const char * pcszName);
	
	XAPI result_t		menuItemSetToolTip(hmenuitem_t hMenuItem, const char * pToolTip);
	XAPI result_t		menuItemSetState(hmenuitem_t hMenuItem, int iState);
	XAPI result_t		menuItemSetStateImage(hmenuitem_t hMenuItem, int iState);
	XAPI result_t		menuItemKey(hmenuitem_t hMenuItem, const char * pszKey, int iModifiers);

#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif // _menu_h_
