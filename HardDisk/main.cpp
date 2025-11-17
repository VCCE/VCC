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
#include "harddisk.h"
#include <Windows.h>
#include <stdio.h>
#include "resource.h" 
#include "vcc/utils/FileOps.h"
#include "vcc/utils/winapi.h"
#include "vcc/utils/filesystem.h"
#include "vcc/bus/cartridge_factory.h"
#include <memory>


HINSTANCE gModuleInstance = nullptr;


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		gModuleInstance = hinstDLL;
	}

	return TRUE;
}

extern "C" __declspec(dllexport) ::vcc::bus::CreatePakFactoryFunction GetPakFactory()
{
	return [](
		std::unique_ptr<::vcc::bus::expansion_bus> bus) -> std::unique_ptr<::vcc::bus::cartridge>
		{
			return std::make_unique<vcc_hard_disk_cartridge>(move(bus), gModuleInstance);
		};
}

static_assert(
	std::is_same_v<decltype(&GetPakFactory), ::vcc::bus::GetPakFactoryFunction>,
	"Virtual Hard Disk GetPakFactory does not have the correct signature.");
