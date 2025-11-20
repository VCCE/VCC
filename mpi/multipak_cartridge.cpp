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
#include "configuration_dialog.h"
#include "multipak_configuration.h"
#include "vcc/bus/cartridge.h"
#include <array>
#include "multipak_expansion_port_ui.h"
#include "multipak_expansion_port_bus.h"
#include <vcc/utils/filesystem.h>
#include <vcc/ui/menu/menu_builder.h>
#include <vcc/utils/winapi.h>
#include "resource.h"
#include <vcc/bus/cartridges/empty_cartridge.h>


multipak_cartridge::multipak_cartridge(
	std::shared_ptr<expansion_port_host_type> host,
	std::shared_ptr<expansion_port_ui_type> ui,
	std::shared_ptr<expansion_port_bus_type> bus,
	std::shared_ptr<multipak_device> device,
	HINSTANCE module_instance,
	multipak_configuration& configuration)
	:
	host_(host),
	ui_(ui),
	bus_(bus),
	device_(device),
	module_instance_(module_instance),
	configuration_(configuration),
	settings_dialog_(
		module_instance,
		configuration,
		device_,
		std::bind(&multipak_cartridge::insert_cartridge, this, std::placeholders::_1, std::placeholders::_2),
		std::bind(&multipak_cartridge::eject_cartridge, this, std::placeholders::_1))
{
	for (auto& controller : cartridges_)
	{
		controller = std::make_shared<::vcc::bus::cartridges::empty_cartridge>();
	}
}


multipak_cartridge::name_type multipak_cartridge::name() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME);
}

multipak_cartridge::catalog_id_type multipak_cartridge::catalog_id() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_CATNUMBER);
}

multipak_cartridge::description_type multipak_cartridge::description() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_DESCRIPTION);
}

multipak_cartridge::device_type& multipak_cartridge::device()
{
	return *device_;
}


void multipak_cartridge::start()
{
	device_->start();

	for (auto slot(0u); slot < device_->slot_count(); slot++)
	{
		const auto path(vcc::utils::find_pak_module_path(configuration_.slot_cartridge_path(slot)));
		if (!path.empty())
		{
			// FIXME-CHET: Check return value and act on it
			insert_cartridge(slot, path);
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
	device_->stop();
}

void multipak_cartridge::status(char* text_buffer, size_t buffer_size)
{
	char TempStatus[64] = "";

	sprintf(text_buffer, "MPI:%d,%d", device_->selected_cts_slot() + 1, device_->selected_scs_slot() + 1);
	for (auto slot(0u); slot < cartridges_.size(); ++slot)
	{
		strcpy(TempStatus, "");
		cartridges_[slot]->status(TempStatus, sizeof(TempStatus));
		if (TempStatus[0])
		{
			strcat(text_buffer, " | ");
			strcat(text_buffer, TempStatus);
		}
	}
}

void multipak_cartridge::menu_item_clicked(unsigned char menu_item_id)
{
	if (menu_item_id == 19)
	{
		settings_dialog_.open();
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
	::vcc::ui::menu::menu_builder builder;

	for (int slot(cartridges_.size() - 1); slot >= 0; slot--)
	{
		if (const auto& items(cartridges_[slot]->get_menu_items()); !items.empty())
		{
			if (!builder.empty())
			{
				builder.add_root_separator();
			}

			builder.add_items(
				items,
				expansion_port_base_menu_id + slot * expansion_port_menu_id_range_size);
		}
	}

	if (!builder.empty())
	{
		builder.add_root_separator();
	}

	builder.add_root_item(19, "MPI Config");

	return builder.release_items();
}


multipak_cartridge::mount_status_type multipak_cartridge::insert_cartridge(
	slot_id_type slot,
	const path_type& filename)
{
	auto loadedCartridge(vcc::utils::load_cartridge(
		filename,
		host_,
		std::make_unique<multipak_expansion_port_ui>(),
		std::make_unique<multipak_expansion_port_bus>(slot, bus_, *device_)));
	if (loadedCartridge.load_result != mount_status_type::success)
	{
		return loadedCartridge.load_result;
	}

	cartridges_[slot] = move(loadedCartridge.cartridge);
	device_->insert_cartridge(
		slot,
		move(loadedCartridge.handle),
		cartridges_[slot]);

	return loadedCartridge.load_result;
}

void multipak_cartridge::eject_cartridge(slot_id_type slot)
{
	device_->eject_cartridge(slot);
	cartridges_[slot] = std::make_shared<::vcc::bus::cartridges::empty_cartridge>();
}
