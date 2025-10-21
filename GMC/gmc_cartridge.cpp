#include "gmc_cartridge.h"
#include "resource.h"
#include <vcc/common/DialogOps.h>
#include <vcc/core/utils/winapi.h>
#include <vcc/core/utils/filesystem.h>
#include <Windows.h>
#include "../CartridgeMenu.h"
#include <vcc/core/utils/configuration_serializer.h>

namespace
{

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

const gmc_cartridge::path_type gmc_cartridge::configuration_section_id_("GMC-SN74689");
const gmc_cartridge::path_type gmc_cartridge::configuration_rom_key_id_("ROM");

gmc_cartridge::gmc_cartridge(std::unique_ptr<context_type> context, HINSTANCE module_instance)
	:
	context_(move(context)),
	module_instance_(module_instance)
{}


gmc_cartridge::name_type gmc_cartridge::name() const
{
	return ::vcc::core::utils::load_string(module_instance_, IDS_MODULE_NAME);
}

gmc_cartridge::catalog_id_type gmc_cartridge::catalog_id() const
{
	return ::vcc::core::utils::load_string(module_instance_, IDS_CATNUMBER);
}

gmc_cartridge::description_type gmc_cartridge::description() const
{
	return ::vcc::core::utils::load_string(module_instance_, IDS_DESCRIPTION);
}

void gmc_cartridge::start()
{
	::vcc::core::configuration_serializer serializer(context_->configuration_path());
	const auto selected_file(serializer.read(configuration_section_id_, configuration_rom_key_id_));

	load_rom(selected_file, false);
	build_menu();
}

void gmc_cartridge::stop()
{}


void gmc_cartridge::reset()
{
	psg_.device_start();
	build_menu();
}

void gmc_cartridge::process_horizontal_sync()
{}

void gmc_cartridge::write_port(unsigned char port, unsigned char data)
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

unsigned char gmc_cartridge::read_port(unsigned char port)
{
	if (port == mmio_registers::select_bank)
	{
		return rom_image_.selected_bank();
	}

	return 0;
}

unsigned char gmc_cartridge::read_memory_byte(unsigned short address)
{
	return rom_image_.read_memory_byte(address);
}

void gmc_cartridge::status(char* status, size_t buffer_size)
{
	std::string message("GMC Active");

	const auto activeRom(::vcc::core::utils::get_filename(rom_image_.filename()));
	if (!rom_image_.empty())
	{
		message += " (" + activeRom + " Loaded)";
	}
	else
	{
		message += activeRom.empty() ? " (No ROM Selected)" : "(Unable to load `" + activeRom + "`)";
	}

	if(message.size() >= buffer_size)
	{
		message.reserve(buffer_size - 1);
	}

	strcpy(status, message.c_str());
}

unsigned short gmc_cartridge::sample_audio()
{
	SN76489Device::stream_sample_t lbuffer = 0;
	SN76489Device::stream_sample_t rbuffer = 0;

	return psg_.sound_stream_update(lbuffer, rbuffer);
}

void gmc_cartridge::menu_item_clicked(unsigned char menuId)
{
	if (menuId == menu_item_ids::select_rom)
	{
		const auto selected_file(SelectROMFile());
		if (selected_file.empty())
		{
			return;
		}

		::vcc::core::configuration_serializer serializer(context_->configuration_path());
		serializer.write(
			configuration_section_id_,
			configuration_rom_key_id_,
			selected_file);

		load_rom(selected_file, true);
		build_menu();
	}
}


void gmc_cartridge::build_menu()
{
	context_->add_menu_item("", MID_BEGIN, MIT_Head);
	context_->add_menu_item("", MID_ENTRY, MIT_Seperator);
	context_->add_menu_item("Select GMC ROM", ControlId(menu_item_ids::select_rom), MIT_StandAlone);
	context_->add_menu_item("", MID_FINISH, MIT_Head);
}

void gmc_cartridge::load_rom(const path_type& filename, bool reset_on_load)
{
	if (!filename.empty() && rom_image_.load(filename))
	{
		context_->assert_cartridge_line(true);
		if (reset_on_load)
		{
			context_->reset();
		}
	}
	else
	{
		rom_image_.clear();
	}
}
