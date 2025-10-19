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
#include <vcc/core/cartridges/basic_cartridge.h>
#include <vcc/core/legacy_cartridge_definitions.h>
#include <Windows.h>
#include <string>


namespace vcc { namespace core { namespace cartridges
{

	class legacy_cartridge: public basic_cartridge
	{
	public:

		using path_type = std::string;


	public:

		LIBCOMMON_EXPORT legacy_cartridge(
			HMODULE moduleHandle,
			const path_type& configurationPath,
			AppendCartridgeMenuModuleCallback addMenuItemCallback,
			ReadMemoryByteModuleCallback readDataFunction,
			WriteMemoryByteModuleCallback writeDataFunction,
			AssertInteruptModuleCallback assertCallback,
			AssertCartridgeLineModuleCallback assertCartCallback);

		LIBCOMMON_EXPORT name_type name() const override;
		LIBCOMMON_EXPORT catalog_id_type catalog_id() const override;

		LIBCOMMON_EXPORT void reset() override;
		LIBCOMMON_EXPORT void heartbeat() override;
		LIBCOMMON_EXPORT void status(char* status) override;
		LIBCOMMON_EXPORT void write_port(unsigned char port_id, unsigned char value) override;
		LIBCOMMON_EXPORT unsigned char read_port(unsigned char port_id) override;
		LIBCOMMON_EXPORT unsigned char read_memory_byte(unsigned short memory_address) override;
		LIBCOMMON_EXPORT unsigned short sample_audio() override;
		LIBCOMMON_EXPORT void menu_item_clicked(unsigned char menu_item_id) override;


	protected:

		LIBCOMMON_EXPORT void initialize_pak() override;


	private:

		const HMODULE handle_;
		const path_type configurationPath_;
		const name_type name_;
		const catalog_id_type catalog_id_;

		// callbacks
		const AppendCartridgeMenuModuleCallback addMenuItemCallback_;
		const ReadMemoryByteModuleCallback readDataFunction_;
		const WriteMemoryByteModuleCallback writeDataFunction_;
		const AssertInteruptModuleCallback assertCallback_;
		const AssertCartridgeLineModuleCallback assertCartCallback_;

		// imported module functions
		const ResetModuleFunction reset_;
		const HeartBeatModuleFunction heartbeat_;
		const GetStatusModuleFunction status_;
		const WritePortModuleFunction  write_port_;
		const ReadPortModuleFunction read_port_;
		const ReadMemoryByteModuleFunction read_memory_byte_;
		const SampleAudioModuleFunction sample_audio_;
		const MenuItemClickedModuleFunction menu_item_clicked_;
	};

} } }
