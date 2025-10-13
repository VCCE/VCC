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
#include <vcc/core/cartridge_loader.h>
#include <vcc/core/cartridges/rom_cartridge.h>
#include <vcc/core/cartridges/legacy_cartridge.h>
#include <vector>
#include <fstream>
#include <iterator>


namespace vcc { namespace core
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
		const cartridge_loader_context& context,
		const std::string& filename)
	{
		constexpr size_t PAK_MAX_MEM = 0x40000;

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
		// it should be enabled.
		constexpr bool enable_bank_switching = true;

		return {
			nullptr,
			std::make_unique<vcc::core::cartridges::rom_cartridge>(
				context.assertCartridgeLine,
				extract_filename(filename),
				"",
				move(romImage),
				enable_bank_switching),
			cartridge_loader_status::success
		};
	}

	cartridge_loader_result load_legacy_cartridge(
		const cartridge_loader_context& context,
		const std::string& filename,
		const std::string& iniPath)
	{
		if (GetModuleHandle(filename.c_str()) != nullptr)
		{
			return { nullptr, nullptr, cartridge_loader_status::already_loaded };
		}

		cartridge_loader_result details;
		details.handle.reset(LoadLibrary(filename.c_str()));
		_DLOG("pak:LoadLibrary %s %d\n", filename.c_str(), loadedModule);
		if (details.handle == nullptr)
		{
			return { nullptr, nullptr, cartridge_loader_status::cannot_open };
		}

		if (GetProcAddress(details.handle.get(), "ModuleName") == nullptr)
		{
			return { nullptr, nullptr, cartridge_loader_status::not_expansion };
		}

		details.cartridge = std::make_unique<vcc::core::cartridges::legacy_cartridge>(
			details.handle.get(),
			iniPath,
			context.addMenuItemCallback,
			context.readData,
			context.writeData,
			context.assertInterrupt,
			context.assertCartridgeLine);
		details.load_result = cartridge_loader_status::success;

		return details;
	}

	cartridge_loader_result load_cartridge(
		const cartridge_loader_context& context,
		const std::string& filename,
		const std::string& iniPath)
	{
		switch (vcc::core::determine_cartridge_type(filename))
		{
		default:
		case cartridge_file_type::not_opened:	//	File doesn't exist or can't be opened
			return { nullptr, nullptr, cartridge_loader_status::cannot_open };

		case cartridge_file_type::rom_image:	//	File is a ROM image
			return vcc::core::load_rom_cartridge(context, filename);

		case cartridge_file_type::library:		//	File is a DLL
			return vcc::core::load_legacy_cartridge(context, filename, iniPath);
		}
	}

} }
