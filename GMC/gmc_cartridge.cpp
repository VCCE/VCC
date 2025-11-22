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
#include "gmc_cartridge.h"
#include "resource.h"
#include <vcc/ui/menu/menu_builder.h>
#include "vcc/common/DialogOps.h"
#include "vcc/utils/persistent_value_store.h"
#include "vcc/utils/winapi.h"
#include "vcc/utils/filesystem.h"
#include <Windows.h>

namespace
{

	std::string select_rom_file()
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

gmc_cartridge::gmc_cartridge(
	std::shared_ptr<expansion_port_host_type> host,
	std::unique_ptr<expansion_port_ui_type> ui,
	std::unique_ptr<expansion_port_bus_type> bus,
	HINSTANCE module_instance)
	:
	bus_(move(bus)),
	ui_(move(ui)),
	host_(move(host)),
	module_instance_(module_instance)
{
}


gmc_cartridge::name_type gmc_cartridge::name() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME);
}

gmc_cartridge::catalog_id_type gmc_cartridge::catalog_id() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_CATNUMBER);
}

gmc_cartridge::description_type gmc_cartridge::description() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_DESCRIPTION);
}

gmc_cartridge::device_type& gmc_cartridge::device()
{
	return *this;
}


void gmc_cartridge::start()
{
	::vcc::utils::persistent_value_store settings(host_->configuration_path());
	const auto selected_file(settings.read(configuration_section_id_, configuration_rom_key_id_));

	load_rom(selected_file, false);
	psg_.start();
}

void gmc_cartridge::reset()
{
	psg_.reset();
}

void gmc_cartridge::write_port(unsigned char port, unsigned char data)
{
	switch (port)
	{
	case mmio_ports::select_bank:
		rom_image_.select_bank(data);
		break;

	case mmio_ports::psg_io:
		psg_.write(data);
		break;
	}
}

unsigned char gmc_cartridge::read_port(unsigned char port)
{
	if (port == mmio_ports::select_bank)
	{
		return rom_image_.selected_bank();
	}

	return 0;
}

unsigned char gmc_cartridge::read_memory_byte(size_type address)
{
	return rom_image_.read_memory_byte(address);
}

void gmc_cartridge::status(char* status, size_t buffer_size)
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

	strcpy(status, message.c_str());
}

unsigned short gmc_cartridge::sample_audio()
{
	sample_type lbuffer = 0;
	sample_type rbuffer = 0;

	return psg_.sound_stream_update(lbuffer, rbuffer);
}

void gmc_cartridge::menu_item_clicked(unsigned char menuId)
{
	if (menuId == menu_item_ids::select_rom)
	{
		const auto selected_file(select_rom_file());
		if (selected_file.empty())
		{
			return;
		}

		::vcc::utils::persistent_value_store settings(host_->configuration_path());
		settings.write(
			configuration_section_id_,
			configuration_rom_key_id_,
			selected_file);

		load_rom(selected_file, true);
	}
}


gmc_cartridge::menu_item_collection_type gmc_cartridge::get_menu_items() const
{
	return ::vcc::ui::menu::menu_builder()
		.add_root_item(menu_item_ids::select_rom, "Select GMC ROM")
		.release_items();
}

void gmc_cartridge::load_rom(const path_type& filename, bool reset_on_load)
{
	if (!filename.empty() && rom_image_.load(filename))
	{
		bus_->set_cartridge_select_line(true);
		if (reset_on_load)
		{
			bus_->reset();
		}
	}
	else
	{
		rom_image_.clear();
	}
}
