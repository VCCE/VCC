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
	std::shared_ptr<expansion_port_bus_type> bus,
	HINSTANCE module_instance)
	:
	bus_(bus),
	ui_(move(ui)),
	host_(move(host)),
	module_instance_(module_instance),
	device_(bus)
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
	return device_;
}


void gmc_cartridge::start()
{
	::vcc::utils::persistent_value_store settings(host_->configuration_path());
	const auto selected_file(settings.read(configuration_section_id_, configuration_rom_key_id_));

	device_.start(selected_file);
}

void gmc_cartridge::status(char* status, size_t buffer_size)
{
	std::string message("GMC Active");

	const auto activeRom(::vcc::utils::get_filename(device_.rom_filename()));
	if (device_.has_rom())
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

		device_.load_rom(selected_file, true);
	}
}


gmc_cartridge::menu_item_collection_type gmc_cartridge::get_menu_items() const
{
	return ::vcc::ui::menu::menu_builder()
		.add_root_item(menu_item_ids::select_rom, "Select GMC ROM")
		.release_items();
}

