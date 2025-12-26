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

//==========================================================================
// Orch90 is perhaps the simplest pak, it uses PakSampleAudio() to simulate
// two 8-bit digital-to-analog converters.
//==========================================================================

#include <Windows.h>
#include <stdio.h>
#include "resource.h"
#include <vcc/core/FileOps.h>
#include <vcc/bus/cpak_cartridge_definitions.h>
#include <vcc/core/limits.h>
#include <vcc/core/logger.h>

static HINSTANCE gModuleInstance;
static void* gCallbackContext = nullptr;
static PakAssertCartridgeLineHostCallback PakSetCart = nullptr;
static unsigned char LeftChannel=0,RightChannel=0;
static unsigned char Rom[8192];
bool LoadRom();

// DLL exports
extern "C"
{
	__declspec(dllexport) const char* PakGetName()
	{
		static char string_buffer[MAX_LOADSTRING];

		strcpy(string_buffer, "Orchestra-90");

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
		void* const callback_context,
		const char* const /*configuration_path*/,
		HWND hVccWnd,
		const cpak_callbacks* const callbacks)
	{
		DLOG_C("orch90 init\n");
		gCallbackContext = callback_context;
		PakSetCart = callbacks->assert_cartridge_line;
	}

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

	__declspec(dllexport) unsigned char PakReadPort(unsigned char /*Port*/)
	{
		return 0;
	}

	__declspec(dllexport) void PakReset()
	{
		DLOG_C("orch90 reset\n");
		if (LoadRom()) PakSetCart(gCallbackContext, 1);
	}

	__declspec(dllexport) unsigned char PakReadMemoryByte(unsigned short Address)
	{
		return(Rom[Address & 8191]);
	}

	__declspec(dllexport) unsigned short PakSampleAudio()
	{
		//This is called every scan line 262 Lines * 60 Frames = 15780 Hz 15720
		return((LeftChannel<<8) | RightChannel) ;
	}
}

// DLLMain
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		gModuleInstance = hinstDLL;
	return TRUE;
}

//Load orch90.rom
bool LoadRom()
{
	memset(Rom, 0xff, 8192);

	// Assume rom is in same directory as orch90.dll
	char RomPath[MAX_PATH];
	GetModuleFileName(nullptr, RomPath, MAX_PATH);
	PathRemoveFileSpec(RomPath);
	strcpy(RomPath, "ORCH90.ROM");

	FILE *rom_handle = fopen(RomPath, "rb");
	if (rom_handle == nullptr) return false;

	unsigned short index = 0;
	while ((feof(rom_handle) == 0) & (index<8192))
		Rom[index++] = fgetc(rom_handle);
	fclose(rom_handle);
	return true;
}
