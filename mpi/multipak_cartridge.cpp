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
			menu_item_ids::select_slot_1_and_reset,
			menu_item_ids::insert_into_slot_1,
			menu_item_ids::eject_slot_1},

		slot_action_command_descriptor{
			menu_item_ids::select_slot_2,
			menu_item_ids::select_slot_2_and_reset,
			menu_item_ids::insert_into_slot_2,
			menu_item_ids::eject_slot_2},

		slot_action_command_descriptor{
			menu_item_ids::select_slot_3,
			menu_item_ids::select_slot_3_and_reset,
			menu_item_ids::insert_into_slot_3,
			menu_item_ids::eject_slot_3},

		slot_action_command_descriptor{
			menu_item_ids::select_slot_4,
			menu_item_ids::select_slot_4_and_reset,
			menu_item_ids::insert_into_slot_4,
			menu_item_ids::eject_slot_4},
	};


	multipak_cartridge::multipak_cartridge(
		std::shared_ptr<expansion_port_host_type> host,
		std::shared_ptr<expansion_port_ui_type> ui,
		std::shared_ptr<expansion_port_bus_type> bus,
		std::shared_ptr<multipak_cartridge_driver> driver,
		// TODO-CHET: globally rename module_instance to library_handle
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
			bus,
			ui,
			std::bind(&multipak_cartridge::select_and_insert_cartridge, this, std::placeholders::_1),
			std::bind(&multipak_cartridge::eject_cartridge, this, std::placeholders::_1, true, true))
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
			const auto path(vcc::utils::find_pak_module_path(configuration_->slot_path(slot).string()));
			if (!path.empty())
			{
				if (const auto insert_result(insert_cartridge(slot, path, false, false));
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

	multipak_cartridge::status_type multipak_cartridge::status() const
	{
		auto status(std::format("MPI:{},{}", driver_->selected_cts_slot() + 1, driver_->selected_scs_slot() + 1));
		for (const auto& cartridge : cartridges_)
		{
			auto cart_status(cartridge->status());
			if (!cart_status.empty())
			{
				status += " | ";
				status += cart_status;
			}
		}

		return status;
	}

	void multipak_cartridge::menu_item_clicked(menu_item_id_type menu_item_id)
	{
		if (menu_item_id < cartridges_menu::first_item_id)
		{
			return multipak_menu_item_clicked(menu_item_id);
		}

		menu_item_id -= cartridges_menu::first_item_id;

		const auto cartridge_id(menu_item_id / cartridges_menu::item_count);
		if (cartridge_id >= cartridges_.size())
		{
			throw std::out_of_range("Cannot process menu item. Item id is out of range.");
		}

		menu_item_id %= cartridges_menu::item_count;

		cartridges_[cartridge_id]->menu_item_clicked(menu_item_id);
	}

	void multipak_cartridge::multipak_menu_item_clicked(menu_item_id_type menu_item_id)
	{
		switch (menu_item_id)
		{
		case menu_item_ids::open_settings:
			settings_dialog_.open();
			break;

		case menu_item_ids::select_slot_1:
		case menu_item_ids::select_slot_2:
		case menu_item_ids::select_slot_3:
		case menu_item_ids::select_slot_4:
			switch_to_slot(menu_item_id - menu_item_ids::select_slot_1, false);
			break;

		case menu_item_ids::select_slot_1_and_reset:
		case menu_item_ids::select_slot_2_and_reset:
		case menu_item_ids::select_slot_3_and_reset:
		case menu_item_ids::select_slot_4_and_reset:
			switch_to_slot(menu_item_id - menu_item_ids::select_slot_1_and_reset, true);
			break;

		case menu_item_ids::insert_into_slot_1:
		case menu_item_ids::insert_into_slot_2:
		case menu_item_ids::insert_into_slot_3:
		case menu_item_ids::insert_into_slot_4:
			select_and_insert_cartridge(menu_item_id - menu_item_ids::insert_into_slot_1);
			break;

		case menu_item_ids::eject_slot_1:
		case menu_item_ids::eject_slot_2:
		case menu_item_ids::eject_slot_3:
		case menu_item_ids::eject_slot_4:
			eject_cartridge(menu_item_id - menu_item_ids::eject_slot_1, true, true);
			break;
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
					cartridges_menu::first_item_id + slot * cartridges_menu::item_count);
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
					command_ids.insert,
					is_slot_occupied ? "Insert different Cartridge or ROM Pak" : "Insert Cartridge or ROM Pak")
				.add_submenu_item(
					command_ids.eject,
					"Eject " + driver_->slot_name(index),
					nullptr,
					driver_->empty(index))
				.add_submenu_item(
					command_ids.select,
					"Switch To",
					nullptr,
					is_at_selected_slot)
				.add_submenu_item(
					command_ids.select_and_reset,
					"Switch To And Reset",
					nullptr,
					is_at_selected_slot);
		}

		menu.add_root_item(menu_item_ids::open_settings, "Open Multi-Pak Slot Manager");

		return menu.release_items();
	}

	void multipak_cartridge::switch_to_slot(slot_id_type slot, bool reset)
	{
		driver_->switch_to_slot(slot);
		configuration_->selected_slot(slot);
		settings_dialog_.update_selected_slot();
		if (reset)
		{
			bus_->reset();
		}
	}

	void multipak_cartridge::select_and_insert_cartridge(slot_id_type slot)
	{
		const auto callback = [this, &slot](const std::string& filename)
			{
				return insert_cartridge(slot, filename, true, true);
			};

		::vcc::utils::select_cartridge_file(
			ui_->app_window(),
			std::format("Insert Cartridge or ROM Pak into slot {}", slot + 1),
			configuration_->last_accessed_path(),
			callback);
	}

	multipak_cartridge::mount_status_type multipak_cartridge::insert_cartridge(
		slot_id_type slot,
		const path_type& filename,
		bool update_settings,
		bool allow_reset)
	{
		auto loadedCartridge(vcc::utils::load_cartridge(
			filename,
			host_,
			std::make_unique<multipak_expansion_port_ui>(ui_),
			std::make_unique<multipak_expansion_port_bus>(bus_, slot, *driver_)));
		if (loadedCartridge.load_result != mount_status_type::success)
		{
			return loadedCartridge.load_result;
		}

		if (update_settings)
		{
			configuration_->slot_path(slot, filename);
			configuration_->last_accessed_path(filename.parent_path());
		}

		eject_cartridge(slot, false, false);

		cartridges_[slot] = move(loadedCartridge.cartridge);
		driver_->insert_cartridge(
			slot,
			move(loadedCartridge.handle),
			cartridges_[slot],
			allow_reset);

		settings_dialog_.update_slot_details(slot);

		return loadedCartridge.load_result;
	}

	void multipak_cartridge::eject_cartridge(slot_id_type slot, bool update_settings, bool allow_reset)
	{
		if (update_settings)
		{
			configuration_->slot_path(slot, {});
		}

		// FIXME: eject_cartridge indirectly creates an empty cartridge as a placeholder.
		// We should be using that same one here. Might make more sense to handle this
		// through insert/eject events from the driver.
		cartridges_[slot] = std::make_shared<::vcc::bus::cartridges::empty_cartridge>();
		driver_->eject_cartridge(slot, allow_reset);

		settings_dialog_.update_slot_details(slot);
	}

}
