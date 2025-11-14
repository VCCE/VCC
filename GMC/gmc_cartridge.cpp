#include "sn76496.h"
#include "resource.h"
#include <vcc/bus/cartridge_capi.h>
#include <vcc/devices/rom/banked_rom_image.h>
#include <vcc/common/DialogOps.h>
#include <vcc/utils/persistent_value_store.h>
#include <vcc/utils/winapi.h>
#include <vcc/utils/filesystem.h>
#include "../CartridgeMenu.h"
#include <Windows.h>


namespace
{

	HINSTANCE gModuleInstance;

	using context_type = cartridge_capi_context;
	using path_type = ::std::string;
	using rom_image_type = ::vcc::devices::rom::banked_rom_image;

	struct menu_item_ids
	{
		static const unsigned int select_rom = 3;
	};

	struct mmio_registers
	{
		static const unsigned char select_bank = 0x40;
		static const unsigned char psg_io = 0x41;
	};

	void* gHostKey = nullptr;
	context_type* context_ = nullptr;
	rom_image_type rom_image_;
	SN76489Device psg_;

	const path_type configuration_section_id_("GMC-SN74689");
	const path_type configuration_rom_key_id_("ROM");
	std::string configuration_path_;


	std::string SelectROMFile()
	{
		FileDialog dlg;

		dlg.setFilter("ROM Files\0*.ROM\0\0");
		dlg.setDefExt("rom");
		dlg.setTitle("Select GMC Rom file");
		if (dlg.show())
		{
			return dlg.path();
		}

		return {};
	}

}

void load_rom(const path_type& filename, bool reset_on_load);
void build_menu();

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		gModuleInstance = hinstDLL;
	}

	return true;
}


extern "C"
{

	__declspec(dllexport) const char* PakGetName()
	{
		static const auto name(::vcc::utils::load_string(gModuleInstance, IDS_MODULE_NAME));
		return name.c_str();
	}

	__declspec(dllexport) const char* PakGetCatalogId()
	{
		static const auto catalog_id(::vcc::utils::load_string(gModuleInstance, IDS_CATNUMBER));
		return catalog_id.c_str();
	}

	__declspec(dllexport) const char* PakGetDescription()
	{
		static const auto description(::vcc::utils::load_string(gModuleInstance, IDS_DESCRIPTION));
		return description.c_str();
	}


	__declspec(dllexport) void PakInitialize(
		void* const host_key,
		const char* const configuration_path,
		const cartridge_capi_context* const context)
	{
		gHostKey = host_key;
		configuration_path_ = configuration_path;

		::vcc::utils::persistent_value_store settings(configuration_path);
		const auto selected_file(settings.read(configuration_section_id_, configuration_rom_key_id_));

		load_rom(selected_file, false);
		build_menu();
	}


	__declspec(dllexport) void PakReset()
	{
		psg_.device_start();
		build_menu();
	}


	__declspec(dllexport) void PakWritePort(unsigned char port, unsigned char data)
	{
		switch (port)
		{
		case mmio_registers::select_bank:
			rom_image_.select_bank(data);
			break;

		case mmio_registers::psg_io:
			psg_.write(data);
			break;
		}
	}

	__declspec(dllexport) unsigned char PakReadPort(unsigned char port)
	{
		if (port == mmio_registers::select_bank)
		{
			return rom_image_.selected_bank();
		}

		return 0;
	}

	__declspec(dllexport) unsigned char PakReadMemoryByte(unsigned short address)
	{
		return rom_image_.read_memory_byte(address);
	}

	__declspec(dllexport) void PakGetStatus(char* text_buffer, size_t buffer_size)
	{
		std::string message("GMC Active");

		const auto activeRom(::vcc::utils::get_filename(rom_image_.filename()));
		if (!rom_image_.empty())
		{
			message += " (" + activeRom + " Loaded)";
		}
		else
		{
			message += activeRom.empty() ? " (No ROM Selected)" : "(Unable to load `" + activeRom + "`)";
		}

		if (message.size() >= buffer_size)
		{
			message.reserve(buffer_size - 1);
		}

		strcpy(text_buffer, message.c_str());
	}

	__declspec(dllexport) unsigned short PakSampleAudio()
	{
		SN76489Device::stream_sample_t lbuffer = 0;
		SN76489Device::stream_sample_t rbuffer = 0;

		return psg_.sound_stream_update(lbuffer, rbuffer);
	}

	__declspec(dllexport) void PakMenuItemClicked(unsigned char menuId)
	{
		if (menuId == menu_item_ids::select_rom)
		{
			const auto selected_file(SelectROMFile());
			if (selected_file.empty())
			{
				return;
			}

			::vcc::utils::persistent_value_store settings(configuration_path_);
			settings.write(
				configuration_section_id_,
				configuration_rom_key_id_,
				selected_file);

			load_rom(selected_file, true);
			build_menu();
		}
	}

}


void build_menu()
{
	context_->add_menu_item(gHostKey, "", MID_BEGIN, MIT_Head);
	context_->add_menu_item(gHostKey, "", MID_ENTRY, MIT_Seperator);
	context_->add_menu_item(gHostKey, "Select GMC ROM", ControlId(menu_item_ids::select_rom), MIT_StandAlone);
	context_->add_menu_item(gHostKey, "", MID_FINISH, MIT_Head);
}

void load_rom(const path_type& filename, bool reset_on_load)
{
	if (!filename.empty() && rom_image_.load(filename))
	{
		context_->assert_cartridge_line(gHostKey, true);
		if (reset_on_load)
		{
			// FIXME: This functionality is not available in the C API
			//context_->reset(gHostKey);
		}
	}
	else
	{
		rom_image_.clear();
	}
}
