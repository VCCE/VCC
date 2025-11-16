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
#include "../resource/resource.h"
#include "vcc/bus/cartridges/rom_cartridge.h"
#include "vcc/bus/cartridge_factory.h"
#include "vcc/utils/cartridge_loader.h"
#include "vcc/utils/winapi.h"
#include "vcc/utils/filesystem.h"
#include <vector>
#include <fstream>
#include <iterator>
#include <map>


extern HINSTANCE gModuleInstance;

namespace vcc::utils
{

	namespace
	{

		std::string extract_filename(std::string name)
		{
			name = name.substr(name.find_last_of("/\\") + 1);
			
			if (const auto end_index(name.find_last_of('.')); end_index > 0)
			{
				name = name.substr(0, end_index);
			}

			return name;
		}

	}

	cartridge_file_type determine_cartridge_type(const std::string& filename)
	{
		std::ifstream input(filename, std::ios::binary);
		if (!input.is_open())
		{
			return cartridge_file_type::not_opened;
		}

		if (input.get() == 'M' && input.get() == 'Z')
		{
			return cartridge_file_type::library;
		}

		return cartridge_file_type::rom_image;
	}

	cartridge_loader_result load_rom_cartridge(
		const std::string& filename,
		std::unique_ptr<::vcc::bus::expansion_port_host> host,
		std::unique_ptr<::vcc::bus::expansion_port_ui> ui,
		std::unique_ptr<::vcc::bus::expansion_port_bus> bus)
	{
		if (bus == nullptr)
		{
			throw std::invalid_argument("Cannot load ROM cartridge. Context is null.");
		}

		auto rom_image(load_file_to_vector(filename));
		if (!rom_image.has_value())
		{
			return { nullptr, nullptr, cartridge_loader_status::cannot_open };
		}

		if (rom_image->empty())
		{
			return { nullptr, nullptr, cartridge_loader_status::not_rom };
		}

		// Force enable bank switching since we don't have a way to detect if
		// it should be enabled.   (CCC file > 16KB?)
		constexpr bool enable_bank_switching = true;

		auto rom_cartridge(
			std::make_unique<::vcc::bus::cartridges::rom_cartridge>(
				move(bus),
				extract_filename(filename),
				"",
				move(*rom_image),
				enable_bank_switching));
		return {
			nullptr,
			move(rom_cartridge),
			cartridge_loader_status::success
		};
	}

	cartridge_loader_result load_library_cartridge(
		const std::string& filename,
		std::unique_ptr<::vcc::bus::expansion_port_host> host,
		std::unique_ptr<::vcc::bus::expansion_port_ui> ui,
		std::unique_ptr<::vcc::bus::expansion_port_bus> bus)
	{
		if (bus == nullptr)
		{
			throw std::invalid_argument("Cannot load library cartridge. Context is null.");
		}

		if (GetModuleHandle(filename.c_str()) != nullptr)
		{
			return { nullptr, nullptr, cartridge_loader_status::already_loaded };
		}

		cartridge_loader_result details;
		details.handle.reset(LoadLibrary(filename.c_str()));
		DLOG_C("pak:LoadLibrary %s %d\n", filename.c_str(), GetLastError());
		if (details.handle == nullptr)
		{
			return { nullptr, nullptr, cartridge_loader_status::cannot_open };
		}

		if (GetProcAddress(details.handle.get(), "ModuleName") != nullptr)
		{
			return { nullptr, nullptr, cartridge_loader_status::unsupported_api };
		}

		const auto factoryAccessor(reinterpret_cast<::vcc::bus::create_cartridge_factory_prototype>(GetProcAddress(
			details.handle.get(),
			"GetPakFactory")));
		if (factoryAccessor != nullptr)
		{
			details.cartridge = factoryAccessor()(move(host), move(ui), move(bus));
			details.load_result = cartridge_loader_status::success;

			return details;
		}

		return { nullptr, nullptr, cartridge_loader_status::not_expansion };
	}

	cartridge_loader_result load_cartridge(
		const std::string& filename,
		std::unique_ptr<::vcc::bus::expansion_port_host> host,
		std::unique_ptr<::vcc::bus::expansion_port_ui> ui,
		std::unique_ptr<::vcc::bus::expansion_port_bus> bus)
	{
		if (bus == nullptr)
		{
			throw std::invalid_argument("Cannot load cartridge. Context is null.");
		}

		switch (::vcc::utils::determine_cartridge_type(filename))
		{
		default:
		case cartridge_file_type::not_opened:	//	File doesn't exist or can't be opened
			return { nullptr, nullptr, cartridge_loader_status::cannot_open };

		case cartridge_file_type::rom_image:	//	File is a ROM image
			return load_rom_cartridge(filename, move(host), move(ui), move(bus));

		case cartridge_file_type::library:		//	File is a DLL
			return load_library_cartridge(filename, move(host), move(ui), move(bus));
		}
	}

	std::string load_error_string(cartridge_loader_status status)
	{
		static const std::map<cartridge_loader_status, UINT> string_id_map = {
			{ cartridge_loader_status::already_loaded, IDS_ERROR_CARTRIDGE_ALREADY_LOADED},
			{ cartridge_loader_status::cannot_open, IDS_ERROR_CARTRIDGE_CANNOT_OPEN},
			{ cartridge_loader_status::not_found, IDS_ERROR_CARTRIDGE_NOT_FOUND },
			{ cartridge_loader_status::unsupported_api, IDS_ERROR_CARTRIDGE_API_NOT_SUPPORTED },
			{ cartridge_loader_status::not_loaded, IDS_ERROR_CARTRIDGE_NOT_LOADED },
			{ cartridge_loader_status::not_rom, IDS_ERROR_CARTRIDGE_NOT_ROM },
			{ cartridge_loader_status::not_expansion, IDS_ERROR_CARTRIDGE_NOT_EXPANSION }
		};

		const auto string_id_ptr(string_id_map.find(status));

		return vcc::utils::load_string(
			gModuleInstance,
			string_id_ptr != string_id_map.end() ? string_id_ptr->second : IDS_ERROR_UNKNOWN);
	}

}
