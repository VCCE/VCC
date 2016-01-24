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
	The CoCo Program Pak - ROM Pak or plug-in DLL device
*/
/****************************************************************************/
/*
	Revision history:

	2016-01-12 - @Wersley - Updated to use the new Pak API
*/
/****************************************************************************/
/*
	TODO: add range checking when capturing a request to add a dynamic menu 
			item to keep it from going over the max menu count
*/
/****************************************************************************/

#include "vccPak.h"

// temporary
#include "../_build/win/vcc-core.h"

//
// vcc-core
//
#include "logger.h"
#include "fileops.h"

//
// system
//
#include <process.h>
#include <assert.h>

//
// platform
//
#include <commdlg.h>

/****************************************************************************/

/** the single loaded Pak object - ROM or plug-in DLL */
static vccpak_t		g_Pak = {
	// init blank / empty
	NULL,
	"",
	"Empty",
	"",
	"",
	0,
	{ NULL } 
};	

// dynamic menu info for all Paks
int				g_DynMenuIndex = 0;			///< menu count
vccdynmenu_t	g_DynMenus[MAX_MENUS];	///< menu item info

/** Last path used opening any Pak (ROM or DLL) */
char LastPakPath[FILENAME_MAX] = "";

/****************************************************************************/
/** 
	detect the type of Pak the user is trying to load
*/
vccpaktype_t vccPakGetType(const char * Filename)
{
	FILE *	DummyHandle = NULL;
	char	Temp[3] = "";

	DummyHandle = fopen(Filename, "rb");
	if (DummyHandle == NULL)
	{
		return VCCPAK_TYPE_NOFILE;	//File Doesn't exist
	}

	Temp[0] = fgetc(DummyHandle);
	Temp[1] = fgetc(DummyHandle);
	Temp[2] = 0;
	fclose(DummyHandle);

	if (strcmp(Temp, "MZ") == 0)
	{
		return VCCPAK_TYPE_PLUGIN;
	}

	return VCCPAK_TYPE_ROM;
}

/****************************************************************************/
/****************************************************************************/
/**
*/
void vccPakSetLastPath(char * pLastPakPath)
{
	strcpy(LastPakPath, pLastPakPath);
}

/**
*/
char * vccPakGetLastPath()
{
	return LastPakPath;
}

/****************************************************************************/
/**
*/
int vccPakWantsCart(vccpak_t * pPak)
{
	// TODO: add a flag to disable CART per pak?
	if (pPak != NULL)
	{
		return (pPak->api.setCartPtr != NULL
			|| pPak->ExternalRomBuffer != NULL
			);
	}

	return FALSE;
}

/****************************************************************************/
/**
*/
void vccPakTimer(void)
{
	if ( g_Pak.api.heartbeat != NULL )
	{
		(*g_Pak.api.heartbeat)();
	}
}

/****************************************************************************/
/**
*/
void vccPakReset(void)
{
	g_Pak.BankedCartOffset=0;
	if (g_Pak.api.reset != NULL)
	{
		(*g_Pak.api.reset)();
	}
}

/****************************************************************************/
/**
*/
void vccPakGetStatus(char * pStatusBuffer, size_t bufferSize)
{
	if (g_Pak.api.status != NULL)
	{
		(*g_Pak.api.status)(pStatusBuffer, bufferSize);
	}
	else
	{
		sprintf(pStatusBuffer, "");
	}
}

/****************************************************************************/
/**
*/
unsigned char vccPakPortRead (unsigned char port)
{
	if (g_Pak.api.portRead != NULL)
	{
		return (*g_Pak.api.portRead)(port);
	}

	return 0;
}

/****************************************************************************/
/**
*/
void vccPakPortWrite(unsigned char Port,unsigned char Data)
{
	if ( g_Pak.api.portWrite != NULL )
	{
		(*g_Pak.api.portWrite)(Port,Data);
	}
	
	// change ROM banks?
	// TODO: this should be handled by i/o port registration if this is a ROM pack
	if (    (Port == 0x40) 
		 && (g_Pak.RomPackLoaded == TRUE)
		)
	{
		g_Pak.BankedCartOffset = (Data & 15) << 14;
	}
}

/****************************************************************************/
/**
*/
unsigned char vccPakMem8Read (unsigned short Address)
{
	if (g_Pak.api.memRead != NULL)
	{
		return (*g_Pak.api.memRead)(Address & 32767);
	}

	// TODO: this should be handled by the plug-in if it is a ROM pak only
	if (g_Pak.ExternalRomBuffer != NULL )
	{
		return (g_Pak.ExternalRomBuffer[(Address & 32767) + g_Pak.BankedCartOffset]);
	}
	
	return(0);
}

/****************************************************************************/
/**
*/
void vccPakMem8Write(unsigned char Port,unsigned char Data)
{
	return;
}

/****************************************************************************/
/**
*/
unsigned short vccPackGetAudioSample(void)
{
	if (g_Pak.api.getAudioSample != NULL)
	{
		return(*g_Pak.api.getAudioSample)();
	}
	
	return 0;
}

/****************************************************************************/
/**
*/
void vccPakSetInterruptCallPtr(vccpakapi_assertinterrupt_t CPUAssertInterupt)
{
	if (g_Pak.api.setInterruptCallPtr != NULL)
	{
		(*g_Pak.api.setInterruptCallPtr)(CPUAssertInterupt);
	}
}

/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/**
*/
int vccPakLoadCart(void)
{
	OPENFILENAME ofn ;	
	char szFileName[FILENAME_MAX]="";
	BOOL result;

	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner         = (HWND)g_vccCorehWnd;
	ofn.lpstrFilter       = "Program Packs\0*.ROM;*.BIN;*.DLL\0\0";			// filter string
	ofn.nFilterIndex      = 1 ;							// current filter index
	ofn.lpstrFile         = szFileName ;				// contains full path and filename on return
	ofn.nMaxFile          = FILENAME_MAX;				// sizeof lpstrFile
	ofn.lpstrFileTitle    = NULL;						// filename and extension only
	ofn.nMaxFileTitle     = FILENAME_MAX;				// sizeof lpstrFileTitle
	ofn.lpstrTitle        = TEXT("Load Program Pack") ;	// title bar string
	ofn.Flags             = OFN_HIDEREADONLY;
	ofn.lpstrInitialDir = NULL;							// initial directory
	if (strlen(LastPakPath) > 0)
	{
		ofn.lpstrInitialDir = LastPakPath;
	}

	result = GetOpenFileName(&ofn);
	if (result)
	{
		// save last path
		strcpy(LastPakPath, szFileName);
		PathRemoveFileSpec(LastPakPath);

		if (!vccPakInsertModule(szFileName))
		{
			return(0);
		}
	}

	return(1);
}

/****************************************************************************/
/**
*/
void vccPakSetParamFlags(vccpak_t * pPak)
{
	char String[1024] = "";
	char TempIni[FILENAME_MAX] = "";

	strcat(String, "Module Name: ");
	strcat(String, g_Pak.name);
	strcat(String, "\n");

	if (g_Pak.api.config != NULL)
	{
		g_Pak.params |= VCCPAK_HASCONFIG;

		strcat(String, "Has Configurable options\n");
	}
	if (g_Pak.api.portWrite != NULL)
	{
		g_Pak.params |= VCCPAK_HASIOWRITE;

		strcat(String, "Is IO writable\n");
	}
	if (g_Pak.api.portRead != NULL)
	{
		g_Pak.params |= VCCPAK_HASIOREAD;

		strcat(String, "Is IO readable\n");
	}
	if (g_Pak.api.setInterruptCallPtr != NULL)
	{
		g_Pak.params |= VCCPAK_NEEDSCPUIRQ;

		strcat(String, "Generates Interupts\n");
	}
	if (g_Pak.api.memPointers != NULL)
	{
		g_Pak.params |= VCCPAK_DOESDMA;

		strcat(String, "Generates DMA Requests\n");
	}
	if (g_Pak.api.heartbeat != NULL)
	{
		g_Pak.params |= VCCPAK_NEEDHEARTBEAT;

		strcat(String, "Needs Heartbeat\n");
	}
	if (g_Pak.api.getAudioSample != NULL)
	{
		g_Pak.params |= VCCPAK_ANALOGAUDIO;

		strcat(String, "Analog Audio Outputs\n");
	}
	if (g_Pak.api.memWrite != NULL)
	{
		g_Pak.params |= VCCPAK_CSWRITE;

		strcat(String, "Needs ChipSelect Write\n");
	}
	if (g_Pak.api.memRead != NULL)
	{
		g_Pak.params |= VCCPAK_CSREAD;

		strcat(String, "Needs ChipSelect Read\n");
	}
	if (g_Pak.api.status != NULL)
	{
		g_Pak.params |= VCCPAK_RETURNSSTATUS;

		strcat(String, "Returns Status\n");
	}
	if (g_Pak.api.reset != NULL)
	{
		g_Pak.params |= VCCPAK_CARTRESET;

		strcat(String, "Needs Reset Notification\n");
	}
	if (g_Pak.api.setINIPath != NULL)
	{
		g_Pak.params |= VCCPAK_SAVESINI;
	}
	if (g_Pak.api.setCartPtr != NULL)
	{
		g_Pak.params |= VCCPAK_ASSERTCART;

		strcat(String, "Can Assert CART\n");
	}
}

/****************************************************************************/

void vccPakGetAPI(vccpak_t * pPak)
{
	if (pPak != NULL)
	{
		pPak->api.init = (vccpakapi_init_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_INIT);
		pPak->api.getName = (vccpakapi_getname_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_GETNAME);
		pPak->api.config = (vccpakapi_config_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_CONFIG);
		pPak->api.portWrite = (vccpakapi_portwrite_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_PORTWRITE);
		pPak->api.portRead = (vccpakapi_portread_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_PORTREAD);
		pPak->api.setInterruptCallPtr = (vccpakapi_setintptr_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_ASSERTINTERRUPT);
		pPak->api.memPointers = (vccpakapi_setmemptrs_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_MEMPOINTERS);
		pPak->api.heartbeat = (vccpakapi_heartbeat_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_HEARTBEAT);
		pPak->api.memWrite = (vcccpu_write8_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_MEMWRITE);
		pPak->api.memRead = (vcccpu_read8_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_MEMREAD);
		pPak->api.status = (vccpakapi_status_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_STATUS);
		pPak->api.getAudioSample = (vccpakapi_getaudiosample_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_AUDIOSAMPLE);
		pPak->api.reset = (vccpakapi_reset_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_RESET);
		pPak->api.setINIPath = (vccpakapi_setinipath_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_SETINIPATH);
		pPak->api.setCartPtr = (vccpakapi_setcartptr_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_SETCART);
		pPak->api.dynMenuBuild = (vccpakapi_dynmenubuild_t)GetProcAddress((HINSTANCE)g_Pak.hDLib, VCC_PAKAPI_DYNMENUBUILD);
	}
}

/****************************************************************************/
/**
	TODO: rename function
*/
void vccPakGetCurrentModule(char * DefaultModule)
{
	strcpy(DefaultModule, g_Pak.path);
}

/****************************************************************************/
/**
*/
int vccPakInsertModule(char * ModulePath)
{
	char			Temp[MAX_LOADSTRING]	= "";
	vccpaktype_t	FileType			= VCCPAK_TYPE_UNKNOWN;

	FileType = vccPakGetType(ModulePath);

	switch (FileType)
	{
		//File doesn't exist
	case VCCPAK_TYPE_NOFILE:
		return VCCPAK_NOMODULE;
		break;

	case VCCPAK_TYPE_ROM:
		vccPakUnloadDll();
		vccPakLoadExtROM(ModulePath);
		strncpy(g_Pak.path, ModulePath, FILENAME_MAX);
		strncpy(g_Pak.name,ModulePath, FILENAME_MAX);
		PathStripPath(g_Pak.name);

		vccPakRebuildMenu();

		vccCoreSetResetPending(VCC_RESET_PENDING_HARD);

		// assert CART
		vccCoreSetCart(1);

		return VCCPAK_NOMODULE;
	break;

	case VCCPAK_TYPE_PLUGIN:
		vccPakUnloadDll();

		g_Pak.hDLib = LoadLibrary(ModulePath);
		if ( g_Pak.hDLib == NULL )
		{
			memset(&g_Pak, 0, sizeof(g_Pak));

			return VCCPAK_NOMODULE;
		}
		strcpy(g_Pak.path, ModulePath);

		// clear CART
		vccCoreSetCart(0);

		vccPakGetAPI(&g_Pak);

		// check for required functions, re: plug-in is valid
		if (   g_Pak.api.init == NULL 
			|| g_Pak.api.getName == NULL
			)
		{
			FreeLibrary((HINSTANCE)g_Pak.hDLib);
			memset(&g_Pak, 0, sizeof(g_Pak));
			
			return VCCPAK_NOTVCC;
		}

		//
		// Initialize pak
		//

		g_Pak.BankedCartOffset = 0;

		// init Pak
		(*g_Pak.api.init)(0, g_vccCorehWnd, vccPakDynMenuCallback);
		// Get Pak name
		(*g_Pak.api.getName)(g_Pak.name, g_Pak.catnumber);
		
		strcpy(g_Pak.label, g_Pak.name);
		strcat(g_Pak.label, "  ");
		strcat(g_Pak.label, g_Pak.catnumber);

		vccPakSetParamFlags(&g_Pak);

		if (g_Pak.api.memPointers != NULL)
		{
			// pass in our memory read/write functions
			(*g_Pak.api.memPointers)(vccCoreMemRead, vccCoreMemWrite);
		}
		if (g_Pak.api.setInterruptCallPtr != NULL)
		{
			// pass in our assert interrrupt function
			(*g_Pak.api.setInterruptCallPtr)(vccCoreAssertInterrupt);
		}
		if (g_Pak.api.setCartPtr != NULL)
		{
			// Transfer the address of the SetCart routine to the pak
			(*g_Pak.api.setCartPtr)(vccCoreSetCart);
		}
		// set INI path.  The Pak should load its settings at this point
		if (g_Pak.api.setINIPath != NULL)
		{
			char TempIni[FILENAME_MAX];
			vccCoreGetINIFilePath(TempIni);
			(*g_Pak.api.setINIPath)(TempIni);
		}
		// we should not have to do this here
		// the soft/hard reset should do it
		if (g_Pak.api.reset != NULL)
		{
			(*g_Pak.api.reset)();
		}

		vccPakRebuildMenu();

		vccCoreSetResetPending(VCC_RESET_PENDING_HARD);

		return VCCPAK_LOADED;
		break;
	}

	return VCCPAK_NOMODULE;
}

/****************************************************************************/
/**
	Load a ROM pack
	
	@return total bytes loaded, or 0 on failure
*/
int vccPakLoadExtROM(char * filename)
{
	const size_t PAK_MAX_MEM = 0x40000;

	// If there is an existing ROM, ditch it
	if (g_Pak.ExternalRomBuffer != NULL) 
	{
		free(g_Pak.ExternalRomBuffer);
		g_Pak.ExternalRomBuffer = NULL;
	}
	
	// Allocate memory for the ROM
	g_Pak.ExternalRomBuffer = (uint8_t*)malloc(PAK_MAX_MEM);

	// If memory was unable to be allocated, fail
	if (g_Pak.ExternalRomBuffer == NULL) 
	{
		MessageBox(0, "cant allocate ram", "Ok", 0);
		return 0;
	}
	
	// Open the ROM file, fail if unable to
	FILE *rom_handle = fopen(filename, "rb");
	if (rom_handle == NULL)
	{
		return 0;
	}

	// Load the file, one byte at a time.. (TODO: Get size and read entire block)
	size_t index=0;
	while (    (feof(rom_handle) == 0) 
		    && (index < PAK_MAX_MEM)
		) 
	{
		g_Pak.ExternalRomBuffer[index++] = fgetc(rom_handle);
	}
	fclose(rom_handle);
	
	vccPakUnloadDll();
	g_Pak.BankedCartOffset = 0;
	g_Pak.RomPackLoaded = true;
	  
	return index;
}

/****************************************************************************/
/**
*/
void vccPakUnloadDll(void)
{
	// clear Pak API calls
	memset(&g_Pak.api, 0, sizeof(vccpakapi_t));

	if (g_Pak.hDLib != NULL)
	{
		FreeLibrary((HINSTANCE)g_Pak.hDLib);
	}
	g_Pak.hDLib =NULL;

	vccPakRebuildMenu();
}

/****************************************************************************/
/**
*/
void vccPakUnload(void)
{
	vccPakUnloadDll();

	vccCoreSetCart(0);
	
	if (g_Pak.ExternalRomBuffer != NULL)
	{
		free(g_Pak.ExternalRomBuffer);
	}
	g_Pak.ExternalRomBuffer = NULL;

	memset(&g_Pak, 0, sizeof(vccpak_t));

	vccCoreSetResetPending(VCC_RESET_PENDING_HARD);

	vccPakRebuildMenu();
}

/****************************************************************************/
/****************************************************************************/
/*
	Dynamic menus
*/

/****************************************************************************/

HMENU g_hCartridgeMenu = NULL;

// position in main menu to insert the Cartridge top-level menu
#define MAIN_MENU_PAK_MENU_POSITION	2

/****************************************************************************/
/**
	Create the root Cartridge menu and insert it into the main menu bar
	if it does not exist.
*/
void vccPakRootMenuCreate()
{
	if (g_hCartridgeMenu == NULL)
	{
		char			MenuTitle[MAX_MENU_TEXT];
		MENUITEMINFO	menuItemInfo;

		strcpy(MenuTitle, "Cartridge");

		g_hCartridgeMenu = CreatePopupMenu();

		memset(&menuItemInfo, 0, sizeof(MENUITEMINFO));
		menuItemInfo.cbSize = sizeof(MENUITEMINFO);
		menuItemInfo.fMask = MIIM_TYPE | MIIM_SUBMENU | MIIM_ID;
		menuItemInfo.fType = MFT_STRING;
		menuItemInfo.wID = ID_BASEMENU - 1;
		menuItemInfo.hSubMenu = g_hCartridgeMenu;
		menuItemInfo.dwTypeData = MenuTitle;
		menuItemInfo.cch = strlen(MenuTitle);

		InsertMenuItem(GetMenu(g_vccCorehWnd), MAIN_MENU_PAK_MENU_POSITION, MF_BYPOSITION, &menuItemInfo);
	}
}

/****************************************************************************/
/**
	Remove and destroy the main Cartridge menu from the main menu bar if it exists.
*/
void vccPakRootMenuDestroy()
{
	if (g_hCartridgeMenu != NULL)
	{
		// delete our menu we added to the main menu
		DeleteMenu(GetMenu(g_vccCorehWnd), MAIN_MENU_PAK_MENU_POSITION, MF_BYPOSITION);
		g_hCartridgeMenu = NULL;
	}
}

/****************************************************************************/

void vccPakRebuildMenu()
{
	vccPakDynMenuCallback(0, "", VCC_DYNMENU_FLUSH, DMENU_TYPE_NONE);

	vccPakDynMenuCallback(0, "", VCC_DYNMENU_REFRESH, DMENU_TYPE_NONE);
}

/****************************************************************************/
/**
	Take action on a dynamic menu being activated/selected
*/
void vccPakDynMenuActivated(int MenuItem)
{
	switch (MenuItem)
	{
		// load a new Pak
	case VCC_DYNMENU_ACTION_LOAD:
		vccCoreLoadPack();
		break;

		// eject loaded Pak
	case VCC_DYNMENU_ACTION_UNLOAD:
		vccPakUnload();
		break;

		// Pak specific menu action to be taken
	default:
		if (g_Pak.api.config != NULL)
		{
			(*g_Pak.api.config)(MenuItem);
		}
		break;
	}
}

/****************************************************************************/
/**
	Dynamic menu callback used to add items to the Pak menu, clear the Pak menu,
	or rebuild/refresh the system menu (mirroring the currently defined Pak menu).

	@param MenuName Menu text to display for this menu item
	@param MenuId The ID of the menu to add, defined by the caller
		This can also be two special values.  If these vaslues are used, 
		the other two parameters are ignored.  
		VCC_DYNMENU_FLUSH = Clear the Pak menu
		VCC_DYNMENU_REFRESH	= Rebuild the system 'Cartridge' menu to match the current Pak menu
	@param Type Menu type.  
		DMENU_HEAD = define the main menu for a pak.  The menu will be added to the Cartridge menu as a sub-item
		DMENU_SLAVE = define a menu action item
		DMENU_SEPARATOR = menu separator (added to main Cartridge menu)
		DMENU_STANDALONE = MEnu item to add to main Cartridge menu

	Menus are created in the order they are received.
	Head menus are added directly as a sub-menu of Cartridge.
	Slave items are menu action items added to the most recently defined head menu
	Stand alone items are menu action items added to the Cartridge menu

	Note: Currently you cannot add menu separators into a HEAD menu between SLAVE menu items
*/
void vccPakDynMenuCallback(int id, const char * MenuName, int MenuId, dynmenutype_t Type)
{
	switch (MenuId)
	{
		// reset the menu definitions
		case VCC_DYNMENU_FLUSH:
		{
			char	Temp[MAX_MENU_TEXT];

			// clear all menu definitions
			memset(g_DynMenus, 0, sizeof(g_DynMenus));
			g_DynMenuIndex = 0;

			// 'main menu' for this Pak
			sprintf(Temp, "Cartridge: %s", g_Pak.name);
			vccPakDynMenuCallback(0, Temp, ID_BASEMENU, DMENU_TYPE_HEAD);

			// action load
			vccPakDynMenuCallback(0, "Load Cart", ID_SDYNAMENU + VCC_DYNMENU_ACTION_LOAD, DMENU_TYPE_SLAVE);

			// action unload/eject
			sprintf(Temp, "Eject Cart: %s", g_Pak.name);
			vccPakDynMenuCallback(0, Temp, ID_SDYNAMENU + VCC_DYNMENU_ACTION_UNLOAD, DMENU_TYPE_SLAVE);

			// rebuild menus specific to pak
			if (g_Pak.api.dynMenuBuild != NULL)
			{
				(*g_Pak.api.dynMenuBuild)();
			}
		}
		break;

		// rebuild/refresh the system menu to mirror Pak defined menus
		case VCC_DYNMENU_REFRESH:
			vccPakDynMenuRefresh();
		break;

		// add menu item to the Cartridge menu
		default:
			strcpy(g_DynMenus[g_DynMenuIndex].name,MenuName);
			g_DynMenus[g_DynMenuIndex].id	= MenuId;
			g_DynMenus[g_DynMenuIndex].type	= Type;
			g_DynMenuIndex++;
		break;	
	}
}

/****************************************************************************/
/**
	Rebuild system menu from the Pak menu definitions (mirrors menus defined for the Pak)
	(Windows version)

	@sa vccPakDynMenuCallback for how the menus are set up
*/
void vccPakDynMenuRefresh(void)
{
	MENUITEMINFO	menuItemInfo;
	int				LastHeadMenu = -1;

	// clear and re-create the Cartridge menu
	vccPakRootMenuDestroy();
	vccPakRootMenuCreate();

	// interate through all of the menu definitions
	for (int x=0; x<g_DynMenuIndex; x++)
	{
		// force menu item type to be a separator of there is no string
		if (strlen(g_DynMenus[x].name) == 0)
		{
			g_DynMenus[x].type = DMENU_TYPE_SEPARATOR;
		}

		switch ( g_DynMenus[x].type )
		{
			// create a pop-up menu
		case DMENU_TYPE_HEAD:
			g_DynMenus[x].hMenu = CreatePopupMenu();
			memset(&menuItemInfo,0,sizeof(MENUITEMINFO));
			menuItemInfo.cbSize		= sizeof(MENUITEMINFO);
			menuItemInfo.fMask		= MIIM_TYPE | MIIM_SUBMENU | MIIM_ID;
			menuItemInfo.fType		= MFT_STRING;
			menuItemInfo.wID		= g_DynMenus[x].id;
			menuItemInfo.hSubMenu	= g_DynMenus[x].hMenu;
			menuItemInfo.dwTypeData	= g_DynMenus[x].name;
			menuItemInfo.cch		= strlen(g_DynMenus[x].name);
			InsertMenuItem(g_hCartridgeMenu,GetMenuItemCount(g_hCartridgeMenu),TRUE,&menuItemInfo);
			LastHeadMenu = x;
			break;

			// add menu item to most recently defined HEAD menu
		case DMENU_TYPE_SLAVE:
			memset(&menuItemInfo,0,sizeof(MENUITEMINFO));
			menuItemInfo.cbSize		= sizeof(MENUITEMINFO);
			menuItemInfo.fMask		= MIIM_TYPE |  MIIM_ID;
			menuItemInfo.fType		= MFT_STRING;
			menuItemInfo.wID		= g_DynMenus[x].id;
			menuItemInfo.dwTypeData = g_DynMenus[x].name;
			menuItemInfo.cch		= strlen(g_DynMenus[x].name);
			InsertMenuItem(g_DynMenus[LastHeadMenu].hMenu,GetMenuItemCount(g_DynMenus[LastHeadMenu].hMenu),TRUE,&menuItemInfo);
			break;

			// add a menu separator to the Cartridge menu
		case DMENU_TYPE_SEPARATOR:
			memset(&menuItemInfo, 0, sizeof(MENUITEMINFO));
			menuItemInfo.cbSize = sizeof(MENUITEMINFO);
			menuItemInfo.fType = MF_SEPARATOR;
			menuItemInfo.wID = g_DynMenus[x].id;
			InsertMenuItem(g_hCartridgeMenu, GetMenuItemCount(g_hCartridgeMenu), TRUE, &menuItemInfo);
			break;

			// add a stand-alone menu item to the cartridge menu
		case DMENU_TYPE_STANDALONE:
			memset(&menuItemInfo,0,sizeof(MENUITEMINFO));
			menuItemInfo.cbSize		= sizeof(MENUITEMINFO);
			menuItemInfo.fMask		= MIIM_TYPE |  MIIM_ID;
			menuItemInfo.fType		= MFT_STRING;
			menuItemInfo.wID		= g_DynMenus[x].id;
			menuItemInfo.dwTypeData = g_DynMenus[x].name;
			menuItemInfo.cch		= strlen(g_DynMenus[x].name);
			InsertMenuItem(g_hCartridgeMenu, GetMenuItemCount(g_hCartridgeMenu), TRUE,&menuItemInfo);
			break;
		}
	}

	DrawMenuBar(g_vccCorehWnd);
}

/****************************************************************************/
