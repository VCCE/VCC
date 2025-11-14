////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute itand/or
//	modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your
//	option) any later version.
//	
//	VCC (Virtual Color Computer) is distributed in the hope that it will be
//	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
//	Public License for more details.
//	
//	You should have received a copy of the GNU General Public License along with
//	VCC (Virtual Color Computer). If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////
#include "resource.h"
#include <Windows.h>
#include <vcc/utils/FileOps.h>
#include <vcc/utils/winapi.h>
#include <vcc/utils/filesystem.h>
#include <vcc/bus/cartridge_capi.h>


namespace
{

	using context_type = cartridge_capi_context;

	void* gHostKeyPtr = nullptr;
	HINSTANCE gModuleInstance = nullptr;
	cartridge_capi_context* context_ = nullptr;
	unsigned char left_channel_buffer_ = 0;
	unsigned char right_channel_buffer_ = 0;

	unsigned char Rom[8192];


	bool LoadExtRom(const std::string& filename)	//Returns 1 on if loaded
	{
		FILE *rom_handle = nullptr;
		unsigned short index = 0;
		bool RetVal = false;

		rom_handle = fopen(filename.c_str(), "rb");
		if (rom_handle == nullptr)
		{
			memset(Rom, 0xFF, 8192);
		}
		else
		{
			while ((feof(rom_handle) == 0) & (index < 8192))
			{
				Rom[index++] = fgetc(rom_handle);
			}

			RetVal = true;
			fclose(rom_handle);
		}
		return RetVal;
	}

}



BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		gModuleInstance = hinstDLL;
	}

	return TRUE;
}


extern "C"
{

	__declspec(dllexport) void PakInitialize(
		void* const host_key,
		[[maybe_unused]] const char* const configuration_path,
		[[maybe_unused]] const cartridge_capi_context* const context)
	{
		gHostKeyPtr = host_key;
	}

	__declspec(dllexport) const char* PakGetName()
	{
		static const auto name(::vcc::utils::load_string(gModuleInstance, IDS_MODULE_NAME));
		return name.c_str();
	}

	__declspec(dllexport) const char* PakGetCatalogId()
	{
		static const auto catalog_id(::vcc::utils::load_string(gModuleInstance, IDS_CATNUMBER));
		return catalog_id.c_str();
	}

	__declspec(dllexport) const char* PakGetDescription()
	{
		static const auto description(::vcc::utils::load_string(gModuleInstance, IDS_DESCRIPTION));
		return description.c_str();
	}

	__declspec(dllexport) void PakReset()
	{
		using ::vcc::utils::get_module_path;
		using ::vcc::utils::get_directory_from_path;

		if (LoadExtRom(get_directory_from_path(get_module_path(gModuleInstance)) + "ORCH90.ROM"))
		{
			context_->assert_cartridge_line(gHostKeyPtr, true);
		}

	}

	__declspec(dllexport) void PakWritePort(unsigned char Port, unsigned char Data)
	{
		switch (Port)
		{
		case 0x7A:
			right_channel_buffer_ = Data;
			break;

		case 0x7B:
			left_channel_buffer_ = Data;
			break;
		}
	}

	__declspec(dllexport) unsigned char PakReadMemoryByte(unsigned short address)
	{
		return Rom[address & 8191];
	}

	__declspec(dllexport) unsigned short PakSampleAudio()
	{
		return (left_channel_buffer_ << 8) | right_channel_buffer_;
	}

}
