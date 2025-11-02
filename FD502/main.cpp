////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute it and/or
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
#include "fd502_cartridge.h"
#include "vcc/bus/cartridge_factory.h"


HINSTANCE gModuleInstance;


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


extern "C" __declspec(dllexport) ::vcc::bus::cartridge_plugin_factory_prototype GetCartridgePluginFactory()
{
	return [](
		std::shared_ptr<::vcc::bus::expansion_port_host> host,
		std::unique_ptr<::vcc::bus::expansion_port_ui> ui,
		std::unique_ptr<::vcc::bus::expansion_port_bus> bus) -> ::vcc::bus::cartridge_factory_result
		{
			auto configuration(std::make_unique<::vcc::cartridges::fd502::fd502_configuration>(
				host->configuration_path(),
				"Cartridges.FD502"));

			std::shared_ptr<::vcc::bus::expansion_port_bus> shared_bus(move(bus));
			auto driver(std::make_unique<::vcc::cartridges::fd502::fd502_cartridge_driver>(host, shared_bus));

			return std::make_unique<::vcc::cartridges::fd502::fd502_cartridge>(
				move(host),
				move(ui),
				shared_bus,
				move(driver),
				move(configuration),
				gModuleInstance);
		};
}

static_assert(
	std::is_same_v<decltype(&GetCartridgePluginFactory), ::vcc::bus::get_cartridge_plugin_factory_prototype>,
	"FD502 GetCartridgePluginFactory does not have the correct signature.");

