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
#include <vcc/cartridges/rom_cartridge.h>
#include <vcc/cartridges/capi_adapter_cartridge.h>
#include <vcc/utils/cartridge_loader.h>
#include <vcc/utils/winapi.h>
#include <vcc/core/cartridge_factory.h>
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
			const auto endIndex = name.find_last_of('.');
			if (endIndex > 0)
			{
				name = name.substr(0, endIndex);
			}

			return name;
		}

	}

	// Look for magic "MZ" to detect DLL.  Assume rom if not.
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

	// Load rom cartridge
	cartridge_loader_result load_rom_cartridge(
		const std::string& filename,
		std::unique_ptr<::vcc::core::cartridge_context> context)
	{
		constexpr size_t PAK_MAX_MEM = 0x40000;   // 256KB

		std::vector<uint8_t> romImage;

		romImage.reserve(PAK_MAX_MEM);

		// Open the ROM file, fail if unable to
		std::ifstream input(filename, std::ios::binary);
		if (!input.is_open())
		{
			return { nullptr, nullptr, cartridge_loader_status::cannot_open };
		}

		// Load the file
		input.unsetf(std::ios::skipws);
		std::copy(
			std::istream_iterator<uint8_t>(input),
			std::istream_iterator<uint8_t>(),
			back_inserter(romImage));

		if (romImage.empty())
		{
			return { nullptr, nullptr, cartridge_loader_status::not_rom };
		}

		// Force enable bank switching since we don't have a way to detect if
		// it should be enabled.   (CCC file > 16KB?)
		constexpr bool enable_bank_switching = true;

		return {
			nullptr,
			std::make_unique<vcc::cartridges::rom_cartridge>(
				move(context),
				extract_filename(filename),
				"",
				move(romImage),
				enable_bank_switching),
			cartridge_loader_status::success
		};
	}

	// Load C API hardware cart
	cartridge_loader_result load_capi_cartridge(
		const std::string& filename,
		std::unique_ptr<::vcc::core::cartridge_context> cartridge_context,
		void* const host_context,
		const std::string& iniPath,
		const cartridge_capi_context& capi_context)
	{
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

		const auto factoryAccessor(reinterpret_cast<GetPakFactoryFunction>(GetProcAddress(
			details.handle.get(),
			"GetPakFactory")));
		if (factoryAccessor != nullptr)
		{
			details.cartridge = factoryAccessor()(move(cartridge_context), capi_context);
			details.load_result = cartridge_loader_status::success;

			return details;
		}

		if (GetProcAddress(details.handle.get(), "PakInitialize") != nullptr)
		{
			details.cartridge = std::make_unique<vcc::cartridges::capi_adapter_cartridge>(
				details.handle.get(),
				host_context,
				iniPath,
				capi_context);
			details.load_result = cartridge_loader_status::success;

			return details;
		}

		return { nullptr, nullptr, cartridge_loader_status::not_expansion };
	}

	cartridge_loader_result load_cartridge(
		const std::string& filename,
		std::unique_ptr<::vcc::core::cartridge_context> cartridge_context,
		const cartridge_capi_context& capi_context,
		void* const host_context,
		const std::string& iniPath)
	{
		switch (::vcc::utils::determine_cartridge_type(filename))
		{
		default:
		case cartridge_file_type::not_opened:	//	File doesn't exist or can't be opened
			return { nullptr, nullptr, cartridge_loader_status::cannot_open };

		case cartridge_file_type::rom_image:	//	File is a ROM image
			return load_rom_cartridge(filename, move(cartridge_context));

		case cartridge_file_type::library:		//	File is a DLL
			return load_capi_cartridge(
				filename,
				move(cartridge_context),
				host_context,
				iniPath,
				capi_context);
		}
	}

	::std::string load_error_string(cartridge_loader_status status)
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
