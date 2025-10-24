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
#include <Windows.h>
#include <stdio.h>
#include "resource.h" 
#include <vcc/common/FileOps.h>
#include <vcc/core/legacy_cartridge_definitions.h>
#include <vcc/core/limits.h>

static HINSTANCE gModuleInstance;
static void* gHostKey = nullptr;
static PakAssertCartridgeLineHostCallback PakSetCart = nullptr;
static unsigned char LeftChannel=0,RightChannel=0;
unsigned char LoadExtRom(const char *);
static unsigned char Rom[8192];
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
	if (fdwReason == DLL_PROCESS_DETACH ) //Clean Up 
	{
		//Put shutdown procs here
		return 1;
	}
	gModuleInstance = hinstDLL;
	return 1;
}


extern "C" 
{          

	__declspec(dllexport) const char* PakGetName()
	{
		static char string_buffer[MAX_LOADSTRING];

		//LoadString(gModuleInstance, IDS_MODULE_NAME, string_buffer, MAX_LOADSTRING);
		strcpy(string_buffer, "Orchestra-90");

		return string_buffer;
	}

	__declspec(dllexport) const char* PakCatalogName()
	{
		static char string_buffer[MAX_LOADSTRING];

		LoadString(gModuleInstance, IDS_CATNUMBER, string_buffer, MAX_LOADSTRING);

		return string_buffer;
	}

	__declspec(dllexport) const char* PakGetDescription()
	{
		static char string_buffer[MAX_LOADSTRING];

		LoadString(gModuleInstance, IDS_DESCRIPTION, string_buffer, MAX_LOADSTRING);

		return string_buffer;
	}

	__declspec(dllexport) void PakInitialize(
		void* const host_key,
		const char* const /*configuration_path*/,
		const pak_initialization_parameters* const parameters)
	{
		gHostKey = host_key;
		PakSetCart = parameters->assert_cartridge_line;
	}

}


extern "C" 
{         
	__declspec(dllexport) void PakWritePort(unsigned char Port,unsigned char Data)
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
}


extern "C"
{
	__declspec(dllexport) unsigned char PakReadPort(unsigned char /*Port*/)
	{
		return 0;
	}
}

extern "C"
{
	__declspec(dllexport) unsigned char PakReset()
	{
		char RomPath[MAX_PATH];

		memset(Rom, 0xff, 8192);
		GetModuleFileName(nullptr, RomPath, MAX_PATH);
		PathRemoveFileSpec(RomPath);
		strcpy(RomPath, "ORCH90.ROM");
		
		if (LoadExtRom(RomPath))	//If we can load the rom them assert cart 
			PakSetCart(gHostKey, 1);

		return 0;
	}
}


extern "C"
{
	__declspec(dllexport) unsigned char PakReadMemoryByte(unsigned short Address)
	{
		return(Rom[Address & 8191]);
	}
}



// This gets called at the end of every scan line 262 Lines * 60 Frames = 15780 Hz 15720
extern "C" 
{          
	__declspec(dllexport) unsigned short PakSampleAudio()
	{
		
		return((LeftChannel<<8) | RightChannel) ;
	}
}



unsigned char LoadExtRom(const char *FilePath)	//Returns 1 on if loaded
{

	FILE *rom_handle = nullptr;
	unsigned short index = 0;
	unsigned char RetVal = 0;

	rom_handle = fopen(FilePath, "rb");
	if (rom_handle == nullptr)
		memset(Rom, 0xFF, 8192);
	else
	{
		while ((feof(rom_handle) == 0) & (index<8192))
			Rom[index++] = fgetc(rom_handle);
		RetVal = 1;
		fclose(rom_handle);
	}
	return RetVal;
}
