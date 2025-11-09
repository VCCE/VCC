////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	This is an expansion module for the Vcc Emulator. It simulated the functions
//	of the TRS-80 Multi-Pak Interface.
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
#include "multipak_cartridge.h"
#include <vcc/bus/cartridge_factory.h>
#include <Windows.h>


namespace
{

	HINSTANCE gModuleInstance = nullptr;
	multipak_configuration gConfiguration("MPI");

}


BOOL WINAPI DllMain(HINSTANCE module_instance, DWORD reason, [[maybe_unused]] LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		gModuleInstance = module_instance;
	}

	return TRUE;
}

//
extern "C" __declspec(dllexport) CreatePakFactoryFunction GetPakFactory()
{
	return [](
		std::unique_ptr<::vcc::bus::cartridge_context> context,
		const cartridge_capi_context& capi_context) -> std::unique_ptr<::vcc::bus::cartridge>
	{
		gConfiguration.configuration_path(context->configuration_path());

		return std::make_unique<multipak_cartridge>(gModuleInstance, gConfiguration, move(context), capi_context);
	};

}
