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
	Disto RamPak - up to 16MB

	dmode /rp sct=40 t0s=40 cyl=3f0 vfy=0 drv=1

	drv = MPI slot - 1
	vfy = verify is not really needed

	Note: cyl=400 should work but frequently (not always) it
	produces an error #0000 in NitrOS9 3.3.0 when trying to format
	3f0 seems to work always.  need to investigate further

	TODO: save contents of RamPak on exit and re-load on startup?
	TODO: add config 
			- flag to save/restore as if it was non-volatile
			- set mem size
			- set base address
*/
/****************************************************************************/

#include "memboard.h"
#include "defines.h"

#include "../../vcc/vccPakAPI.h"

#include <windows.h>
#include "resource.h" 

/****************************************************************************/

static HINSTANCE	g_hinstDLL	= NULL;
static HWND			g_hWnd		= NULL;
static int			g_id = 0;

//static vccapi_dynamicmenucallback_t DynamicMenuCallback = NULL;

static int			ScanCount	= 0;
static int			LastSector	= 0;

/****************************************************************************/

extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_INIT(int id, void * wndHandle, vccapi_dynamicmenucallback_t Temp)
{
	g_id = id;
	g_hWnd = (HWND)wndHandle;

	//DynamicMenuCallback = Temp;

	InitMemBoard();
}

/****************************************************************************/

extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_GETNAME(char * ModName, char * CatNumber)
{
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	LoadString(g_hinstDLL,IDS_CATNUMBER,CatNumber, MAX_LOADSTRING);
}

/****************************************************************************/
/**
	Return the status of the Pak

	we can return a short string
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_STATUS(char * buffer, size_t bufferSize)
{
	int	sector = (((int)g_Address >> 8) & 0x00FF) | (((int)g_Block << 8) & 0xFF00);

	strcpy(buffer, "RamPak: ");
	strcat(buffer, DStatus);

	ScanCount++;
	if ( sector != LastSector )
	{
		ScanCount = 0;
		LastSector = sector;
	}

	// some arbitrary value of loops through here
	// before we indicate as Idle
	if ( ScanCount > 63 )
	{
		ScanCount = 0;
		strcpy(DStatus, "Idle");
	}
}

/****************************************************************************/
/**
	Write a byte to our i/o ports
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_PORTWRITE(unsigned char Port, unsigned char Data)
{
	switch (Port)
	{
		case BASE_PORT + 0:
		case BASE_PORT + 1:
		case BASE_PORT + 2:
			WritePort(Port,Data);
			return;
		break;

		case BASE_PORT + 3:
			WriteArray(Data);
			return;
		break;

		default:
			return;
		break;
	}	//End port switch		
}

/****************************************************************************/
/**
	Read a byte from our i/o ports
*/
extern "C" __declspec(dllexport) unsigned char VCC_PAKAPI_DEF_PORTREAD(unsigned char Port)
{
	switch (Port)
	{
		case BASE_PORT + 3:
			return ReadArray();
		break;

		default:
			return(0);
		break;
	}	//End port switch
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
//static vccpakapi_dynmenubuild_t	__dynMenuBuild		= VCC_PAKAPI_DEF_DYNMENUBUILD;
//static vccpakapi_config_t			__config			= VCC_PAKAPI_DEF_CONFIG;
//static vccpakapi_heartbeat_t		__heartbeat			= VCC_PAKAPI_DEF_HEARTBEAT;
static vccpakapi_status_t			__status			= VCC_PAKAPI_DEF_STATUS;
//static vccpakapi_getaudiosample_t	__getAudioSample	= VCC_PAKAPI_DEF_AUDIOSAMPLE;
//static vccpakapi_reset_t			__reset				= VCC_PAKAPI_DEF_RESET;
static vccpakapi_portread_t			__portRead			= VCC_PAKAPI_DEF_PORTREAD;
static vccpakapi_portwrite_t		__portWrite			= VCC_PAKAPI_DEF_PORTWRITE;
//static vcccpu_read8_t				__memRead			= VCC_PAKAPI_DEF_MEMREAD;
//static vcccpu_write8_t			__memWrite			= VCC_PAKAPI_DEF_MEMWRITE;
//static vccpakapi_setmemptrs_t		__memPointers		= VCC_PAKAPI_DEF_MEMPOINTERS;
//static vccpakapi_setcartptr_t		__setCartPtr		= VCC_PAKAPI_DEF_SETCART;
//static vccpakapi_setintptr_t		__assertInterrupt	= VCC_PAKAPI_DEF_ASSERTINTERRUPT;
//static vccpakapi_setinipath_t		__setINIPath		= VCC_PAKAPI_DEF_SETINIPATH;

#endif // _DEBUG

/****************************************************************************/
/****************************************************************************/

/*
	DLL Main entry point (Windows)
*/
BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpReserved)  // reserved
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			// one-time init
			g_hinstDLL = hinstDLL;
		}
		break;

		case DLL_PROCESS_DETACH:
		{
			// one time destruction
			DestroyMemBoard();
		}
		break;
	}

	return(1);
}

/****************************************************************************/
