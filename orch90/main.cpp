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
#include "orchestra90cc_cartridge.h"
#include <Windows.h>
#include <stdio.h>
#include "resource.h" 
#include <vcc/common/FileOps.h>
#include <vcc/core/legacy_cartridge_definitions.h>
#include <vcc/core/limits.h>
#include <memory>
#include <vcc/core/utils/winapi.h>
#include <vcc/core/utils/filesystem.h>
#include <vcc/core/cartridge_factory.h>


static HINSTANCE gModuleInstance=nullptr;


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		gModuleInstance = hinstDLL;
	}

	return TRUE;
}

extern "C" __declspec(dllexport) CreatePakFactoryFunction GetPakFactory()
{
	return [](
		std::unique_ptr<::vcc::core::cartridge_context> context,
		[[maybe_unused]] const cpak_cartridge_context& cpak_context) -> std::unique_ptr<::vcc::core::cartridge>
	{
		return std::make_unique<orchestra90cc_cartridge>(gModuleInstance, move(context));
	};
}
