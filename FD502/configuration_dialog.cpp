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
#include "configuration_dialog.h"
#include "resource.h"
#include <vcc/common/DialogOps.h>
#include <vcc/ui/select_file_dialog.h>
#include <vcc/utils/winapi.h>


namespace vcc::cartridges::fd502
{

	namespace
	{

		struct rom_image_ui_details
		{
			configuration_dialog::rom_image_id_type rom_id;
			UINT control_id;
		};

		const std::array<rom_image_ui_details, 3> rom_id_to_control_id_map_ =
		{ {
			{ configuration_dialog::rom_image_id_type::custom, IDC_SETTINGS_ROM_CUSTOM },
			{ configuration_dialog::rom_image_id_type::microsoft, IDC_SETTINGS_ROM_RSDOS },
			{ configuration_dialog::rom_image_id_type::rgbdos, IDC_SETTINGS_ROM_RGBDOS }
		} };

	}


	configuration_dialog::configuration_dialog(
		// TODO-CHET: globally rename module_handle and module_instance to native_library_handle
		HINSTANCE module_handle,
		std::shared_ptr<fd502_configuration> configuration,
		std::shared_ptr<fd502_cartridge_driver> driver,
		settings_changed_function_type settings_changed)
		:
		dialog_window(module_handle, IDD_SETTINGS),
		configuration_(move(configuration)),
		driver_(move(driver)),
		settings_changed_(move(settings_changed))
	{
		if (configuration_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct FD502 Configuration Dialog. Configuration is null.");
		}

		if (driver_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct FD502 Configuration Dialog. Driver is null.");
		}
	}


	INT_PTR configuration_dialog::on_command(WPARAM wParam, [[maybe_unused]] LPARAM lParam)
	{
		switch (LOWORD(wParam))
		{
		case IDC_SETTINGS_ROM_CUSTOM:
			selected_rom_id_ = rom_image_id_type::custom;
			break;

		case IDC_SETTINGS_ROM_RSDOS:
			selected_rom_id_ = rom_image_id_type::microsoft;
			break;

		case IDC_SETTINGS_ROM_RGBDOS:
			selected_rom_id_ = rom_image_id_type::rgbdos;
			break;

		case IDC_SETTINGS_ROM_CUSTOM_BROWSE:
			on_browse_for_rom_image();
			break;
		}

		// Update control states
		enable_control(IDC_SETTINGS_ROM_CUSTOM_BROWSE, selected_rom_id_ == rom_image_id_type::custom);

		return FALSE;
	}


	bool configuration_dialog::on_init_dialog()
	{
		// We don't need the return value as we always return true so keyboard focus
		// is given to the default control (if there is one).
		(void)dialog_window::on_init_dialog();

		selected_rom_id_ = configuration_->get_rom_image_id();

		CenterDialog(handle());

		set_button_check(IDC_SETTINGS_GENERAL_ENABLE_RTC, configuration_->is_rtc_enabled());
		set_button_check(IDC_SETTINGS_GENERAL_TURBO, configuration_->is_turbo_mode_enabled());
		set_button_check(IDC_SETTINGS_GENERAL_SAVEMOUNTS, configuration_->serialize_drive_mount_settings());
		for (const auto& [rom_id, control_id] : rom_id_to_control_id_map_)
		{
			set_button_check(control_id, rom_id == selected_rom_id_);
		}

		// TODO-CHET: this is a duplicate of whats in the command handler. consolidate into update_control_states()
		enable_control(IDC_SETTINGS_ROM_CUSTOM_BROWSE, selected_rom_id_ == rom_image_id_type::custom);

		set_control_text(IDC_SETTINGS_ROM_CUSTOM_PATH, configuration_->rom_image_path());

		set_button_check(IDC_SETTINGS_BECKER_ENAB, configuration_->is_becker_port_enabled());
		set_control_text(IDC_SETTINGS_BECKER_HOST, configuration_->becker_port_server_address());
		set_control_text(IDC_SETTINGS_BECKER_PORT, configuration_->becker_port_server_port());

		return true;
	}

	void configuration_dialog::on_ok()
	{
		configuration_->enable_rtc(is_button_checked(IDC_SETTINGS_GENERAL_ENABLE_RTC));
		configuration_->set_turbo_mode(is_button_checked(IDC_SETTINGS_GENERAL_TURBO));
		configuration_->set_serialize_drive_mount_settings(is_button_checked(IDC_SETTINGS_GENERAL_SAVEMOUNTS));
		configuration_->set_rom_image_id(selected_rom_id_);
		configuration_->set_rom_image_path(get_control_text(IDC_SETTINGS_ROM_CUSTOM_PATH));
		configuration_->enable_becker_port(is_button_checked(IDC_SETTINGS_BECKER_ENAB));
		configuration_->becker_port_server_address(get_control_text(IDC_SETTINGS_BECKER_HOST));
		configuration_->becker_port_server_port(get_control_text(IDC_SETTINGS_BECKER_PORT));

		settings_changed_();

		dialog_window::on_ok();
	}


	void configuration_dialog::on_browse_for_rom_image()
	{
		::vcc::ui::select_file_dialog select_file_dialog;

		select_file_dialog
			.set_selection_filter({ {"ROM Images", {"*.rom", "*.ccc", "*.pak"} } })
			.set_title("Select Disk BASIC ROM Image")
			.set_initial_directory(configuration_->rom_image_directory().parent_path());
		if (select_file_dialog.do_modal_load_dialog(handle()))
		{
			// FIXME-CHET: Set the new ROM directory.
			set_control_text(IDC_SETTINGS_ROM_CUSTOM_PATH, select_file_dialog.selected_path());
		}
	}

}
