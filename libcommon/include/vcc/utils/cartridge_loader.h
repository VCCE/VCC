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
/// @file
///
/// Contains functions for loading various types of cartridge files.
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_port_host.h"
#include "vcc/bus/expansion_port_ui.h"
#include "vcc/bus/expansion_port_bus.h"
#include "vcc/utils/dll_deleter.h"
#include <string>
#include <functional>
#include <memory>
#include <filesystem>
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
		/// @brief The cartridge id is unknown.
		unknown_cartridge,
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
		/// @todo rename this to managed_handle_type
		using handle_type = std::unique_ptr<std::remove_pointer_t<HMODULE>, vcc::utils::dll_deleter>;
		/// @brief Specifies the type pointer used to reference instances of a cartridge.
		using cartridge_ptr_type = std::unique_ptr<::vcc::bus::cartridge>;

		/// @brief The reference to the shared library containing the custom cartridge
		/// functionality or null if the cartridge loaded is not a shared library. If the load
		/// files this value is null.
		handle_type handle;

		/// @brief The reference to the instance of the loaded cartridge.  If the load fails
		/// this member is null.
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
	/// @param pathname The name of the file to check.
	/// 
	/// @return The type of cartridge detected.
	LIBCOMMON_EXPORT [[nodiscard]] cartridge_file_type determine_cartridge_type(const std::filesystem::path& pathname);

	/// @brief Load a ROM file as a cartridge.
	/// 
	/// @param pathname The name of the ROM file to load.
	/// @param host The host system that will manage the cartridge.
	/// @param ui The user interface manager that allows interaction between the cartridge
	/// and the user.
	/// @param bus The expansion bus of the emulated system.
	/// 
	/// @return A status result and data references needed to manage the cartridge.
	/// 
	/// @throws std::invalid_argument if `bus` is null.
	LIBCOMMON_EXPORT [[nodiscard]] cartridge_loader_result load_rom_cartridge(
		const std::filesystem::path& pathname,
		std::shared_ptr<::vcc::bus::expansion_port_host> host,
		std::unique_ptr<::vcc::bus::expansion_port_ui> ui,
		std::unique_ptr<::vcc::bus::expansion_port_bus> bus);

	/// @brief Load a shared library as a cartridge.
	/// 
	/// Load a shared library containing a cartridge plugin.
	/// 
	/// @param pathname The name of the shared library to load.
	/// @param host The host system that will manage the cartridge.
	/// @param ui The user interface manager that allows interaction between the cartridge
	/// and the user.
	/// @param bus The expansion bus of the emulated system.
	/// 
	/// @return A status result and data references needed to manage the cartridge.
	/// 
	/// @throws std::invalid_argument if `bus` is null.
	LIBCOMMON_EXPORT [[nodiscard]] cartridge_loader_result load_library_cartridge(
		const std::filesystem::path& pathname,
		std::shared_ptr<::vcc::bus::expansion_port_host> host,
		std::unique_ptr<::vcc::bus::expansion_port_ui> ui,
		std::unique_ptr<::vcc::bus::expansion_port_bus> bus);

	/// @brief Load a cartridge file.
	/// 
	/// Load a variety of automatically detected cartridge types.
	/// 
	/// @param pathname The name of the cartridge to load.
	/// @param host The host system that will manage the cartridge.
	/// @param ui The user interface manager that allows interaction between the cartridge
	/// and the user.
	/// @param bus The expansion bus of the emulated system.
	/// 
	/// @return A status result and data references needed to manage the cartridge.
	/// 
	/// @throws std::invalid_argument if `bus` is null.
	LIBCOMMON_EXPORT [[nodiscard]] cartridge_loader_result load_cartridge(
		const std::filesystem::path& pathname,
		std::shared_ptr<::vcc::bus::expansion_port_host> host,
		std::unique_ptr<::vcc::bus::expansion_port_ui> ui,
		std::unique_ptr<::vcc::bus::expansion_port_bus> bus);

	/// @brief Retrieve a string describing a cartridge loader error.
	/// 
	/// @param status The status to retrieve the description for.
	/// 
	/// @return A string containing a description of the error.
	[[nodiscard]] LIBCOMMON_EXPORT std::string load_error_string(cartridge_loader_status status);

	/// @brief Selects a cartridge file 
	/// 
	/// Opens a file select dialog and invokes a callback function if one is selected.
	/// 
	/// @param parent_window The parent or owner window of the file selection dialog.
	/// @param title The title of the dialog to present to the user.
	/// @param initial_path The directory to open.
	/// @param execute_load A callback function that will be invoked if a file is selected.
	LIBCOMMON_EXPORT void select_cartridge_file(
		HWND parent_window,
		const std::string& title,
		const std::filesystem::path& initial_path,
		const std::function<cartridge_loader_status(const std::string&)>& execute_load);

}
