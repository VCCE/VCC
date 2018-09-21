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

/*****************************************************************************/
/*
	Cocoa menu implementation
*/
/*****************************************************************************/

#import "OMenu.h"

#include "menu.h"
#include "emuDevice.h"

/*****************************************************************************/
/**
	create menu
 */
hmenu_t menuCreate(const char * pName)
{
	hmenu_t		hMenu	= NULL;
	NSMenu *	pMenu;
	NSString *	pstrTitle;
		
	/*
		Allocate new menu and retain
	 */
	pMenu = [[NSMenu alloc] init];
	//[pMenu retain];
	//assert([pMenu retainCount] == 2);

	/* 
		set menu title
	 */
	pstrTitle = [NSString stringWithCString:pName encoding:NSASCIIStringEncoding];
	[pMenu setTitle:pstrTitle];
	
	hMenu = (hmenu_t)CFBridgingRetain(pMenu);
	
	return hMenu;
}

/*****************************************************************************/
/**
	destroy menu
 */
result_t menuDestroy(hmenu_t hMenu)
{
	result_t	errResult	= XERROR_NONE;
	//NSMenu *	pMenu;

	//pMenu = (NSMenu *)CFBridgingRelease(hMenu);
	//assert([pMenu retainCount] == 2);
	
	// destroy configuration 
	//[pMenu dealloc];
	//[pMenu release];
	//pMenu = nil;
	
	return errResult;
}

/*****************************************************************************/
/**
	Add a menu item to the end of the menu
 */
result_t menuAddItem(hmenu_t hMenu, const char * pName, int iCommand)
{
	result_t		errResult	= XERROR_NONE;
	NSMenu *		pMenu;
	NSMenuItem *	pMenuItem;
	NSString *	pstrTitle;
	
	pMenu = (__bridge NSMenu *)hMenu;
	assert(pMenu != nil);
	
	pMenuItem = [[NSMenuItem alloc] init];
	pstrTitle = [NSString stringWithCString:pName encoding:NSASCIIStringEncoding];
	[pMenuItem setTitle:pstrTitle];
	
	[pMenuItem setTag:iCommand];
	
	[pMenu addItem:pMenuItem];
	
	return errResult;
}

/*****************************************************************************/

result_t menuAddSeparator(hmenu_t hMenu)
{
	result_t		errResult	= XERROR_NONE;
	NSMenu *		pMenu;

	pMenu = (__bridge NSMenu *)hMenu;
	assert(pMenu != nil);
	
	[pMenu addItem:[NSMenuItem separatorItem]];
	
	return errResult;
}

/*****************************************************************************/
/**
	add a sub-menu to the end of the menu
 */
result_t menuAddSubMenu(hmenu_t hMenu, hmenu_t hSubMenu)
{
	result_t		errResult	= XERROR_NONE;
	NSMenu *		pMenu;
	NSMenu *		pSubMenu;
	NSMenuItem *	pSubMenuItem;
	
	pMenu = (__bridge NSMenu *)hMenu;
	assert(pMenu != nil);
	
	pSubMenu = (__bridge NSMenu *)hSubMenu;
	assert(pSubMenu != nil);
	
	pSubMenuItem = [[NSMenuItem alloc] init];
	assert(pSubMenuItem != nil);

	//pstrTitle = [NSString stringWithCString:pName encoding:NSASCIIStringEncoding];
	[pSubMenuItem setTitle:[pSubMenu title]];
	
	[pMenu addItem:pSubMenuItem];
	[pMenu setSubmenu:pSubMenu forItem:pSubMenuItem];
	
	return errResult;
}

/*****************************************************************************/
/**
 */
result_t menuItemSetToolTip(hmenuitem_t hMenuItem, const char * pToolTip)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	NSMenuItem *	pMenuItem	= (__bridge NSMenuItem *)hMenuItem;
	
	if ( pMenuItem != nil )
	{
		assert(0);
	}
	
	return errResult;
}

/*****************************************************************************/
/**
 */
result_t menuItemSetState(hmenuitem_t hMenuItem, int iState)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	NSMenuItem *	pMenuItem	= (__bridge NSMenuItem *)hMenuItem;
	
	if ( pMenuItem != nil )
	{
		switch ( iState )
		{
			case COMMAND_STATE_NONE:
				//[pMenuItem setState:NSMixedState];
			break;
				
			case COMMAND_STATE_OFF:
				[pMenuItem setState:NSOffState];
			break;
				
			case COMMAND_STATE_ON:
				[pMenuItem setState:NSOnState];
			break;
		}
	}
	
	return errResult;
}

/*****************************************************************************/
/**
 */
result_t menuItemSetStateImage(hmenuitem_t hMenuItem, int iState)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	NSMenuItem *	pMenuItem	= (__bridge NSMenuItem *)hMenuItem;
	
	if ( pMenuItem != nil )
	{
		assert(0);
	}
	
	return errResult;
}

/*****************************************************************************/
/**
 */
result_t menuItemKey(hmenuitem_t hMenuItem, const char * pszKey, int iModifiers)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	NSMenuItem *	pMenuItem	= (__bridge NSMenuItem *)hMenuItem;
	
	if ( pMenuItem != nil )
	{
		assert(0);
	}
	
	return errResult;
}

/*****************************************************************************/
/**
	Find a sub-menu (does not descend into sub-menus)
 
	It is assumed the item we are searching for has a sub-menu
 */
hmenu_t menuFind(hmenu_t hMenu, const char * pcszName)
{
	NSMenu *		pMenu		= (__bridge NSMenu *)hMenu;
	NSMenuItem *	pMenuItem;
	NSString *		pTitle;
	hmenu_t			hMenuFound	= NULL;
	
	if ( pMenu != NULL )
	{
		pTitle = [NSString stringWithCString:pcszName encoding:NSASCIIStringEncoding];
		pMenuItem = [pMenu itemWithTitle:pTitle];
		
		hMenuFound = (__bridge hmenu_t)([pMenuItem submenu]);
	}
	
	return hMenuFound;
}

/*****************************************************************************/

