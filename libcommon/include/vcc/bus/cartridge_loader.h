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
#pragma once
#include <vcc/bus/cartridge.h>
#include <vcc/bus/cartridge_callbacks.h>
#include <vcc/bus/cpak_cartridge_definitions.h>
#include <vcc/bus/dll_deleter.h>
#include <string>
#include <memory>
#include <Windows.h>


namespace vcc::core
{

	enum class cartridge_file_type
	{
		not_opened,
		rom_image,
		library
	};

	enum class cartridge_loader_status
	{
		success,
		already_loaded,
		cannot_open,
		not_found,
		not_loaded,
		not_rom,
		not_expansion
	};

	struct LIBCOMMON_EXPORT cartridge_loader_result
	{
		using handle_type = std::unique_ptr<std::remove_pointer_t<HMODULE>, vcc::core::dll_deleter>;
		using cartridge_ptr_type = std::unique_ptr<vcc::core::cartridge>;

#pragma warning(push)
#pragma warning(disable: 4251)
		handle_type handle;
		cartridge_ptr_type cartridge;
#pragma warning(pop)
		cartridge_loader_status load_result = cartridge_loader_status::not_loaded;
	};

	// TODO: Are these really needed - callbacks are defined in cartridge_callbacks.h
/*	struct cartridge_loader_context
	{
		const PakAssertInteruptHostCallback pak_assert_interrupt;
		const PakAssertCartridgeLineHostCallback pak_assert_cartridge_line;
		const PakWriteMemoryByteHostCallback pak_write_memory_byte;
		const PakReadMemoryByteHostCallback pak_read_memory_byte;
		const PakAppendCartridgeMenuHostCallback pak_add_menu_item;
	};
*/
	LIBCOMMON_EXPORT cartridge_file_type determine_cartridge_type(const std::string& filename);

	// TODO: Eliminate the redundant overloads of loader result
	LIBCOMMON_EXPORT cartridge_loader_result load_rom_cartridge(
		const std::string& filename,
		std::unique_ptr<cartridge_context> cartridge_context);

	LIBCOMMON_EXPORT cartridge_loader_result load_cpak_cartridge(
		const std::string& filename,
		std::unique_ptr<cartridge_context> cartridge_context,
		void* const host_context,
		const std::string& iniPath,
		HWND hVccWnd,
		const cpak_callbacks& cpak_callbacks);

	LIBCOMMON_EXPORT cartridge_loader_result load_cartridge(
		const std::string& filename,                          // Cartridge filename
		std::unique_ptr<cartridge_context> cartridge_context, // Yet another cartridge context
		void* const host_context,                             // Even more context
		const std::string& iniPath,                           // Path of ini file
		HWND hVccWnd,                                         // handle to main VCC window proc
		const cpak_callbacks& cpak_callbacks);                // Callbacks AKA context

	// Return load error string per cartridge load status
	LIBCOMMON_EXPORT std::string cartridge_load_error_string(
			const cartridge_loader_status error_status);

}
