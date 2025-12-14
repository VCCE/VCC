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
#include "memboard.h"
#include "defines.h"
#include "resource.h" 
#include <vcc/bus/legacy_cartridge_definitions.h>
#include <Windows.h>

static HINSTANCE gModuleInstance;


BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
	if (fdwReason == DLL_PROCESS_ATTACH) //Clean Up 
	{
		gModuleInstance = hinstDLL;
	}

	return TRUE;
}

extern "C" 
{          

	__declspec(dllexport) const char* PakGetName()
	{
		static char string_buffer[MAX_LOADSTRING];

		LoadString(gModuleInstance, IDS_MODULE_NAME, string_buffer, MAX_LOADSTRING);

		return string_buffer;
	}

	__declspec(dllexport) const char* PakGetCatalogId()
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
		void* const /*host_key*/,
		const char* const /*configuration_path*/,
		HWND hVccWnd,
		const cpak_callbacks* const /*context*/)
	{
		InitMemBoard();
	}

}


extern "C" 
{         
	__declspec(dllexport) void PakWritePort(unsigned char Port,unsigned char Data)
	{
		switch (Port)
		{
		    // Address of RAM disk to read/write 
			case 0x40:
			case 0x41:
			case 0x42:
				WritePort(Port,Data);
				return;
            // Write to RAM disk
			case 0x43:
				WriteArray(Data);
				return;

			default:
				return;
		}	//End port switch		
	}
}


extern "C"
{
	__declspec(dllexport) unsigned char PakReadPort(unsigned char Port)
	{
		switch (Port)
		{
			// Read from RAM disk
			case 0x43:
				return(ReadArray());

			default:
				return 0;
		}	//End port switch
	}
}
