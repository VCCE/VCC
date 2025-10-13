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
#include <vcc/core/cartridges/legacy_cartridge.h>
#include <stdexcept>

namespace vcc { namespace core { namespace cartridges
{

	namespace
	{

		template<class Type_>
		Type_ GetImportedProcAddress(HMODULE module, LPCSTR procName, Type_ defaultFunction)
		{
			const auto importedFunction(reinterpret_cast<Type_>(GetProcAddress(module, procName)));

			return importedFunction ? importedFunction : defaultFunction;
		}


		void default_get_module_name(char* moduleName, char* projectId, AppendCartridgeMenuModuleCallback addMenuItemCallback)
		{
			*moduleName = 0;
			*projectId = 0;
		}

		void default_menu_item_clicked(unsigned char)
		{}

		void default_set_interupt_call_pointer(AssertInteruptModuleCallback)
		{}

		void default_dma_mem_pointers(ReadMemoryByteModuleCallback, WriteMemoryByteModuleCallback)
		{}

		void default_heartbeat(void)
		{}

		void default_write_port(unsigned char, unsigned char)
		{}

		unsigned char default_read_port(unsigned char)
		{
			return {};
		}

		unsigned char default_read_memory_byte(unsigned short)
		{
			return {};
		}

		void default_status(char* status)
		{
			*status = 0;
		}

		unsigned short default_sample_audio(void)
		{
			return {};
		}

		void default_reset(void)
		{}

		void default_set_ini_path(const char*)
		{}

		void default_set_cart_pointer(AssertCartridgeLineModuleCallback)
		{}

		legacy_cartridge::name_type get_module_name(HMODULE moduleHandle)
		{
			static char name_buffer[MAX_PATH];
			static char catalog_id_buffer[MAX_PATH];
			const auto getModuleName(GetImportedProcAddress(moduleHandle, "ModuleName", default_get_module_name));
			
			getModuleName(name_buffer, catalog_id_buffer, nullptr);

			return name_buffer;
		}

		legacy_cartridge::name_type get_module_catalog_id(HMODULE moduleHandle)
		{
			static char name_buffer[MAX_PATH];
			static char catalog_id_buffer[MAX_PATH];
			const auto getModuleName(GetImportedProcAddress(moduleHandle, "ModuleName", default_get_module_name));

			getModuleName(name_buffer, catalog_id_buffer, nullptr);

			return catalog_id_buffer;
		}

	}


	legacy_cartridge::legacy_cartridge(
		HMODULE moduleHandle,
		const path_type& configurationPath,
		AppendCartridgeMenuModuleCallback addMenuItemCallback,
		ReadMemoryByteModuleCallback readDataFunction,
		WriteMemoryByteModuleCallback writeDataFunction,
		AssertInteruptModuleCallback assertCallback,
		AssertCartridgeLineModuleCallback assertCartCallback)
		:
		handle_(moduleHandle),
		configurationPath_(configurationPath),
		name_(get_module_name(moduleHandle)),
		catalog_id_(get_module_catalog_id(moduleHandle)),
		addMenuItemCallback_(addMenuItemCallback),
		readDataFunction_(readDataFunction),
		writeDataFunction_(writeDataFunction),
		assertCallback_(assertCallback),
		assertCartCallback_(assertCartCallback),
		reset_(GetImportedProcAddress(moduleHandle, "ModuleReset", default_reset)),
		heartbeat_(GetImportedProcAddress(moduleHandle, "HeartBeat", default_heartbeat)),
		status_(GetImportedProcAddress(moduleHandle, "ModuleStatus", default_status)),
		write_port_(GetImportedProcAddress(moduleHandle, "PackPortWrite", default_write_port)),
		read_port_(GetImportedProcAddress(moduleHandle, "PackPortRead", default_read_port)),
		read_memory_byte_(GetImportedProcAddress(moduleHandle, "PakMemRead8", default_read_memory_byte)),
		sample_audio_(GetImportedProcAddress(moduleHandle, "ModuleAudioSample", default_sample_audio)),
		menu_item_clicked_(GetImportedProcAddress(moduleHandle, "ModuleConfig", default_menu_item_clicked))
	{
		// FIXME: Initialization of name and catalog id requires invoking ModuleName
		// twice (once for each detail). Some older paks may not account for this
		// but this is a short-term hack until the existing modules are migrated. This
		// will include renaming the imported functions to better reflect their purpose
		// using the following (preliminary) changes.
		// 
		//		MemPointers			==>		PakInitialize
		//		AssertInterupt		==>		PakInitialize
		//		SetIniPath			==>		PakInitialize
		//		SetCart				==>		PakInitialize
		//		ModuleName			==>		PakGetName, PakGetCatalogId, PakInitialize
		//		ModuleReset			==>		PakReset
		//		HeartBeat			==>		PakPulse
		//		ModuleStatus		==>		PakGetSTatus
		//		PackPortWrite		==>		PakWritePort
		//		PackPortRead		==>		PwkReadPort
		//		PakMemRead8			==>		PakReadMemoryByte
		//		ModuleAudioSample	==>		PakSampleAudio
		//		ModuleConfig		==>		PakProcessMenuItem
	}

	const legacy_cartridge::name_type& legacy_cartridge::name() const
	{
		return name_;
	}

	const legacy_cartridge::catalog_id_type& legacy_cartridge::catalog_id() const
	{
		return catalog_id_;
	}

	void legacy_cartridge::reset()
	{
		reset_();
	}

	void legacy_cartridge::heartbeat()
	{
		heartbeat_();
	}

	void legacy_cartridge::status(char* status_text)
	{
		status_(status_text);
	}

	void legacy_cartridge::write_port(unsigned char portId, unsigned char value)
	{
		write_port_(portId, value);
	}

	unsigned char legacy_cartridge::read_port(unsigned char portId)
	{ 
		return read_port_(portId);
	}

	unsigned char legacy_cartridge::read_memory_byte(unsigned short memoryAddress)
	{
		return read_memory_byte_(memoryAddress);
	}

	unsigned short legacy_cartridge::sample_audio()
	{
		return sample_audio_();
	}

	void legacy_cartridge::menu_item_clicked(unsigned char menuItemId)
	{
		menu_item_clicked_(menuItemId);
	}

	void legacy_cartridge::initialize_pak()
	{
		const auto dmaMemPointer(GetImportedProcAddress(handle_, "MemPointers", default_dma_mem_pointers));
		dmaMemPointer(readDataFunction_, writeDataFunction_);

		const auto setInteruptCallPointer(GetImportedProcAddress(handle_, "AssertInterupt", default_set_interupt_call_pointer));
		setInteruptCallPointer(assertCallback_);

		static char buffer[MAX_PATH];
		const auto getModuleName(GetImportedProcAddress(handle_, "ModuleName", default_get_module_name));
		getModuleName(buffer, buffer, addMenuItemCallback_);

		const auto setIniPath(GetImportedProcAddress(handle_, "SetIniPath", default_set_ini_path));
		setIniPath(configurationPath_.c_str());

		const auto pakSetCart(GetImportedProcAddress(handle_, "SetCart", default_set_cart_pointer));
		pakSetCart(assertCartCallback_);
	}

} } }
