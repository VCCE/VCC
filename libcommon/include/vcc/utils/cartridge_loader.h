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
#include "vcc/bus/cartridge.h"
#include "vcc/bus/cartridge_context.h"
#include "vcc/utils/dll_deleter.h"
#include <string>
#include <memory>
#include <Windows.h>


namespace vcc::utils
{

	/// @brief Represents the different types of files the loader can identify.
	enum class cartridge_file_type
	{
		/// @brief Specifies that the file was not opened.
		/// 
		/// @todo Remove this and use std::optional to indicate no type determined.
		not_opened,
		/// @brief The cartridge is a raw ROM file.
		rom_image,
		/// @brief The cartridge is a shared library that provides custom functionality.
		library
	};

	/// @brief Represents the different conditions that occur when loading a cartridge.
	enum class cartridge_loader_status
	{
		/// @brief The cartridge was loaded successfully.
		success,
		/// @brief The cartridge is a shared library and is already loaded.
		already_loaded,
		/// @brief The cartridge file cannot be opened.
		cannot_open,
		/// @brief The cartridge file cannot be found.
		not_found,
		/// @brief The cartridge file is a shared library but uses an API not supported by the
		/// system.
		unsupported_api,
		/// @brief The cartridge file was not loaded.
		not_loaded,
		/// @brief The cartridge file was loaded as a ROM pak but the file does not contain a ROM.
		/// @todo Rename this to something else (i.e. invalid_format)
		not_rom,
		/// @brief The cartridge file was loaded as a shared library but it does not use a
		/// known cartridge interface.
		not_expansion
	};

	/// @brief Represents the results of loading a cartridge.
	struct cartridge_loader_result
	{
		/// @brief Specifies the type of handle used to reference shared libraries.
		using handle_type = std::unique_ptr<std::remove_pointer_t<HMODULE>, vcc::utils::dll_deleter>;
		/// @brief Specifies the type pointer used to reference instances of a cartridge.
		using cartridge_ptr_type = std::unique_ptr<::vcc::bus::cartridge>;


		/// @brief The reference to the shared library containing the custom cartridge
		/// functionality or empty (also nullptr) if the cartridge loaded is not
		/// a shared library. If the load files this value is empty.
		handle_type handle;

		/// @brief The reference to the instance of the loaded cartridge.  If the load files
		/// this value is empty.
		cartridge_ptr_type cartridge;

		/// @brief The status result of the load operation.
		cartridge_loader_status load_result = cartridge_loader_status::not_loaded;
	};

	/// @brief Determines the type of cartridge contained in a file.
	/// 
	/// Determines the type of cartridge contained in a file.
	/// 
	/// @todo This should return a std::optional using an empty value to indicate no type determined.
	/// 
	/// @param filename The name of the file to check.
	/// 
	/// @return The type of cartridge detected.
	LIBCOMMON_EXPORT [[nodiscard]] cartridge_file_type determine_cartridge_type(const std::string& filename);

	/// @brief Load a ROM file as a cartridge.
	/// 
	/// @param filename The name of the ROM file to load.
	/// @param cartridge_context The host context of the system that will manage the cartridge.
	/// 
	/// @return A status result and data references needed to manage the cartridge.
	LIBCOMMON_EXPORT [[nodiscard]] cartridge_loader_result load_rom_cartridge(
		const std::string& filename,
		std::unique_ptr<::vcc::bus::cartridge_context> cartridge_context);

	/// @brief Load a shared library as a cartridge.
	/// 
	/// Load a shared library containing a cartridge plugin.
	/// 
	/// @param filename The name of the shared library to load.
	/// @param cartridge_context The fluent C++ context of the system that will manage the cartridge.
	/// @param host_context A handle identifying the host managing the cartridge.
	/// @param configuration_path The path of the configuration file.
	/// 
	/// @return A status result and data references needed to manage the cartridge.
	LIBCOMMON_EXPORT [[nodiscard]] cartridge_loader_result load_library_cartridge(
		const std::string& filename,
		std::unique_ptr<::vcc::bus::cartridge_context> cartridge_context);

	/// @brief Load a cartridge file.
	/// 
	/// Load a variety of automatically detected cartridge types.
	/// 
	/// @param filename The name of the cartridge to load.
	/// @param cartridge_context The fluent C++ context of the system that will manage the cartridge.
	/// @param host_context A handle identifying the host managing the cartridge.
	/// @param configuration_path The path of the configuration file.
	/// 
	/// @return A status result and data references needed to manage the cartridge.
	LIBCOMMON_EXPORT [[nodiscard]] cartridge_loader_result load_cartridge(
		const std::string& filename,
		std::unique_ptr<::vcc::bus::cartridge_context> cartridge_context);

	/// @brief Retrieve a string describing a cartridge loader error
	/// .
	/// @param status The status to retrieve the description for.
	/// 
	/// @return A string containing a description of the error.
	LIBCOMMON_EXPORT std::string load_error_string(cartridge_loader_status status);
}
