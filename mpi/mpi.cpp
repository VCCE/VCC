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
#include "multipak_cartridge.h"
#include "vcc/bus/cartridge_factory.h"
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
extern "C" __declspec(dllexport) ::vcc::bus::cartridge_factory_prototype GetPakFactory()
{
	return [](
		std::unique_ptr<::vcc::bus::expansion_port_host> host,
		std::unique_ptr<::vcc::bus::expansion_port_ui> ui,
		std::unique_ptr<::vcc::bus::expansion_port_bus> bus) -> ::vcc::bus::cartridge_factory_result
		{
			gConfiguration.configuration_path(host->configuration_path());

			return std::make_unique<multipak_cartridge>(move(host), move(ui), move(bus), gModuleInstance, gConfiguration);
		};

}

static_assert(
	std::is_same_v<decltype(&GetPakFactory), ::vcc::bus::create_cartridge_factory_prototype>,
	"MPI GetPakFactory does not have the correct signature.");
