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

		void default_stop()
		{}

		void default_menu_item_clicked(unsigned char)
		{}

		void default_heartbeat()
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

		void default_status(char* text_buffer, size_t buffer_size)
		{
			*text_buffer = 0;
		}

		unsigned short default_sample_audio()
		{
			return {};
		}

		void default_reset()
		{}

		const char* default_get_empty_string()
		{
			return "";
		}

	}


	legacy_cartridge::legacy_cartridge(
		HMODULE module_handle,
		void* const host_context,
		path_type configuration_path,
		const cpak_cartridge_context& cpak_context)
		:
		handle_(module_handle),
		host_context_(host_context),
		configuration_path_(move(configuration_path)),
		cpak_context_(cpak_context),
		initialize_(GetImportedProcAddress<PakInitializeModuleFunction>(module_handle, "PakInitialize", nullptr)),
		terminate_(GetImportedProcAddress(module_handle, "PakTerminate", default_stop)),
		get_name_(GetImportedProcAddress(module_handle, "PakGetName", default_get_empty_string)),
		get_catalog_id_(GetImportedProcAddress(module_handle, "PakGetCatalogId", default_get_empty_string)),
		get_description_(GetImportedProcAddress(module_handle, "PakGetDescription", default_get_empty_string)),
		reset_(GetImportedProcAddress(module_handle, "PakReset", default_reset)),
		heartbeat_(GetImportedProcAddress(module_handle, "PakProcessHorizontalSync", default_heartbeat)),
		status_(GetImportedProcAddress(module_handle, "PakGetStatus", default_status)),
		write_port_(GetImportedProcAddress(module_handle, "PakWritePort", default_write_port)),
		read_port_(GetImportedProcAddress(module_handle, "PakReadPort", default_read_port)),
		read_memory_byte_(GetImportedProcAddress(module_handle, "PakReadMemoryByte", default_read_memory_byte)),
		sample_audio_(GetImportedProcAddress(module_handle, "PakSampleAudio", default_sample_audio)),
		menu_item_clicked_(GetImportedProcAddress(module_handle, "PakMenuItemClicked", default_menu_item_clicked))
	{
		if (initialize_ == nullptr)
		{
			throw std::invalid_argument("Cannot initialize pak interface. Module does not export PakInitialize.");
		}
	}

	legacy_cartridge::name_type legacy_cartridge::name() const
	{
		return get_name_();
	}

	legacy_cartridge::catalog_id_type legacy_cartridge::catalog_id() const
	{
		return get_catalog_id_();
	}

	legacy_cartridge::description_type legacy_cartridge:: description() const
	{
		return get_description_();
	}


	void legacy_cartridge::start()
	{
		initialize_(host_context_, configuration_path_.c_str(), &cpak_context_);
	}

	void legacy_cartridge::stop()
	{
		terminate_();
	}

	void legacy_cartridge::reset()
	{
		reset_();
	}

	void legacy_cartridge::process_horizontal_sync()
	{
		heartbeat_();
	}

	void legacy_cartridge::status(char* text_buffer, size_t buffer_size)
	{
		status_(text_buffer, buffer_size);
	}

	void legacy_cartridge::write_port(unsigned char port_id, unsigned char value)
	{
		write_port_(port_id, value);
	}

	unsigned char legacy_cartridge::read_port(unsigned char port_id)
	{ 
		return read_port_(port_id);
	}

	unsigned char legacy_cartridge::read_memory_byte(unsigned short memory_address)
	{
		return read_memory_byte_(memory_address);
	}

	unsigned short legacy_cartridge::sample_audio()
	{
		return sample_audio_();
	}

	void legacy_cartridge::menu_item_clicked(unsigned char menu_item_id)
	{
		menu_item_clicked_(menu_item_id);
	}

} } }
