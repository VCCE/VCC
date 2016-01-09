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

#include "defines.h"
#include "..\fileops.h"
#include "..\vccPakAPI.h"

#include <stdio.h>
#include "resource.h" 

/****************************************************************************/

static HINSTANCE g_hinstDLL=NULL;
static unsigned char LeftChannel=0,RightChannel=0;

static vccapi_setcart_t PakSetCart	= NULL;

static unsigned char Rom[8192];

/****************************************************************************/

unsigned char LoadExtRom(char *FilePath)	//Returns 1 on if loaded
{

	FILE *rom_handle = NULL;
	unsigned short index = 0;
	unsigned char RetVal = 0;

	rom_handle = fopen(FilePath, "rb");
	if (rom_handle == NULL)
		memset(Rom, 0xFF, 8192);
	else
	{
		while ((feof(rom_handle) == 0) & (index<8192))
			Rom[index++] = fgetc(rom_handle);
		RetVal = 1;
		fclose(rom_handle);
	}

	return(RetVal);
}

/****************************************************************************/
/****************************************************************************/
/*
	VCC Pak API
*/
/****************************************************************************/

extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_GETNAME(char * ModName, char * CatNumber, vccapi_dynamicmenucallback_t Temp, void * wndHandle)
{
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	LoadString(g_hinstDLL,IDS_CATNUMBER,CatNumber, MAX_LOADSTRING);		
	strcpy(ModName,"Orchestra-90");
}

extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_PORTWRITE(unsigned char Port,unsigned char Data)
{
	switch (Port)
	{
	case 0x7A:
		RightChannel=Data;			
		break;

	case 0x7B:
		LeftChannel=Data;
		break;
	}
	return;
}

extern "C" __declspec(dllexport) unsigned char VCC_PAKAPI_DEF_PORTREAD(unsigned char Port)
{
	return(NULL);
}

extern "C" __declspec(dllexport) unsigned char VCC_PAKAPI_DEF_RESET(void)
{
	char RomPath[MAX_PATH];

	memset(Rom, 0xff, 8192);
	GetModuleFileName(NULL, RomPath, MAX_PATH);
	PathRemoveFileSpec(RomPath);
	strcpy(RomPath, "ORCH90.ROM");
		
	if ((PakSetCart!=NULL) & LoadExtRom(RomPath))	//If we can load the rom them assert cart 
		PakSetCart(1);

	return(NULL);
}

extern "C" __declspec(dllexport) unsigned char VCC_PAKAPI_DEF_SETCART(vccapi_setcart_t Pointer)
{
	PakSetCart=Pointer;

	return(NULL);
}

extern "C" __declspec(dllexport) unsigned char VCC_PAKAPI_DEF_MEMREAD(unsigned short Address)
{
	return(Rom[Address & 8191]);
}

// This gets called at the end of every scan line 262 Lines * 60 Frames = 15780 Hz 15720
extern "C" __declspec(dllexport) unsigned short VCC_PAKAPI_DEF_AUDIOSAMPLE(void)
{
	return((LeftChannel<<8) | RightChannel) ;
}

/****************************************************************************/

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpReserved)  // reserved
{
	if (fdwReason == DLL_PROCESS_DETACH) //Clean Up 
	{
		//Put shutdown procs here
		return(1);
	}
	g_hinstDLL = hinstDLL;
	return(1);
}

/****************************************************************************/
