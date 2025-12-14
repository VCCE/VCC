////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute it and/or
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
#include "vcc/ui/menu/menu_builder.h"
#include "vcc/utils/winapi.h"
#include "vcc/utils/filesystem.h"
#include <Windows.h>
#include <stdexcept>


namespace vcc::cartridges::gmc
{

	gmc_cartridge::gmc_cartridge(
		std::shared_ptr<expansion_port_host_type> host,
		std::shared_ptr<expansion_port_ui_type> ui,
		std::unique_ptr<expansion_port_bus_type> bus,
		HINSTANCE module_instance)
		:
		host_(move(host)),
		ui_(move(ui)),
		module_instance_(module_instance),
		driver_(move(bus))
	{
		if (host_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Game Master Cartridge. The host pointer is null.");
		}

		if (ui_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Game Master Cartridge. UI Service is null.");
		}

		if (module_instance_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Game Master Cartridge. The module handle is null.");
		}
	}

	gmc_cartridge::name_type gmc_cartridge::name() const
	{
		return ::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME);
	}

	gmc_cartridge::driver_type& gmc_cartridge::driver()
	{
		return driver_;
	}

	void gmc_cartridge::start()
	{
		::vcc::utils::persistent_value_store settings(host_->configuration_path());
		const auto selected_file(settings.read(configuration::section, configuration::keys::rom_filename));

		driver_.start(selected_file);
	}

	void gmc_cartridge::menu_item_clicked(menu_item_id_type menu_item_id)
	{
		if (menu_item_id == menu_item_ids::select_rom)
		{
			::vcc::utils::select_rompak_cartridge_file(
				ui_->app_window(),
				"Select ROM Image for Game Master Cartridge",
				ui_->last_accessed_rompak_path(),
				std::bind(&gmc_cartridge::load_selected_rom, this, std::placeholders::_1));

			return;
		}

		if (menu_item_id == menu_item_ids::remove_rom)
		{
			value_store_type(host_->configuration_path()).remove(
				configuration::section,
				configuration::keys::rom_filename);

			driver_.eject_rom();
		}
	}


	gmc_cartridge::menu_item_collection_type gmc_cartridge::get_menu_items() const
	{
		return ::vcc::ui::menu::menu_builder()
			.add_root_submenu("Game Master Cartridge")
			.add_submenu_item(menu_item_ids::select_rom, "Insert ROM")
			.add_submenu_item(menu_item_ids::remove_rom, "Eject ROM", nullptr, !driver_.has_rom())
			.release_items();
	}


	gmc_cartridge::load_rom_status_type gmc_cartridge::load_selected_rom(const path_type& filename)
	{
		if (const auto result(driver_.load_rom(filename, true)); result)
		{
			value_store_type(host_->configuration_path()).write(
				configuration::section,
				configuration::keys::rom_filename,
				filename);

			ui_->last_accessed_rompak_path(filename.parent_path());

			return load_rom_status_type::success;
		}

		return load_rom_status_type::general_load_failure;
	}

}
