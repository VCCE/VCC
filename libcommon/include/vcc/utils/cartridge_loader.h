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
#include <vcc/core/cartridge.h>
#include <vcc/core/cartridge_context.h>
#include <vcc/core/cartridge_capi.h>
#include <vcc/utils/dll_deleter.h>
#include <string>
#include <memory>
#include <Windows.h>


namespace vcc::utils
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
		unsupported_api,
		not_loaded,
		not_rom,
		not_expansion
	};


	struct LIBCOMMON_EXPORT cartridge_loader_result
	{
		using handle_type = std::unique_ptr<std::remove_pointer_t<HMODULE>, vcc::utils::dll_deleter>;
		using cartridge_ptr_type = std::unique_ptr<vcc::core::cartridge>;

#pragma warning(push)
#pragma warning(disable: 4251)
		handle_type handle;
		cartridge_ptr_type cartridge;
#pragma warning(pop)
		cartridge_loader_status load_result = cartridge_loader_status::not_loaded;
	};

	LIBCOMMON_EXPORT [[nodiscard]] cartridge_file_type determine_cartridge_type(const std::string& filename);

	LIBCOMMON_EXPORT [[nodiscard]] cartridge_loader_result load_rom_cartridge(
		const std::string& filename,
		std::unique_ptr<::vcc::core::cartridge_context> cartridge_context);

	LIBCOMMON_EXPORT [[nodiscard]] cartridge_loader_result load_capi_cartridge(
		const std::string& filename,
		std::unique_ptr<::vcc::core::cartridge_context> cartridge_context,
		void* const host_context,
		const std::string& iniPath,
		const cartridge_capi_context& capi_context);

	LIBCOMMON_EXPORT [[nodiscard]] cartridge_loader_result load_cartridge(
		const std::string& filename,
		std::unique_ptr<::vcc::core::cartridge_context> cartridge_context,
		const cartridge_capi_context& capi_context,
		void* const host_context,
		const std::string& iniPath);

	LIBCOMMON_EXPORT ::std::string load_error_string(cartridge_loader_status status);
}
