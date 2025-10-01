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
#include "../ModuleDefs.h"
#include "resource.h" 
#include "defines.h"
#include "memboard.h"

static CARTMENUCALLBACK DynamicMenuCallback = nullptr;


static HINSTANCE g_hinstDLL;

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
	if (fdwReason == DLL_PROCESS_DETACH ) //Clean Up 
	{
		return 1;
	}
	g_hinstDLL=hinstDLL;
	return 1;
}

extern "C" 
{          
	__declspec(dllexport) void ModuleName(char *ModName,char *CatNumber,CARTMENUCALLBACK Temp)
	{
		LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
		LoadString(g_hinstDLL,IDS_CATNUMBER,CatNumber, MAX_LOADSTRING);
		InitMemBoard();
		DynamicMenuCallback =Temp;
		return ;
	}
}


extern "C" 
{         
	__declspec(dllexport) void PackPortWrite(unsigned char Port,unsigned char Data)
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
	__declspec(dllexport) unsigned char PackPortRead(unsigned char Port)
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
