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
#include <vcc/bus/cpak_cartridge_definitions.h>
#include <vcc/bus/cartridge_menuitem.h>
#include <Windows.h>
#include <string>

namespace VCC::Core
{

	class cpak_cartridge: public cartridge
	{
	public:

		using path_type = std::string;

	public:

		cpak_cartridge(
			HMODULE module_handle,                   // Cartridge filename
			slot_id_type const SlotId,               // Slot id
			path_type configuration_path,            // Path of ini file
			HWND hVccWnd,                            // Handle to main VCC window proc 
			const cpak_callbacks& cpak_callbacks);   // Callbacks

		name_type name() const override;
		catalog_id_type catalog_id() const override;
		description_type description() const override;

		void start() override;
		void stop() override;

		void reset() override;
		void process_horizontal_sync() override;
		void status(char* text_buffer, size_t buffer_size) override;
		void write_port(unsigned char port_id, unsigned char value) override;
		unsigned char read_port(unsigned char port_id) override;
		unsigned char read_memory_byte(unsigned short memory_address) override;
		unsigned short sample_audio() override;
		void menu_item_clicked(unsigned char menu_item_id) override;
		bool get_menu_item(menu_item_entry* item, size_t index) override;

	private:

		const HMODULE handle_;
		slot_id_type SlotId_;
		const HWND hVccWnd_;
		const path_type configuration_path_;
		const cpak_callbacks cpak_callbacks_;

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
		const PakGetMenuItemFunction get_menu_item_;
	};

}
