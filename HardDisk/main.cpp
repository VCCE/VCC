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


BOOL WINAPI DllMain(
	HINSTANCE module_instance,
	DWORD call_reason,
	[[maybe_unused]] LPVOID reserved)
{
	if (call_reason == DLL_PROCESS_ATTACH)
	{
		gModuleInstance = module_instance;
	}

	return TRUE;
}

extern "C" __declspec(dllexport) ::vcc::bus::cartridge_factory_prototype GetPakFactory()
{
	return [](
		std::shared_ptr<::vcc::bus::expansion_port_host> host,
		std::unique_ptr<::vcc::bus::expansion_port_ui> ui,
		std::unique_ptr<::vcc::bus::expansion_port_bus> bus) -> ::vcc::bus::cartridge_factory_result
		{
			return std::make_unique<vcc_hard_disk_cartridge>(move(host), move(ui), move(bus), gModuleInstance);
		};
}

static_assert(
	std::is_same_v<decltype(&GetPakFactory), ::vcc::bus::create_cartridge_factory_prototype>,
	"Virtual Hard Disk GetPakFactory does not have the correct signature.");
