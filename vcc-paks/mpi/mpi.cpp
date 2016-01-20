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
	This is an expansion module for the Vcc Emulator. 
	
	It simulated the functions of the TRS-80 Multi-Pak Interface

	TODO: emulation is not quite exact, according to...
	
		<snippet from the Cloud9 web site>

		"Here's a simple test that will determine if your Mulit-Pak has already 
		been upgraded to work with a CoCo 3. From BASIC's OK prompt, 
		type PRINT PEEK(&HFF90) and press ENTER. 
		If 126 is returned then the Multi-Pak has been upgraded. 
		If it returns 255 then it has NOT been upgraded."

		VCC is currently producing 204 as the return value

*/
/****************************************************************************/
/*
	Revision history:

	2016-01-12 - @Wersley - Updated to use the new Pak API.  The code is not
							yet optimal, but it seems to work.  A portion of
							code can still be removed once the vcc-core is in
							a library related to Pak object management.
*/
/****************************************************************************/

#include "mpi.h"

#include "../../vcc/fileops.h"
#include "../../vcc/vccPak.h"	// note: only for definitions right now

#include <stdio.h>
#include <assert.h>

#include <windows.h>
#include <commctrl.h>
#include "resource.h" 

/****************************************************************************/
/*
	Local definitions
*/

/** 
	Maximum number of Paks
	This needs to be a power of 2, not more than (1<<4)
	The UI would need to updated to handle more than
	4 Paks dynamically
*/
#define MAXPAX			(1<<2)

/****************************************************************************/

// global/common functions provided by VCC that will be 
// called by the Pak to interface with the emulator
// These will no longer be needed when vcc-core is in a DLL
static vccpakapi_assertinterrupt_t	vccAssertInt		= NULL;
static vcccpu_read8_t				vccMemRead			= NULL;
static vcccpu_write8_t				vccMemWrite			= NULL;
static vccapi_setcart_t				vccSetCart			= NULL;
static vccapi_dynamicmenucallback_t	vccDynMenuCallback	= NULL;

//
// MPI variables
//
static HINSTANCE		g_hinstDLL	= NULL;
static HWND				g_hWnd		= NULL;
static int				g_id		= 0;

static unsigned char	ChipSelectSlot	= 3;
static unsigned char	SpareSelectSlot = 3;
static unsigned char	SwitchSlot		= 3;
static unsigned char	SlotRegister	= 255;

static char				IniFile[FILENAME_MAX] = "";
static char				LastPath[FILENAME_MAX] = "";

//
// Array of Paks - one per MPI slot
//
vccpak_t		g_Paks[MAXPAX];

//
// Dynamic menus
//

// captured menu definitions from all of the Paks we have inserted right now
int				g_DynMenusIndeces[MAXPAX];
vccdynmenu_t	g_DynMenus[MAXPAX][MAX_MENUS];

void mpiDynamicMenuCallback(int id, const char * MenuName, int MenuId, dynmenutype_t Type);

/****************************************************************************/
/**
	Rebuild the Pak menu
*/
void vccPakRebuildMenu()
{
	vccDynMenuCallback(g_id, "", VCC_DYNMENU_FLUSH, DMENU_TYPE_NONE);

	vccDynMenuCallback(g_id, "", VCC_DYNMENU_REFRESH, DMENU_TYPE_NONE);
}

/****************************************************************************/
/**
*/
boolean vccPakWantsCart(vccpak_t * pPak)
{
	// TODO: add a flag to disable CART per pak?

	return (   g_Paks[SpareSelectSlot].api.setCartPtr != NULL
			|| g_Paks[SpareSelectSlot].ExternalRomBuffer != NULL
		);
}

/****************************************************************************/
/**
*/
void vccPakAssertCART(vccpak_t * pPak)
{
	vccSetCart(0);
	if (vccPakWantsCart(pPak))
	{
		vccSetCart(1);
	}
}

/****************************************************************************/
/**
	Get file type user tried to open

	TOOD: use one from VCC
*/
int FileID(char *Filename)
{
	FILE *DummyHandle = NULL;
	char Temp[3] = "";
	DummyHandle = fopen(Filename, "rb");
	if (DummyHandle == NULL)
	{
		return VCCPAK_TYPE_NOFILE;	//File Doesn't exist
	}

	Temp[0] = fgetc(DummyHandle);
	Temp[1] = fgetc(DummyHandle);
	Temp[2] = 0;
	fclose(DummyHandle);

	// TODO: this may not be accurate anymore?
	if (strcmp(Temp, "MZ") == 0)
	{
		return VCCPAK_TYPE_PLUGIN;
	}

	return VCCPAK_TYPE_ROM;
}

/****************************************************************************/
/**
	Unload a Pak from the given slot
*/
void UnloadModule(int Slot)
{
	if (g_Paks[Slot].hDLib != NULL)
	{
		FreeLibrary((HMODULE)g_Paks[Slot].hDLib);
	}
	if (g_Paks[Slot].ExternalRomBuffer != NULL)
	{
		free(g_Paks[Slot].ExternalRomBuffer);
	}
	memset(&g_Paks[Slot], 0, sizeof(vccpak_t));
}

/****************************************************************************/
/**
	Load a Pak into the given slot
*/
int MountModule(int Slot, char * ModName)
{
	unsigned char	ModuleType	= 0;
	char			ModuleName[FILENAME_MAX]="";
	unsigned int	index		= 0;
	FILE *			rom_handle;

	strcpy(ModuleName,ModName);

	// Wah???
	if (Slot > MAXPAX - 1)
	{
		return VCCPAK_ERROR;
	}

	ModuleType = FileID(ModuleName);
	switch (ModuleType)
	{
	case VCCPAK_TYPE_NOFILE:
		return VCCPAK_ERROR;
	break;

	case VCCPAK_TYPE_ROM:
		UnloadModule(Slot);
		g_Paks[Slot].ExternalRomBuffer =(unsigned char *)malloc(0x40000);
		if (g_Paks[Slot].ExternalRomBuffer ==NULL)
		{
			MessageBox(0,"Rom pointer is NULL","Error",0);
			return VCCPAK_ERROR; //Can Allocate RAM
		}
		rom_handle=fopen(ModuleName,"rb");
		if (rom_handle==NULL)
		{
			MessageBox(0,"File handle is NULL","Error",0);
			return VCCPAK_ERROR;
		}
		while ((feof(rom_handle) == 0) & (index < 0x40000))
		{
			g_Paks[Slot].ExternalRomBuffer[index++] = fgetc(rom_handle);
		}
		fclose(rom_handle);
		strcpy(g_Paks[Slot].path,ModuleName);
		PathStripPath(ModuleName);
//		PathRemovePath(ModuleName);
		PathRemoveExtension(ModuleName);
		strcpy(g_Paks[Slot].name,ModuleName);
		strcpy(g_Paks[Slot].label,ModuleName); //JF

		return VCCPAK_LOADED;
	break;

	case VCCPAK_TYPE_PLUGIN:
		UnloadModule(Slot);
		strcpy(g_Paks[Slot].path,ModuleName);
		g_Paks[Slot].hDLib = LoadLibrary(ModuleName);
		if (g_Paks[Slot].hDLib == NULL)
		{
			return VCCPAK_NOMODULE;	//Error Can't open File
		}

		g_Paks[Slot].api.init		= (vccpakapi_init_t)GetProcAddress((HINSTANCE)g_Paks[Slot].hDLib, VCC_PAKAPI_INIT);
		g_Paks[Slot].api.getName	= (vccpakapi_getname_t)GetProcAddress((HMODULE)g_Paks[Slot].hDLib, VCC_PAKAPI_GETNAME);
		g_Paks[Slot].api.config		= (vccpakapi_config_t)GetProcAddress((HMODULE)g_Paks[Slot].hDLib, VCC_PAKAPI_CONFIG);
		g_Paks[Slot].api.portWrite	= (vccpakapi_portwrite_t) GetProcAddress((HMODULE)g_Paks[Slot].hDLib, VCC_PAKAPI_PORTWRITE);
		g_Paks[Slot].api.portRead	= (vccpakapi_portread_t) GetProcAddress((HMODULE)g_Paks[Slot].hDLib, VCC_PAKAPI_PORTREAD);
		g_Paks[Slot].api.heartbeat	= (vccpakapi_heartbeat_t) GetProcAddress((HMODULE)g_Paks[Slot].hDLib, VCC_PAKAPI_HEARTBEAT);
		g_Paks[Slot].api.memWrite	= (vcccpu_write8_t) GetProcAddress((HMODULE)g_Paks[Slot].hDLib, VCC_PAKAPI_MEMWRITE);
		g_Paks[Slot].api.memRead	= (vcccpu_read8_t) GetProcAddress((HMODULE)g_Paks[Slot].hDLib, VCC_PAKAPI_MEMREAD);
		g_Paks[Slot].api.status		= (vccpakapi_status_t) GetProcAddress((HMODULE)g_Paks[Slot].hDLib, VCC_PAKAPI_STATUS);
		g_Paks[Slot].api.getAudioSample = (vccpakapi_getaudiosample_t) GetProcAddress((HMODULE)g_Paks[Slot].hDLib, VCC_PAKAPI_AUDIOSAMPLE);
		g_Paks[Slot].api.reset		= (vccpakapi_reset_t) GetProcAddress((HMODULE)g_Paks[Slot].hDLib, VCC_PAKAPI_RESET);

		g_Paks[Slot].api.setInterruptCallPtr = (vccpakapi_setintptr_t)GetProcAddress((HMODULE)g_Paks[Slot].hDLib, VCC_PAKAPI_ASSERTINTERRUPT);
		g_Paks[Slot].api.memPointers = (vccpakapi_setmemptrs_t)GetProcAddress((HMODULE)g_Paks[Slot].hDLib, VCC_PAKAPI_MEMPOINTERS);
		g_Paks[Slot].api.setCartPtr	= (vccpakapi_setcartptr_t)GetProcAddress((HMODULE)g_Paks[Slot].hDLib, VCC_PAKAPI_SETCART);
		g_Paks[Slot].api.setINIPath = (vccpakapi_setinipath_t)GetProcAddress((HMODULE)g_Paks[Slot].hDLib, VCC_PAKAPI_SETINIPATH);
		g_Paks[Slot].api.dynMenuBuild = (vccpakapi_dynmenubuild_t)GetProcAddress((HINSTANCE)g_Paks[Slot].hDLib, VCC_PAKAPI_DYNMENUBUILD);

		g_Paks[Slot].api.dynMenuCallback = mpiDynamicMenuCallback;

		if (   g_Paks[Slot].api.init == NULL
			|| g_Paks[Slot].api.getName == NULL
			)
		{
			UnloadModule(Slot);
			MessageBox(0,"Not a valid Module","Ok",0);
			return VCCPAK_NOTVCC; //Error Not a Vcc Module 
		}
		
		// init Pak - id is Slot number (1 is added since our id is probably 0)
		(*g_Paks[Slot].api.init)(Slot + 1, g_hWnd, mpiDynamicMenuCallback);
		// get Pak name
		(*g_Paks[Slot].api.getName)(g_Paks[Slot].name, g_Paks[Slot].catnumber);
		
		strcpy(g_Paks[Slot].label, g_Paks[Slot].name);
		strcat(g_Paks[Slot].label,"  ");
		strcat(g_Paks[Slot].label, g_Paks[Slot].catnumber);

		if (g_Paks[Slot].api.setInterruptCallPtr != NULL)
		{
			(*g_Paks[Slot].api.setInterruptCallPtr)(vccAssertInt);
		}
		if (g_Paks[Slot].api.memPointers != NULL)
		{
			(*g_Paks[Slot].api.memPointers)(vccMemRead, vccMemWrite);
		}
		if (g_Paks[Slot].api.setCartPtr != NULL)
		{
			// Transfer the address of the SetCart routine to the pak
			(*g_Paks[Slot].api.setCartPtr)(vccSetCart);
		}
		// set INI path.  The Pak should load its settings at this point
		if (g_Paks[Slot].api.setINIPath != NULL)
		{
			(*g_Paks[Slot].api.setINIPath)(IniFile);
		}
		// we should not have to do this here
		// the soft/hard reset should do it
		if (g_Paks[Slot].api.reset != NULL)
		{
			(*g_Paks[Slot].api.reset)();
		}

		// TODO: pass hard reset call back to VCC
		//EmuState.ResetPending = VCC_RESET_PENDING_HARD;

		return VCCPAK_LOADED;
	break;
	}

	return(0);
}

/****************************************************************************/
/**
*/
void LoadCartDLL(unsigned char Slot,char *DllPath)
{
	OPENFILENAME ofn ;
	unsigned char RetVal=0;

	UnloadModule(Slot);
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner         = g_hWnd;
	ofn.lpstrFilter       =	"Program Packs\0*.ROM;*.DLL\0\0" ;	// filter string
	ofn.nFilterIndex      = 1 ;									// current filter index
	ofn.lpstrFile         = DllPath;							// contains full path and filename on return
	ofn.nMaxFile          = FILENAME_MAX;						// sizeof lpstrFile
	ofn.lpstrFileTitle    = NULL;								// filename and extension only
	ofn.nMaxFileTitle     = FILENAME_MAX ;						// sizeof lpstrFileTitle
	ofn.lpstrTitle        = TEXT("Load Program Pack");			// title bar string
	ofn.Flags             = OFN_HIDEREADONLY;
	ofn.lpstrInitialDir = NULL;									// initial directory
	
	if (strlen(LastPath) > 0)
	{
		ofn.lpstrInitialDir = LastPath;
	}

	if ( GetOpenFileName (&ofn) )
	{
		// save last path
		strcpy(LastPath, DllPath);
		PathRemoveFileSpec(LastPath);

		RetVal = MountModule(Slot,DllPath);
	}
}

/****************************************************************************/
/**
*/
void LoadConfig(void)
{
	char	ModName[MAX_LOADSTRING]="";

	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	SwitchSlot=GetPrivateProfileInt(ModName,"SWPOSITION",MAXPAX-1,IniFile);
	ChipSelectSlot=SwitchSlot;
	SpareSelectSlot=SwitchSlot;

	for (int Slot = 0; Slot < MAXPAX; Slot++)
	{
		char	SlotName[MAX_LOADSTRING];

		sprintf(SlotName, "SLOT%d", Slot);
		GetPrivateProfileString(ModName, SlotName, "", g_Paks[Slot].path, FILENAME_MAX, IniFile);
		CheckPath(g_Paks[Slot].path);

		if (strlen(g_Paks[Slot].path) != 0)
		{
			MountModule(Slot, g_Paks[Slot].path);
		}
	}

	GetPrivateProfileString(ModName, "LastPath", "", LastPath, FILENAME_MAX, IniFile);

	vccPakRebuildMenu();
}

/****************************************************************************/
/**
*/
void WriteConfig(void)
{
	char ModName[MAX_LOADSTRING]="";

	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	WritePrivateProfileInt(ModName,"SWPOSITION",SwitchSlot,IniFile);

	for (int Slot = 0; Slot < MAXPAX; Slot++)
	{
		char	SlotName[MAX_LOADSTRING];

		sprintf(SlotName, "SLOT%d", Slot);

		ValidatePath(g_Paks[Slot].path);
		WritePrivateProfileString(ModName, SlotName, g_Paks[Slot].path, IniFile);
	}
	
	WritePrivateProfileString(ModName, "LastPath", LastPath, IniFile);
}

/****************************************************************************/
/**
	TODO: use vccPak call when it is in a library
*/
void ReadModuleParms(unsigned char Slot, char *String)
{
	strcat(String, "Module Name: ");
	strcat(String, g_Paks[Slot].name);

	strcat(String, "\r\n-----------------------------------------\r\n");

	if (g_Paks[Slot].api.config != NULL)
	{
		strcat(String, "Has Configurable options\r\n");
	}

	if (g_Paks[Slot].api.setINIPath != NULL)
	{
		strcat(String, "Saves Config Info\r\n");
	}

	if (g_Paks[Slot].api.portWrite != NULL)
	{
		strcat(String, "Is IO writable\r\n");
	}

	if (g_Paks[Slot].api.portRead != NULL)
	{
		strcat(String, "Is IO readable\r\n");
	}

	if (g_Paks[Slot].api.setInterruptCallPtr != NULL)
	{
		strcat(String, "Generates Interupts\r\n");
	}

	if (g_Paks[Slot].api.memPointers != NULL)
	{
		strcat(String, "Generates DMA Requests\r\n");
	}

	if (g_Paks[Slot].api.heartbeat != NULL)
	{
		strcat(String, "Needs Heartbeat\r\n");
	}

	if (g_Paks[Slot].api.getAudioSample != NULL)
	{
		strcat(String, "Analog Audio Outputs\r\n");
	}

	if (g_Paks[Slot].api.memWrite != NULL)
	{
		strcat(String, "Needs CS Write\r\n");
	}

	if (g_Paks[Slot].api.memRead != NULL)
	{
		strcat(String, "Needs CS Read (onboard ROM)\r\n");
	}

	if (g_Paks[Slot].api.status != NULL)
	{
		strcat(String, "Returns Status\r\n");
	}

	if (g_Paks[Slot].api.reset != NULL)
	{
		strcat(String, "Needs Reset Notification\r\n");
	}

	if (vccPakWantsCart(&g_Paks[Slot]) )
	{
			strcat(String,"Asserts CART\r\n");
	}
}

/****************************************************************************/
/**
The MPI dynamic menu callback.  We pass this to each Pak to capture
their menu definitions.
*/
void mpiDynamicMenuCallback(int id, const char * MenuName, int MenuId, dynmenutype_t Type)
{
	int Slot = id - 1;

	assert(Slot >= 0 && Slot < MAXPAX);

	if (MenuId == VCC_DYNMENU_FLUSH)
	{
		// clear our captured menus
		for (Slot = 0; Slot < MAXPAX; Slot++)
		{
			g_DynMenusIndeces[Slot] = 0;
		}

		vccDynMenuCallback(g_id, "", VCC_DYNMENU_FLUSH, DMENU_TYPE_NONE);

		return;
	}

	if (MenuId == VCC_DYNMENU_REFRESH)
	{
		vccDynMenuCallback(g_id, "", VCC_DYNMENU_REFRESH, DMENU_TYPE_NONE);

		return;
	}

	//
	// Capture menu definition from our contained Paks
	//
	strcpy(g_DynMenus[Slot][g_DynMenusIndeces[Slot]].name, MenuName);
	g_DynMenus[Slot][g_DynMenusIndeces[Slot]].id = MenuId;
	g_DynMenus[Slot][g_DynMenusIndeces[Slot]].type = Type;
	g_DynMenusIndeces[Slot]++;
}

/****************************************************************************/
/****************************************************************************/
/*
	VCC Pak API
*/
/****************************************************************************/

/**
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_INIT(int id, void * wndHandle, vccapi_dynamicmenucallback_t Temp)
{
	g_id = id;
	g_hWnd = (HWND)wndHandle;

	vccDynMenuCallback = Temp;
}

/**
	Return our module name to the emulator
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_GETNAME(char * ModName, char * CatNumber)
{
	LoadString(g_hinstDLL, IDS_MODULE_NAME, ModName, MAX_LOADSTRING);
	LoadString(g_hinstDLL, IDS_CATNUMBER, CatNumber, MAX_LOADSTRING);
}

/****************************************************************************/
/**
	This captures the Function transfer point for the CPU assert interupt
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_ASSERTINTERRUPT(vccpakapi_assertinterrupt_t vccAssertInterrupt)
{
	vccAssertInt = vccAssertInterrupt;

	//
	// Pass the VCC Assert|Interrupt function pointer 
	// to all currently loaded Paks
	//
	for (int Slot = 0; Slot < MAXPAX; Slot++)
	{
		if (g_Paks[Slot].api.setInterruptCallPtr != NULL)
		{
			(*g_Paks[Slot].api.setInterruptCallPtr)(vccAssertInt);
		}
	}
}

/****************************************************************************/
/**
	Write a byte to one of our i/o ports
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_PORTWRITE(unsigned char Port, unsigned char Data)
{
	if (Port == 0x7F) // Addressing the Multi-Pak
	{
		SpareSelectSlot = (Data & (MAXPAX-1));
		ChipSelectSlot  = ((Data >> 4) & (MAXPAX-1));
		assert(SpareSelectSlot < MAXPAX && ChipSelectSlot < MAXPAX);
		SlotRegister = Data;

		vccPakAssertCART(&g_Paks[ChipSelectSlot]);
	}
	else
	// verify range is correct
	if ( (Port>=0x40) & (Port<0x7F) )
	{
		//g_Paks[SpareSelectSlot].BankedCartOffset[SpareSelectSlot] = (Data & 15)<<14;

		if ( g_Paks[SpareSelectSlot].api.portWrite != NULL)
		{
			(*g_Paks[SpareSelectSlot].api.portWrite)(Port, Data);
		}
	}
	else
	{
		// TODO: what does the real MPI do here?
		// this seems like a bit of a hack so the harddisk
		// plug-in would work as a program pack

		// Find a Module that we can write to
		// Should we even ever get to this point?
		// this assumes there is no conflict between two carts
		// and writes to the lowest numbered slot
		// that will accept it
		for (int Slot = 0; Slot < MAXPAX; Slot++)
		{
			if (g_Paks[Slot].api.portWrite != NULL)
			{
				(*g_Paks[Slot].api.portWrite)(Port, Data);
			}
		}
	}
}

/****************************************************************************/
/**
	Read a byte from one of our i/o ports
*/
extern "C" __declspec(dllexport) unsigned char VCC_PAKAPI_DEF_PORTREAD(unsigned char Port)
{
	// MPI register?
	if ( Port == 0x7F )
	{
		assert(SpareSelectSlot < MAXPAX && ChipSelectSlot < MAXPAX);
		SlotRegister |= (SpareSelectSlot | (ChipSelectSlot << 4));

		return (SlotRegister);
	}
	else
	// TODO: verify range is correct
	// SCS range?
	if ( (Port>=0x40) & (Port<0x7F) )
	{
		if (g_Paks[SpareSelectSlot].api.portRead != NULL )
		{
			return (*g_Paks[SpareSelectSlot].api.portRead)(Port);
		}
		
		// is this what the MPI would return
		// if there is no CART in that slot?
		return 0;
	}
	else
	{
		// TODO: what does the real MPI do here?
		// this seems like a bit of a hack so the harddisk
		// plug-in would work as a program pack

		// Find a Module that returns a value
		// Should we even ever get to this point?
		// this assumes there is no conflict between two carts
		// and takes the first 'valid' value starting
		// from the lowest numbered slot
		for (int Slot = 0; Slot < MAXPAX; Slot++)
		{
			if (g_Paks[Slot].api.portRead != NULL )
			{
				int Temp2 = (*g_Paks[Slot].api.portRead)(Port);

				if (Temp2 != 0)
				{
					return(Temp2);
				}
			}
		}
	}

	return(0);
}

/****************************************************************************/
/**
	Periodic call from the emulator to us if we or our Paks need it
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_HEARTBEAT(void)
{
	for (int Slot = 0; Slot < MAXPAX; Slot++)
	{
		if (g_Paks[Slot].api.heartbeat != NULL)
		{
			(*g_Paks[Slot].api.heartbeat)();
		}
	}
}

/****************************************************************************/
/**
	This captures the pointers to the MemRead8 and MemWrite8 functions. This allows the DLL to do DMA xfers with CPU ram.
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_MEMPOINTERS(vcccpu_read8_t Temp1, vcccpu_write8_t Temp2)
{
	vccMemRead = Temp1;
	vccMemWrite = Temp2;
}

/****************************************************************************/
/**
	Read byte from memory (ROM) in the current CTS slot
*/
extern "C" __declspec(dllexport) unsigned char VCC_PAKAPI_DEF_MEMREAD(unsigned short Address)
{
	if (g_Paks[ChipSelectSlot].ExternalRomBuffer != NULL)
	{
		//Bank Select ???
		return(g_Paks[ChipSelectSlot].ExternalRomBuffer[(Address & 32767) + g_Paks[ChipSelectSlot].BankedCartOffset]);
	}

	if (g_Paks[ChipSelectSlot].api.memRead != NULL)
	{
		return(g_Paks[ChipSelectSlot].api.memRead(Address));
	}

	return 0;
}

/****************************************************************************/
/**
	Memory write (not implemented - cannot write to general addresses for us
	or connected Paks)
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_MEMWRITE(unsigned char Data, unsigned short Address)
{
	// no memory write
}

/****************************************************************************/
/**
	Get module status

	Return our status plus the status of all Paks appended
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_STATUS(char * buffer, size_t bufferSize)
{
	char TempStatus[256] = "";

	sprintf(buffer, "MPI:%i,%i", ChipSelectSlot, SpareSelectSlot);

	for (int Slot = 0; Slot<MAXPAX; Slot++)
	{
		strcpy(TempStatus, "");

		if (g_Paks[Slot].api.status != NULL)
		{
			(*g_Paks[Slot].api.status)(TempStatus,sizeof(TempStatus));

			strcat(buffer, "|");
			strcat(buffer, TempStatus);
		}
	}
}

/****************************************************************************/
/**
	Get audio sample

	This gets called at the end of every scan line 262 Lines * 60 Frames = 15780 Hz 15720

	All paks are scanned and the samples are merged/mixed
*/
extern "C" __declspec(dllexport) unsigned short VCC_PAKAPI_DEF_AUDIOSAMPLE(void)
{
	unsigned short TempSample = 0;

	for (int Slot = 0; Slot<MAXPAX; Slot++)
	{
		if (g_Paks[Slot].api.getAudioSample != NULL)
		{
			TempSample += (*g_Paks[Slot].api.getAudioSample)();
		}
	}

	return (TempSample);
}

/****************************************************************************/
/**
	RESET - pass onto each Pak
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_RESET(void)
{
	ChipSelectSlot = SwitchSlot;
	SpareSelectSlot = SwitchSlot;

	for (int Slot = 0; Slot<MAXPAX; Slot++)
	{
		// Do I need to keep independant selects?
		g_Paks[Slot].BankedCartOffset = 0; 

		if (g_Paks[Slot].api.reset != NULL)
		{
			(*g_Paks[Slot].api.reset)();
		}
	}

	vccPakAssertCART(&g_Paks[ChipSelectSlot]);
}

/****************************************************************************/
/**
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_SETINIPATH(char *IniFilePath)
{
	strcpy(IniFile, IniFilePath);
	LoadConfig();
}

/****************************************************************************/
/**
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_SETCART(vccapi_setcart_t Pointer)
{
	vccSetCart = Pointer;
}

/****************************************************************************/
/****************************************************************************/
/*
	Dynamic menu handling

	We capture the dynamic menu definitions from each Pak.  Then build the
	full Pak menu with our own definition and the definitions from each
	Pak when needed.
*/

/****************************************************************************/
/**
*/

#define MENU_ITEMS_PER_SLOT	100
#define MENU_PAK_BASE		20

extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_DYNMENUBUILD()
{
	char	TempMsg[MAX_MENU_TEXT] = "";

	if ( vccDynMenuCallback == NULL )
	{
		// nice message
		MessageBox(g_hWnd, "No good", "Ok", 0);
		return;
	}

	// Pass on the build menu request to all of our Paks
	for (int Slot = 0; Slot < MAXPAX; Slot++)
	{
		// clear this Pak's dynamic menus
		g_DynMenusIndeces[Slot] = 0;

		if (g_Paks[Slot].api.dynMenuBuild != NULL)
		{
			(*g_Paks[Slot].api.dynMenuBuild)();
		}
	}

	//
	// Now build the full menu
	//
	vccDynMenuCallback(g_id, "", ID_SDYNAMENU, DMENU_TYPE_SEPARATOR);

	for (int Slot = MAXPAX - 1; Slot >= 0; Slot--)
	{
		// calculate base id for this slot
		int		pakBaseID = ID_SDYNAMENU + MENU_PAK_BASE + (Slot * MENU_ITEMS_PER_SLOT);

		//
		// Added normal load/unload menu items for each slot
		//
		sprintf(TempMsg, "MPI Slot %d", Slot + 1);
		vccDynMenuCallback(g_id, TempMsg, ID_BASEMENU + Slot + 1, DMENU_TYPE_HEAD);

		vccDynMenuCallback(g_id, "Insert", pakBaseID + VCC_DYNMENU_ACTION_LOAD, DMENU_TYPE_SLAVE);

		sprintf(TempMsg, "Eject: %s", g_Paks[Slot].label);
		vccDynMenuCallback(g_id, TempMsg, pakBaseID + VCC_DYNMENU_ACTION_UNLOAD, DMENU_TYPE_SLAVE);
	}

	// add MPI config menu
	vccDynMenuCallback(g_id, "MPI Config", ID_SDYNAMENU + VCC_DYNMENU_ACTION_CONFIG, DMENU_TYPE_STANDALONE);

	//
	// now add the menus from Paks
	//
	for (int Slot = MAXPAX - 1; Slot >= 0; Slot--)
	{
		// calculate base id for this slot
		int		pakBaseID = ID_SDYNAMENU + MENU_PAK_BASE + (Slot * MENU_ITEMS_PER_SLOT);

		for (int TempIndex = 0; TempIndex < g_DynMenusIndeces[Slot]; TempIndex++)
		{
			vccDynMenuCallback(g_id,
				g_DynMenus[Slot][TempIndex].name,
				// save as an index into the dynamic menus to get 
				// the correct menu ID for this Pak
				pakBaseID + TempIndex + VCC_DYNMENU_ACTION_COUNT,
				g_DynMenus[Slot][TempIndex].type
				);
		}
	}
}

/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/**
	Call config UI - pass onto correct Pak based on menu ID
*/

// forward declaration of the Windows Config dialog WndProc
LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_CONFIG(int MenuID)
{
	if (MenuID == VCC_DYNMENU_ACTION_CONFIG)
	{
		// MPI config
		DialogBox(g_hinstDLL, (LPCTSTR)IDD_DIALOG1, g_hWnd, (DLGPROC)Config);
	}
	else
	{
		// base on the MenuID, figure out what slot it is for and the menu id to pass on
		int		temp	= MenuID - MENU_PAK_BASE;
		int		Slot	= temp / MENU_ITEMS_PER_SLOT;
		int		index	= temp % MENU_ITEMS_PER_SLOT;

		// check if in range
		if (Slot < MAXPAX)
		{
			switch ( index )
			{
			case VCC_DYNMENU_ACTION_LOAD:
				LoadCartDLL(Slot, g_Paks[Slot].path);
				WriteConfig();
				vccPakRebuildMenu();
				break;

			case VCC_DYNMENU_ACTION_UNLOAD:
				UnloadModule(Slot);
				vccPakRebuildMenu();
				break;

			default:
				assert(index - VCC_DYNMENU_ACTION_COUNT >= 0);

				// this should not happen (the config function being NULL and us getting a menu id)
				assert(g_Paks[Slot].api.config != NULL);
				
				if (g_Paks[Slot].api.config != NULL)
				{
					// look up real menu ID to pass to this Pak config
					(*g_Paks[Slot].api.config)(g_DynMenus[Slot][index - VCC_DYNMENU_ACTION_COUNT].id - ID_SDYNAMENU);
				}
				break;
			}
		}
	}
}

/****************************************************************************/
/*
	Debug only simple check to verify API matches the definitions
	If the Pak API changes for one of our defined functions it
	should produce a compile error
*/

#ifdef _DEBUG

static vccpakapi_init_t				__init				= VCC_PAKAPI_DEF_INIT;
static vccpakapi_getname_t			__getName			= VCC_PAKAPI_DEF_GETNAME;
static vccpakapi_dynmenubuild_t		__dynMenuBuild		= VCC_PAKAPI_DEF_DYNMENUBUILD;
static vccpakapi_config_t			__config			= VCC_PAKAPI_DEF_CONFIG;
static vccpakapi_heartbeat_t		__heartbeat			= VCC_PAKAPI_DEF_HEARTBEAT;
static vccpakapi_status_t			__status			= VCC_PAKAPI_DEF_STATUS;
static vccpakapi_getaudiosample_t	__getAudioSample	= VCC_PAKAPI_DEF_AUDIOSAMPLE;
static vccpakapi_reset_t			__reset				= VCC_PAKAPI_DEF_RESET;
static vccpakapi_portread_t			__portRead			= VCC_PAKAPI_DEF_PORTREAD;
static vccpakapi_portwrite_t		__portWrite			= VCC_PAKAPI_DEF_PORTWRITE;
static vcccpu_read8_t				__memRead			= VCC_PAKAPI_DEF_MEMREAD;
static vcccpu_write8_t				__memWrite			= VCC_PAKAPI_DEF_MEMWRITE;
static vccpakapi_setmemptrs_t		__memPointers		= VCC_PAKAPI_DEF_MEMPOINTERS;
static vccpakapi_setcartptr_t		__setCartPtr		= VCC_PAKAPI_DEF_SETCART;
static vccpakapi_setintptr_t		__assertInterrupt	= VCC_PAKAPI_DEF_ASSERTINTERRUPT;
static vccpakapi_setinipath_t		__setINIPath		= VCC_PAKAPI_DEF_SETINIPATH;

#endif // _DEBUG

/****************************************************************************/
/**
	(Windows) Config dialog Window Procedure function
*/
LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned short EDITBOXS[4] = { IDC_EDIT1,IDC_EDIT2,IDC_EDIT3,IDC_EDIT4 };
	unsigned short INSERTBTN[4] = { ID_INSERT1,ID_INSERT2,ID_INSERT3,ID_INSERT4 };
	unsigned short REMOVEBTN[4] = { ID_REMOVE1,ID_REMOVE2,ID_REMOVE3,ID_REMOVE4 };
	unsigned short CONFIGBTN[4] = { ID_CONFIG1,ID_CONFIG2,ID_CONFIG3,ID_CONFIG4 };
	char ConfigText[1024] = "";

	switch (message)
	{
	case WM_INITDIALOG:
		for (int Slot = 0; Slot < MAXPAX; Slot++)
		{
			SendDlgItemMessage(hDlg, EDITBOXS[Slot], WM_SETTEXT, 5, (LPARAM)(LPCSTR)g_Paks[Slot].label);
		}
		SendDlgItemMessage(hDlg, IDC_PAKSELECT, TBM_SETRANGE, TRUE, MAKELONG(0, 3));
		SendDlgItemMessage(hDlg, IDC_PAKSELECT, TBM_SETPOS, TRUE, SwitchSlot);
		ReadModuleParms(SwitchSlot, ConfigText);
		SendDlgItemMessage(hDlg, IDC_MODINFO, WM_SETTEXT, strlen(ConfigText), (LPARAM)(LPCSTR)ConfigText);

		return TRUE;
		break;

	case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		return TRUE;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			WriteConfig();
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
			break;
		} //End switch LOWORD

		for (int Slot = 0; Slot < MAXPAX; Slot++)
		{
			if (LOWORD(wParam) == INSERTBTN[Slot])
			{
				LoadCartDLL(Slot, g_Paks[Slot].path);

				for (int Temp = 0; Temp < MAXPAX; Temp++)
				{
					SendDlgItemMessage(hDlg, EDITBOXS[Temp], WM_SETTEXT, strlen(g_Paks[Slot].label), (LPARAM)(LPCSTR)g_Paks[Slot].label);
				}
			}
		}

		for (int Slot = 0; Slot < MAXPAX; Slot++)
		{
			if (LOWORD(wParam) == REMOVEBTN[Slot])
			{
				UnloadModule(Slot);
				SendDlgItemMessage(hDlg, EDITBOXS[Slot], WM_SETTEXT, strlen(g_Paks[Slot].label), (LPARAM)(LPCSTR)g_Paks[Slot].label);
			}
		}

		for (int Slot = 0; Slot < MAXPAX; Slot++)
		{
			if (LOWORD(wParam) == CONFIGBTN[Slot])
			{
				if (g_Paks[Slot].api.config != NULL)
				{
					(*g_Paks[Slot].api.config)(NULL);
				}
			}
		}
		return TRUE;
		break;

	case WM_HSCROLL:
		SwitchSlot = (unsigned char)SendDlgItemMessage(hDlg, IDC_PAKSELECT, TBM_GETPOS, (WPARAM)0, (WPARAM)0);
		SpareSelectSlot = SwitchSlot;
		ChipSelectSlot = SwitchSlot;
		ReadModuleParms(SwitchSlot, ConfigText);
		SendDlgItemMessage(hDlg, IDC_MODINFO, WM_SETTEXT, strlen(ConfigText), (LPARAM)(LPCSTR)ConfigText);

		vccSetCart(0);
		if (vccPakWantsCart(&g_Paks[ChipSelectSlot]))
		{
			vccSetCart(1);
		}
		break;

	default:
		break;
	}
	return FALSE;
}

/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/**
	Main entry point for the DLL (Windows)
*/
BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpReserved)  // reserved
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_hinstDLL = hinstDLL;
		memset(g_Paks, 0, sizeof(g_Paks));
		break;

	case DLL_PROCESS_DETACH:
		// save configuration
		WriteConfig();

		// clean up each pak
		for (int Temp = 0; Temp < MAXPAX; Temp++)
		{
			UnloadModule(Temp);
		}
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;
	}

	return(1);
}

/****************************************************************************/
