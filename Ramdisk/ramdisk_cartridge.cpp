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
#include <vcc/utils/winapi.h>
#include <vcc/bus/cartridge_capi.h>
#include <array>


extern HINSTANCE gModuleInstance;

namespace
{
	using address_type = ::std::size_t;

	address_type current_address_ = 0;
	address_type address_byte0 = 0;
	address_type address_byte1 = 0;
	address_type address_byte2 = 0;
	std::array<unsigned char, 1024u * 512u> ram_;

	void initialize_state(bool initialize_memory)
	{
		current_address_ = 0;
		address_byte0 = 0;
		address_byte1 = 0;
		address_byte2 = 0;
		if (initialize_memory)
		{
			std::fill(ram_.begin(), ram_.end(), 0xff);
		}
	}
}

HINSTANCE gModuleInstance;


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		gModuleInstance = hinstDLL;
	}

	return true;
}


extern "C"
{

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


	__declspec(dllexport) void PakInitialize(
		void* const host_key,
		const char* const configuration_path,
		const cartridge_capi_context* const context)
	{
		initialize_state(true);
	}

	__declspec(dllexport) void PakReset()
	{
		initialize_state(false);
	}

	__declspec(dllexport) void PakWritePort(unsigned char port_id, unsigned char value)
	{
		switch (port_id)
		{
		case 0x40:
			address_byte0 = value;
			break;

		case 0x41:
			address_byte1 = value;
			break;

		case 0x42:
			address_byte2 = (value & 0x7);
			break;

		case 0x43:
			ram_[current_address_] = value;
			return;

		default:
			return;
		}

		current_address_ = (address_byte2 << 16) | (address_byte1 << 8) | address_byte0;
	}

	__declspec(dllexport) unsigned char PakReadPort(unsigned char port_id)
	{
		if (port_id == 0x43)
		{
			return ram_[current_address_];
		}

		return 0;
	}

}
