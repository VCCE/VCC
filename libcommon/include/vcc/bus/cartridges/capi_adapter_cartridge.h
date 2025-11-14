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
#include <vcc/bus/cartridge_capi.h>
#include <Windows.h>
#include <string>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace vcc::bus::cartridges
{

	class capi_adapter_cartridge : public ::vcc::bus::cartridge
	{
	public:

		using path_type = std::string;


	public:

		LIBCOMMON_EXPORT capi_adapter_cartridge(
			HMODULE module_handle,
			void* host_key,
			path_type configuration_path,
			const cartridge_capi_context& capi_context);

		LIBCOMMON_EXPORT [[nodiscard]] name_type name() const override;
		LIBCOMMON_EXPORT [[nodiscard]] catalog_id_type catalog_id() const override;
		LIBCOMMON_EXPORT [[nodiscard]] description_type description() const override;

		LIBCOMMON_EXPORT void start() override;
		LIBCOMMON_EXPORT void stop() override;
		LIBCOMMON_EXPORT void reset() override;

		LIBCOMMON_EXPORT [[nodiscard]] unsigned char read_memory_byte(size_type memory_address) override;

		LIBCOMMON_EXPORT void write_port(unsigned char port_id, unsigned char value) override;
		LIBCOMMON_EXPORT [[nodiscard]] unsigned char read_port(unsigned char port_id) override;

		LIBCOMMON_EXPORT void update(float delta) override;
		LIBCOMMON_EXPORT [[nodiscard]] unsigned short sample_audio() override;

		LIBCOMMON_EXPORT void status(char* text_buffer, size_type buffer_size) override;
		LIBCOMMON_EXPORT void menu_item_clicked(unsigned char menu_item_id) override;


	private:

		const HMODULE handle_;
		void* const host_key_;
		const path_type configuration_path_;
		const cartridge_capi_context capi_context_;

		// imported module functions
		const PakInitializeModuleFunction initialize_;
		const PakTerminateModuleFunction terminate_;
		const PakGetNameModuleFunction get_name_;
		const PakGetCatalogIdModuleFunction get_catalog_id_;
		const PakGetDescriptionModuleFunction get_description_;
		const PakResetModuleFunction reset_;
		const PakUpdateModuleFunction update_;
		const PakGetStatusModuleFunction status_;
		const PakWritePortModuleFunction  write_port_;
		const PakReadPortModuleFunction read_port_;
		const PakReadMemoryByteModuleFunction read_memory_byte_;
		const PakSampleAudioModuleFunction sample_audio_;
		const PakMenuItemClickedModuleFunction menu_item_clicked_;
	};

}

#endif
