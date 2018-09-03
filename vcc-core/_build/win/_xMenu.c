/*****************************************************************************/
/*
	System menu implementation
*/
/*****************************************************************************/

#include "menu.h"
#include "emuDevice.h"

#if !(defined _WIN32)
#	error "WINDOWS implementation"
#endif

#include <Windows.h>
#include <assert.h>

/*****************************************************************************/
/**
	create menu
 */
hmenu_t menuCreate(const char * pName)
{
	hmenu_t		hMenu	= NULL;
	HMENU		pMenu;
		
	/*
		Allocate new menu and retain
	 */
	pMenu = CreateMenu();

	/* 
		set menu title
	 */
	//pstrTitle = [NSString stringWithCString:pName encoding:NSASCIIStringEncoding];
	//[pMenu setTitle:pstrTitle];
	
	hMenu = pMenu;
	
	return hMenu;
}

/*****************************************************************************/
/**
	destroy menu
 */
result_t menuDestroy(hmenu_t hMenu)
{
	result_t	errResult	= XERROR_NONE;
	HMENU		pMenu;

	pMenu = (HMENU)hMenu;
	
	DestroyMenu(pMenu); 
	
	return errResult;
}

/*****************************************************************************/
/**
	Add a menu item to the end of the menu
 */
result_t menuAddItem(hmenu_t hMenu, const char * pName, int iCommand)
{
	result_t		errResult	= XERROR_NONE;

	assert(0);

	return errResult;
}

/*****************************************************************************/

result_t menuAddSeparator(hmenu_t hMenu)
{
	result_t		errResult	= XERROR_NONE;

	assert(0);

	return errResult;
}

/*****************************************************************************/
/**
	add a sub-menu to the end of the menu
 */
result_t menuAddSubMenu(hmenu_t hMenu, hmenu_t hSubMenu)
{
	result_t		errResult	= XERROR_NONE;

	assert(0);

	return errResult;
}

/*****************************************************************************/
/**
 */
result_t menuItemSetToolTip(hmenuitem_t hMenuItem, const char * pToolTip)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	
	if ( hMenuItem != NULL )
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
	
	if ( hMenuItem != NULL )
	{
		assert(0);
	}
	
	return errResult;
}

/*****************************************************************************/
/**
 */
result_t menuItemSetStateImage(hmenuitem_t hMenuItem, int iState)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	
	if ( hMenuItem != NULL )
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
	
	if ( hMenuItem != NULL )
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
	hmenu_t			hMenuFound	= NULL;
	
	if ( hMenu != NULL )
	{
		assert(0);
	}
	
	return hMenuFound;
}

/*****************************************************************************/

