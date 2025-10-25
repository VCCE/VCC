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
#include <vcc/core/legacy_cartridge_definitions.h>
#include <Windows.h>
#include <string>


namespace vcc { namespace core { namespace cartridges
{

	class legacy_cartridge: public cartridge
	{
	public:

		using path_type = std::string;


	public:

		LIBCOMMON_EXPORT legacy_cartridge(
			HMODULE module_handle,
			void* const host_context,
			path_type configuration_path,
			const cpak_cartridge_context& cpak_context);

		LIBCOMMON_EXPORT name_type name() const override;
		LIBCOMMON_EXPORT catalog_id_type catalog_id() const override;
		LIBCOMMON_EXPORT description_type description() const override;

		LIBCOMMON_EXPORT void start() override;
		LIBCOMMON_EXPORT void stop() override;

		LIBCOMMON_EXPORT void reset() override;
		LIBCOMMON_EXPORT void heartbeat() override;
		LIBCOMMON_EXPORT void status(char* text_buffer, size_t buffer_size) override;
		LIBCOMMON_EXPORT void write_port(unsigned char port_id, unsigned char value) override;
		LIBCOMMON_EXPORT unsigned char read_port(unsigned char port_id) override;
		LIBCOMMON_EXPORT unsigned char read_memory_byte(unsigned short memory_address) override;
		LIBCOMMON_EXPORT unsigned short sample_audio() override;
		LIBCOMMON_EXPORT void menu_item_clicked(unsigned char menu_item_id) override;


	private:

		const HMODULE handle_;
		void* const host_context_;
		const path_type configuration_path_;
		const cpak_cartridge_context cpak_context_;

		// imported module functions
		const PakInitializeModuleFunction initialize_;
		const PakTerminateModuleFunction terminate_;
		const PakGetNameModuleFunction get_name_;
		const PakGetCatalogIdModuleFunction get_catalog_id_;
		const PakGetDescriptionModuleFunction get_description_;
		const PakResetModuleFunction reset_;
		const PakHeartBeatModuleFunction heartbeat_;
		const PakGetStatusModuleFunction status_;
		const PakWritePortModuleFunction  write_port_;
		const PakReadPortModuleFunction read_port_;
		const PakReadMemoryByteModuleFunction read_memory_byte_;
		const PakSampleAudioModuleFunction sample_audio_;
		const PakMenuItemClickedModuleFunction menu_item_clicked_;
	};

} } }
