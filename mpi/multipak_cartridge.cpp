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
#include "multipak_cartridge.h"
#include "multipak_configuration.h"
#include "multipak_expansion_port_ui.h"
#include "multipak_expansion_port_bus.h"
#include "configuration_dialog.h"
#include "resource.h"
#include "vcc/ui/menu/menu_builder.h"
#include "vcc/utils/filesystem.h"
#include "vcc/utils/winapi.h"
#include "vcc/bus/cartridge.h"
#include "vcc/bus/cartridges/empty_cartridge.h"
#include <array>
#include <format>


namespace vcc::cartridges::multipak
{

	namespace
	{

		UINT get_icon_id(bool is_at_selected_slot, bool is_slot_occupied)
		{
			if (is_at_selected_slot)
			{
				return is_slot_occupied
					? IDB_CARTRIDGE_SLOT_OCCUPIED_SELECTED
					: IDB_CARTRIDGE_SLOT_EMPTY_SELECTED;
			}

			return is_slot_occupied
				? IDB_CARTRIDGE_SLOT_OCCUPIED
				: IDB_CARTRIDGE_SLOT_EMPTY;
		}

	}

	const std::array<
		multipak_cartridge::slot_action_command_descriptor,
		multipak_cartridge::total_slot_count> multipak_cartridge::slot_action_command_ids_ =
	{
		slot_action_command_descriptor{
			menu_item_ids::select_slot_1,
			menu_item_ids::insert_into_slot_1,
			menu_item_ids::eject_slot_1},

		slot_action_command_descriptor{
			menu_item_ids::select_slot_2,
			menu_item_ids::insert_into_slot_2,
			menu_item_ids::eject_slot_2},

		slot_action_command_descriptor{
			menu_item_ids::select_slot_3,
			menu_item_ids::insert_into_slot_3,
			menu_item_ids::eject_slot_3},

		slot_action_command_descriptor{
			menu_item_ids::select_slot_4,
			menu_item_ids::insert_into_slot_4,
			menu_item_ids::eject_slot_4},
	};


	multipak_cartridge::multipak_cartridge(
		std::shared_ptr<expansion_port_host_type> host,
		std::shared_ptr<expansion_port_ui_type> ui,
		std::shared_ptr<expansion_port_bus_type> bus,
		std::shared_ptr<multipak_cartridge_driver> driver,
		// FIXME-CHET: globally rename module_instance to library_handle
		HINSTANCE module_instance,
		std::shared_ptr<multipak_configuration> configuration)
		:
		host_(host),
		ui_(ui),
		bus_(bus),
		driver_(driver),
		configuration_(configuration),
		module_instance_(module_instance),
		settings_dialog_(
			module_instance,
			configuration,
			driver_,
			std::bind(&multipak_cartridge::select_and_insert_cartridge, this, std::placeholders::_1),
			std::bind(&multipak_cartridge::eject_cartridge, this, std::placeholders::_1, true))
	{
		if (host_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Multi-Pak Cartridge. Host is null.");
		}

		if (ui_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Multi-Pak Cartridge. UI Service is null.");
		}

		if (bus_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Multi-Pak Cartridge. Bus is null.");
		}

		if (configuration_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Multi-Pak Cartridge. Configuration is null.");
		}

		if (driver_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Multi-Pak Cartridge. Driver is null.");
		}

		if (module_instance_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Multi-Pak Cartridge. Module handle is null.");
		}

		for (auto& controller : cartridges_)
		{
			controller = std::make_shared<::vcc::bus::cartridges::empty_cartridge>();
		}
	}


	multipak_cartridge::name_type multipak_cartridge::name() const
	{
		return ::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME);
	}

	multipak_cartridge::driver_type& multipak_cartridge::driver()
	{
		return *driver_;
	}


	void multipak_cartridge::start()
	{
		driver_->start();

		for (auto slot(0u); slot < driver_->slot_count(); slot++)
		{
			const auto path(vcc::utils::find_pak_module_path(configuration_->slot_path(slot)));
			if (!path.empty())
			{
				if (const auto insert_result(insert_cartridge(slot, path, false));
					insert_result != mount_status_type::success)
				{
					// FIXME-CHET: Report error to host
				}
			}
		}

	}

	void multipak_cartridge::stop()
	{
		settings_dialog_.close();

		for (auto& controller : cartridges_)
		{
			controller->stop();
		}

		cartridges_ = {};
		driver_->stop();
	}

	void multipak_cartridge::status(char* text_buffer, size_t buffer_size)
	{
		char TempStatus[64] = "";

		sprintf(text_buffer, "MPI:%d,%d", driver_->selected_cts_slot() + 1, driver_->selected_scs_slot() + 1);
		for (const auto& cartridge : cartridges_)
		{
			strcpy(TempStatus, "");
			cartridge->status(TempStatus, sizeof(TempStatus));
			if (TempStatus[0])
			{
				strcat(text_buffer, " | ");
				strcat(text_buffer, TempStatus);
			}
		}
	}

	void multipak_cartridge::menu_item_clicked(menu_item_id_type menu_item_id)
	{
		// FIXME-CHET: This needs to be simplified. If the multipak itself is considered
		// slot 0 the id can be divided by the item count per slot to get the slot id
		// (0 = multi-pak, others = slots) and modulus can be used to determine the id to
		// send to the cartridges.
		switch (menu_item_id)
		{
		case menu_item_ids::open_settings:
			settings_dialog_.open();
			return;

		case menu_item_ids::select_slot_1:
		case menu_item_ids::select_slot_2:
		case menu_item_ids::select_slot_3:
		case menu_item_ids::select_slot_4:
			switch_to_slot(menu_item_id - menu_item_ids::select_slot_1);
			return;

		case menu_item_ids::insert_into_slot_1:
		case menu_item_ids::insert_into_slot_2:
		case menu_item_ids::insert_into_slot_3:
		case menu_item_ids::insert_into_slot_4:
			select_and_insert_cartridge(menu_item_id - menu_item_ids::insert_into_slot_4);
			return;

		case menu_item_ids::eject_slot_1:
			eject_cartridge(0, true);
			return;

		case menu_item_ids::eject_slot_2:
			eject_cartridge(1, true);
			return;

		case menu_item_ids::eject_slot_3:
			eject_cartridge(2, true);
			return;

		case menu_item_ids::eject_slot_4:
			eject_cartridge(3, true);
			return;
		}


		if (menu_item_id >= 20 && menu_item_id <= 40)
		{
			cartridges_[0]->menu_item_clicked(menu_item_id - 20);
		}

		if (menu_item_id > 40 && menu_item_id <= 60)
		{
			cartridges_[1]->menu_item_clicked(menu_item_id - 40);
		}

		if (menu_item_id > 60 && menu_item_id <= 80)
		{
			cartridges_[2]->menu_item_clicked(menu_item_id - 60);
		}

		if (menu_item_id > 80 && menu_item_id <= 100)
		{
			cartridges_[3]->menu_item_clicked(menu_item_id - 80);
		}
	}


	multipak_cartridge::menu_item_collection_type multipak_cartridge::get_menu_items() const
	{
		::vcc::ui::menu::menu_builder menu;

		// Build the menus items for all of the inserted cartridges and add them to beginning.
		for (int slot(cartridges_.size() - 1); slot >= 0; slot--)
		{
			if (const auto& items(cartridges_[slot]->get_menu_items()); !items.empty())
			{
				if (!menu.empty())
				{
					menu.add_root_separator();
				}

				// FIXME-CHET: Check for menu item ids that exceed max id value
				menu.add_items(
					items,
					cartridges_menu::base_menu_id + slot * cartridges_menu::menu_id_count);
			}
		}

		if (!menu.empty())
		{
			menu.add_root_separator();
		}

		// Add the multipak menu items for insert/eject slot.
		for (auto slot(driver_->slot_count()); slot > 0; --slot)
		{
			const auto index(slot - 1);
			const bool is_at_selected_slot(driver_->selected_switch_slot() == index);
			const bool is_slot_occupied(!driver_->empty(index));
			const auto icon(::vcc::utils::load_shared_bitmap(
				module_instance_,
				get_icon_id(is_at_selected_slot, is_slot_occupied)));
			const auto& command_ids(slot_action_command_ids_[index]);

			menu
				.add_root_submenu(
					std::format("Multi-Pak Slot {}{}", slot, is_at_selected_slot ? " (selected)" : ""),
					icon)
				.add_submenu_item(
					command_ids.select,
					"Switch To",
					nullptr,
					is_at_selected_slot)
				.add_submenu_item(
					command_ids.insert,
					is_slot_occupied ? "Insert different Cartridge or ROM Pak" : "Insert Cartridge or ROM Pak")
				.add_submenu_item(
					command_ids.eject,
					"Eject " + driver_->slot_name(index),
					nullptr,
					driver_->empty(index));
		}

		menu.add_root_item(menu_item_ids::open_settings, "Open Multi-Pak Slot Manager");

		return menu.release_items();
	}

	void multipak_cartridge::switch_to_slot(slot_id_type slot)
	{
		driver_->switch_to_slot(slot);
		configuration_->selected_slot(slot);
		settings_dialog_.update_selected_slot();
	}

	void multipak_cartridge::select_and_insert_cartridge(slot_id_type slot)
	{
		const auto callback = [this, &slot](const std::string& filename)
			{
				return insert_cartridge(slot, filename, true);
			};

		::vcc::utils::select_cartridge_file(
			GetActiveWindow(),
			std::format("Insert Cartridge or ROM Pak into slot {}", slot + 1),
			configuration_->last_accessed_path(),
			callback);
	}

	multipak_cartridge::mount_status_type multipak_cartridge::insert_cartridge(
		slot_id_type slot,
		const path_type& filename,
		bool update_settings)
	{
		auto loadedCartridge(vcc::utils::load_cartridge(
			filename,
			host_,
			std::make_unique<multipak_expansion_port_ui>(),
			std::make_unique<multipak_expansion_port_bus>(bus_, slot, *driver_)));
		if (loadedCartridge.load_result != mount_status_type::success)
		{
			return loadedCartridge.load_result;
		}

		if (update_settings)
		{
			configuration_->slot_path(slot, filename);
			configuration_->last_accessed_path(::vcc::utils::get_directory_from_path(filename));
		}

		eject_cartridge(slot, false);

		cartridges_[slot] = move(loadedCartridge.cartridge);
		driver_->insert_cartridge(
			slot,
			move(loadedCartridge.handle),
			cartridges_[slot]);

		settings_dialog_.update_slot_details(slot);

		return loadedCartridge.load_result;
	}

	void multipak_cartridge::eject_cartridge(slot_id_type slot, bool update_settings)
	{
		if (update_settings)
		{
			configuration_->slot_path(slot, {});
		}

		// FIXME: eject_cartridge indirectly creates an empty cartridge as a placeholder.
		// We should be using that same one here. Might make more sense to handle this
		// through insert/eject events from the driver.
		cartridges_[slot] = std::make_shared<::vcc::bus::cartridges::empty_cartridge>();
		driver_->eject_cartridge(slot);

		settings_dialog_.update_slot_details(slot);
	}

}
