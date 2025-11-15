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
#include "ramdisk_cartridge.h"
#include "vcc/bus/cartridge_factory.h"
#include <memory>
#include <Windows.h>


static HINSTANCE gModuleInstance;


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		gModuleInstance = hinstDLL;
	}

	return true;
}


//
extern "C" __declspec(dllexport) CreatePakFactoryFunction GetPakFactory()
{
	return []([[maybe_unused]] std::unique_ptr<::vcc::bus::cartridge_context> context,
			  [[maybe_unused]] const cartridge_capi_context& capi_context) -> std::unique_ptr<::vcc::bus::cartridge>
		{
			return std::make_unique<ramdisk_cartridge>(gModuleInstance);
		};
}

static_assert(
	std::is_same_v<decltype(&GetPakFactory), GetPakFactoryFunction>,
	"RamDisk GetPakFactory does not have the correct signature.");
