//#define USE_LOGGING
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
#include <vcc/bus/cartridge_loader.h>
#include <vcc/bus/rom_cartridge.h>
#include <vcc/bus/cpak_cartridge.h>
#include <vcc/util/logger.h>
#include <vector>
#include <fstream>
#include <iterator>

namespace VCC::Core
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

		if (!input.is_open()) {
			return cartridge_file_type::not_opened;
		}

		// Read two bytes from the start of the file
		unsigned char header[2] = {};
		input.clear();
		input.seekg(0, std::ios::beg);
		input.read(reinterpret_cast<char*>(header), 2);
		if (!input) {
			DLOG_C("core::cartridge is too small\n\n");
			return cartridge_file_type::rom_image;
		}

		// Check for magic 'MZ' DLL indicator
		if (header[0] == 'M' && header[1] == 'Z') {
			DLOG_C("core::cartridge type library\n\n");
			return cartridge_file_type::library;
		}

		DLOG_C("core::cartridge assume rom\n\n");
		return cartridge_file_type::rom_image;
	}

	// Load a ROM cartridge
	cartridge_loader_result load_rom_cartridge(
		std::unique_ptr<cartridge_context> context,
		const std::string& filename)
	{
		constexpr size_t PAK_MAX_MEM = 0x40000;

		std::vector<uint8_t> romImage;

		romImage.reserve(PAK_MAX_MEM);

		// Open the ROM file, fail if unable to
		std::ifstream input(filename, std::ios::binary);
		if (!input.is_open()) {
			return { nullptr, nullptr, cartridge_loader_status::cannot_open };
		}

		// Load the file
		input.unsetf(std::ios::skipws);
		std::copy(
			std::istream_iterator<uint8_t>(input),
			std::istream_iterator<uint8_t>(),
			back_inserter(romImage));

		if (romImage.empty()) {
			return { nullptr, nullptr, cartridge_loader_status::not_rom };
		}

		// Force enable bank switching since we don't have a way to detect if
		// it should be enabled.
		constexpr bool enable_bank_switching = true;

		return {
			nullptr,
			std::make_unique<VCC::Core::rom_cartridge>(
				move(context),
				extract_filename(filename),
				"",
				move(romImage),
				enable_bank_switching),
			cartridge_loader_status::success
		};
	}

	// Load a legacy cartridge
	cartridge_loader_result load_cpak_cartridge(
		const std::string& filename,
		std::unique_ptr<cartridge_context> cartridge_context,
		slot_id_type SlotId,
		const std::string& iniPath,
		HWND hVccWnd,
		const cpak_callbacks& cpak_callbacks)
	{

		if (GetModuleHandle(filename.c_str()) != nullptr)
		{
			return { nullptr, nullptr, cartridge_loader_status::already_loaded };
		}

		cartridge_loader_result details;
		HMODULE hCart = LoadLibrary(filename.c_str());
		details.handle.reset(hCart);

		DLOG_C("pak:LoadLibrary %s %d\n", filename.c_str(), GetLastError());
		if (details.handle == nullptr)
		{
			return { nullptr, nullptr, cartridge_loader_status::cannot_open };
		}

		if (GetProcAddress(details.handle.get(), "PakInitialize") != nullptr)
		{
			details.cartridge = std::make_unique<VCC::Core::cpak_cartridge>(
				details.handle.get(),
				SlotId,
				iniPath,
				hVccWnd,
				cpak_callbacks);
			details.load_result = cartridge_loader_status::success;

			return details;
		}

		return { nullptr, nullptr, cartridge_loader_status::not_expansion };
	}

    // Load a cartridge; either a ROM image or a pak dll. Load cartridge is called
	// by both mpi/multipak_cartridge.cpp and pakinterface.cpp.
	// cartridge_loader_result is defined in libcommon/include/vcc/bus/cartridge_loader.h
	// cartridge_context is defined in libcommon/include/vcc/bus/cartridge_context.h
	// SlotId is size_t 0-4, 0 = boot slot (side slot), 1-4 = MPI slots.  SlotId is
	// passed to cpak cart DLLs and is returned as first argment of callbacks
	// cpak_callbacks is used by all hardware paks and is defined in 
	// libcommon/include/vcc/bus/cpak_cartridge_definitions.h.
	cartridge_loader_result load_cartridge(
		const std::string& filename,
		std::unique_ptr<cartridge_context> cartridge_context,
		slot_id_type SlotId,
		const std::string& iniPath,
		HWND hVccWnd,
		const cpak_callbacks& cpak_callbacks)
	{
		DLOG_C("\ncore::load_cartridge %s\n",filename.c_str());
		switch (VCC::Core::determine_cartridge_type(filename))
		{
		default:
		case cartridge_file_type::not_opened:	//	File doesn't exist or can't be opened
			return { nullptr, nullptr, cartridge_loader_status::cannot_open };

		case cartridge_file_type::rom_image:	//	File is a ROM image
			return VCC::Core::load_rom_cartridge(move(cartridge_context), filename);

		case cartridge_file_type::library:		//	File is a DLL
			return VCC::Core::load_cpak_cartridge(
				filename,
				move(cartridge_context),
				SlotId,							// Where cart is inserted
				iniPath,						// Path to vcc ini file
				hVccWnd,						// MainVcc window handle
				cpak_callbacks);				// Callbacks in here
		}
	}

	// Return error string per cartridge load status.  This abandons loading the strings
	// from Vcc.rc resources so mpi does not need to either access or duplicate them.
	// TODO: Move and make this generic so it also can be used elsewhere in the project
	std::string cartridge_load_error_string(const cartridge_loader_status error_status)
	{
		switch (error_status)
		{
		case cartridge_loader_status::success:
			return "Success";
		case cartridge_loader_status::already_loaded:
			return "Module is already loaded.";
		case cartridge_loader_status::cannot_open:
			return "Cannot open module.";
		case cartridge_loader_status::not_found:
			return "Module not found.";
		case cartridge_loader_status::not_loaded:
			return "Unable to load module.";
		case cartridge_loader_status::not_rom:
			return "Module is not a valid ROM image.";
		case cartridge_loader_status::not_expansion:
			return "Module is not a valid expansion plugin.";
		default:
			return "Unknown error.";
		}
	}
}
