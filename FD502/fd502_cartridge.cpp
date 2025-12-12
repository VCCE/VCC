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
#include "fd502_cartridge.h"
#include "wd1793.h"
#include "resource.h"
#include "create_disk_image_dialog.h"
#include <vcc/ui/select_file_dialog.h>
#include <vcc/ui/menu/menu_builder.h>
#include <vcc/utils/disk_image_loader.h>
#include <vcc/utils/winapi.h>
#include <vcc/utils/persistent_value_store.h>
#include <vcc/utils/filesystem.h>


namespace vcc::cartridges::fd502
{

	namespace
	{

		struct cartridge_menu_item_ids
		{
			UINT insert;
			UINT eject;
			UINT insert_next;
		};

		struct menu_item_ids
		{
			static const UINT open_settings = 1;

			// TODO-CHET: when documenting note these need to be sequential
			static const UINT insert_drive_0 = 2;
			static const UINT insert_drive_1 = 3;
			static const UINT insert_drive_2 = 4;
			static const UINT insert_drive_3 = 5;

			// TODO-CHET: when documenting note these need to be sequential
			static const UINT eject_drive_0 = 6;
			static const UINT eject_drive_1 = 7;
			static const UINT eject_drive_2 = 8;
			static const UINT eject_drive_3 = 9;

			// TODO-CHET: when documenting note these need to be sequential
			static const UINT insert_next_drive_0 = 10;
			static const UINT insert_next_drive_1 = 11;
			static const UINT insert_next_drive_2 = 12;
			static const UINT insert_next_drive_3 = 13;
		};

		// FIXME-CHET: rename
		const std::array<cartridge_menu_item_ids, 4> disk_drive_menu_items_ = { {
			{ menu_item_ids::insert_drive_0, menu_item_ids::eject_drive_0, menu_item_ids::insert_next_drive_0 },
			{ menu_item_ids::insert_drive_1, menu_item_ids::eject_drive_1, menu_item_ids::insert_next_drive_1 },
			{ menu_item_ids::insert_drive_2, menu_item_ids::eject_drive_2, menu_item_ids::insert_next_drive_2 },
			{ menu_item_ids::insert_drive_3, menu_item_ids::eject_drive_3, menu_item_ids::insert_next_drive_3 }
		} };

	}


	fd502_cartridge::fd502_cartridge(
		std::shared_ptr<expansion_port_host_type> host,
		std::unique_ptr<expansion_port_ui_type> ui,
		std::shared_ptr<expansion_port_bus_type> bus,
		std::unique_ptr<driver_type> driver,
		std::unique_ptr<configuration_type> configuration,
		HINSTANCE module_instance)
		:
		host_(host),
		ui_(move(ui)),
		bus_(bus),
		driver_(move(driver)),
		configuration_(move(configuration)),
		module_instance_(module_instance),
		settings_dialog_(
			module_instance_,
			configuration_,
			driver_,
			std::bind(&fd502_cartridge::settings_changed, this))
	{
		if (host_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct FD502 Cartridge. Host is null.");
		}

		if (ui_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct FD502 Cartridge. UI Service is null.");
		}

		if (bus_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct FD502 Cartridge. Bus is null.");
		}

		if (configuration_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct FD502 Cartridge. Configuration is null.");
		}

		if (driver_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct FD502 Cartridge. Driver is null.");
		}

		if (module_instance_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct FD502 Cartridge. Module handle is null.");
		}
	}


	fd502_cartridge::name_type fd502_cartridge::name() const
	{
		return ::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME);
	}

	fd502_cartridge::driver_type& fd502_cartridge::driver()
	{
		return *driver_;
	}


	void fd502_cartridge::start()
	{
		driver_->start(
			load_rom(configuration_->get_rom_image_id()),
			configuration_->is_turbo_mode_enabled(),
			configuration_->is_rtc_enabled(),
			configuration_->is_becker_port_enabled(),
			configuration_->becker_port_server_address(),
			configuration_->becker_port_server_port());

		if (configuration_->serialize_drive_mount_settings())
		{
			for (auto drive_id(0u); drive_id < driver_->drive_count(); ++drive_id)
			{
				if (const auto mount_path(configuration_->disk_image_path(drive_id));
					!mount_path.empty())
				{
					auto disk_image(::vcc::utils::load_disk_image(mount_path));
					if (!disk_image)
					{
						// TODO-CHET: handle error
						continue;
					}

					driver_->insert_disk(drive_id, move(disk_image), mount_path);
				}
			}
		}
	}

	void fd502_cartridge::stop()
	{
		settings_dialog_.close();
		driver_->stop();
	}


	fd502_cartridge::status_type fd502_cartridge::status() const
	{
		status_type text_buffer;
		if (const auto current_disk_index_(driver_->get_selected_drive_id());
			current_disk_index_.has_value() && driver_->is_motor_running())
		{
			text_buffer = std::format(
				"FD-502: Drive: {} Trk: {} Sec: {} Hd: {}",
				current_disk_index_.value(),
				driver_->get_head_position(),
				driver_->get_current_sector(),
				driver_->get_selected_head());
		}
		else
		{
			text_buffer = "FD-502 Idle";
		}

		//char beckerstat[64];
		//if (becker_port_.enabled())
		//{
		//	becker_port_.status(beckerstat);
		//	strcat(text_buffer, " | ");
		//	strcat(text_buffer, beckerstat);
		//}

		return text_buffer;
	}

	fd502_cartridge::string_type fd502_cartridge::format_next_disk_image_menu_text(
		drive_id_type drive_index,
		const std::filesystem::path& path) const
	{
		return std::format(
			"Insert next disk `{}` into drive {}",
			path.filename().string(),
			drive_index);
	}

	fd502_cartridge::menu_item_collection_type fd502_cartridge::get_menu_items() const
	{
		::vcc::ui::menu::menu_builder builder;

		// Add 'insert next disk' priority items first
		for (auto drive_index(0u); drive_index < disk_drive_menu_items_.size(); ++drive_index)
		{
			const auto next_disk_path = get_next_disk_image_for_drive(drive_index);
			if (next_disk_path.empty())
			{
				continue;
			}

			builder.add_root_item(
				disk_drive_menu_items_[drive_index].insert_next,
				format_next_disk_image_menu_text(drive_index, next_disk_path));
		}

		// Add floppy drive mounting quick access sub-menus
		for (auto drive_index(0u); drive_index < disk_drive_menu_items_.size(); ++drive_index)
		{
			const auto& ids(disk_drive_menu_items_[drive_index]);
			const auto& disk_image_path(driver_->get_mounted_disk_filename(drive_index));
			const auto& name(disk_image_path.filename());
			const bool is_empty(disk_image_path.empty());
			const auto icon_id(
				driver_->is_disk_write_protected(drive_index)
				? IDB_DISK_WRITE_PROTECTED
				: IDB_DISK_WRITABLE);
			const auto bitmap(
				is_empty
				? nullptr
				: ::vcc::utils::load_shared_bitmap(module_instance_, icon_id, true));

			builder.add_root_submenu(std::format("Floppy Drive {}", drive_index), bitmap);

			if (const auto& next_disk_path(get_next_disk_image_for_drive(drive_index)); !next_disk_path.empty())
			{
				builder.add_submenu_item(
					ids.insert_next,
					format_next_disk_image_menu_text(drive_index, next_disk_path));
			}

			builder
				.add_submenu_item(
					ids.insert,
					std::format("Insert{} disk", name.empty() ? "" : " a different"))
				.add_submenu_item(
					ids.eject,
					is_empty ? "Eject" : std::format("Eject `{}`", name.string()),
					nullptr,
					is_empty);
		}

		return builder
			.add_root_item(menu_item_ids::open_settings, "Open FD-502 Cartridge Settings")
			.release_items();
	}

	void fd502_cartridge::menu_item_clicked(menu_item_id_type MenuID)
	{
		switch (MenuID)
		{
		case menu_item_ids::open_settings:
			settings_dialog_.open(ui_->app_window());
			break;

		case menu_item_ids::insert_drive_0:
		case menu_item_ids::insert_drive_1:
		case menu_item_ids::insert_drive_2:
		case menu_item_ids::insert_drive_3:
			insert_disk(MenuID - menu_item_ids::insert_drive_0);
			break;

		case menu_item_ids::eject_drive_0:
		case menu_item_ids::eject_drive_1:
		case menu_item_ids::eject_drive_2:
		case menu_item_ids::eject_drive_3:
			eject_disk(MenuID - menu_item_ids::eject_drive_0);
			break;

		case menu_item_ids::insert_next_drive_0:
		case menu_item_ids::insert_next_drive_1:
		case menu_item_ids::insert_next_drive_2:
		case menu_item_ids::insert_next_drive_3:
			insert_next_disk(MenuID - menu_item_ids::insert_next_drive_0);
		}
	}


	void fd502_cartridge::insert_disk(drive_id_type drive_id)
	{
		HWND h_own = ui_->app_window();
		::vcc::ui::select_file_dialog insert_disk_dialog;
		insert_disk_dialog.set_title(std::format("Insert Disk Into Drive {}", drive_id))
			.set_initial_directory(configuration_->disk_image_directory())
			.set_selection_filter({ {"Disk Images", {"*.dsk", "*.os9"} } })
			.set_default_extension("dsk")
			.append_flags(OFN_PATHMUSTEXIST);
		if (insert_disk_dialog.do_modal_load_dialog(h_own))
		{
			const auto& selected_path(insert_disk_dialog.selected_path());
			if (!std::filesystem::exists(selected_path))
			{
				if (create_disk_image_dialog(module_instance_, selected_path).do_modal(h_own) != IDOK)
				{
					return;
				}
			}

			if (!std::filesystem::exists(selected_path))
			{
				// TODO-CHET: Add real error message
				MessageBox(h_own, "File does not exist", "Error", 0);
				return;
			}

			insert_disk(drive_id, selected_path);
		}
	}

	void fd502_cartridge::insert_disk(drive_id_type drive_id, const path_type& disk_image_path)
	{
		// Attempt mount if file existed or file create was not canceled
		auto disk_image(::vcc::utils::load_disk_image(disk_image_path));
		if (!disk_image)
		{
			// TODO-CHET: Add real error message
			MessageBox(ui_->app_window(), "Can't open file", "Error", 0);
			return;
		}

		driver_->insert_disk(drive_id, move(disk_image), disk_image_path);
		// TODO-CHET: Maybe this should be handled in the configuration. This way it can
		// keep track of any new mount changes and update accordingly (i.e. turning them
		// on needs to save all the new mounts).
		if (configuration_->serialize_drive_mount_settings())
		{
			configuration_->set_disk_image_path(drive_id, disk_image_path);
			configuration_->set_disk_image_directory(disk_image_path.parent_path());
		}
	}

	void fd502_cartridge::insert_next_disk(drive_id_type drive_id)
	{
		const auto next_disk_path = get_next_disk_image_for_drive(drive_id);
		if (next_disk_path.empty())
		{
			return;
		}

		insert_disk(drive_id, next_disk_path);
	}

	void fd502_cartridge::eject_disk(drive_id_type drive_id)
	{
		driver_->eject_disk(drive_id);
		if (configuration_->serialize_drive_mount_settings())
		{
			configuration_->set_disk_image_path(drive_id, {});
		}
	}


	std::unique_ptr<fd502_cartridge::rom_image_type> fd502_cartridge::load_rom(rom_image_id_type rom_id) const
	{
		auto rom_image(std::make_unique<rom_image_type>());

		switch (rom_id)
		{
		case rom_image_id_type::custom:
			// TODO-CHET: Check for errors
			(void)rom_image->load(configuration_->rom_image_path());
			break;

		case rom_image_id_type::microsoft:
			// TODO-CHET: Check for errors
			(void)rom_image->load(host_->system_rom_path().append("disk11.rom"));
			break;

		case rom_image_id_type::rgbdos:
			// TODO-CHET: Check for errors
			(void)rom_image->load(host_->system_rom_path().append("rgbdos.rom"));
			break;

		default:
			throw std::runtime_error("Unable to load ROM. Unknown ROM ID.");
		}

		return rom_image;
	}

	void fd502_cartridge::settings_changed()
	{
		driver_->set_rom(load_rom(configuration_->get_rom_image_id()));
		driver_->set_turbo_mode(configuration_->is_turbo_mode_enabled());
		driver_->enable_rtc_device(configuration_->is_rtc_enabled());
		driver_->update_becker_port_settings(
			configuration_->is_becker_port_enabled(),
			configuration_->becker_port_server_address(),
			configuration_->becker_port_server_port());
	}


	fd502_cartridge::path_type fd502_cartridge::get_next_disk_image_for_drive(drive_id_type drive_index) const
	{
		const auto& path(std::filesystem::path(driver_->get_mounted_disk_filename(drive_index)));
		auto stem(path.stem().string());
		if (stem.empty())
		{
			return {};
		}

		auto& last_char(stem.back());
		if (!isdigit(last_char))
		{
			return {};
		}
		++last_char;

		const auto& extension(path.extension());
		const auto& directory(path.parent_path());

		auto next_disk(directory);
		next_disk.append(stem + extension.string());

		if (!std::filesystem::exists(next_disk))
		{
			return {};
		}

		return next_disk;

	}
}
