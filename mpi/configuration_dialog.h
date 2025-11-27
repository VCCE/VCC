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
#include "multipak_configuration.h"
#include <vcc/utils/cartridge_loader.h>
#include <Windows.h>


class multipak_controller
{
public:

	using context_type = ::vcc::core::cartridge_context;
	using mount_status_type = ::vcc::utils::cartridge_loader_status;
	using slot_id_type = ::multipak_configuration::slot_id_type;
	using path_type = ::multipak_configuration::path_type;
	using label_type = ::std::string;
	using description_type = ::std::string;


public:

	virtual ~multipak_controller() = default;

	virtual label_type slot_label(slot_id_type slot) const = 0;
	virtual description_type slot_description(slot_id_type slot) const = 0;

	virtual bool empty(slot_id_type slot) const = 0;

	virtual void eject_cartridge(slot_id_type slot) = 0;
	virtual mount_status_type mount_cartridge(slot_id_type slot, const path_type& filename) = 0;

	virtual void switch_to_slot(slot_id_type slot) = 0;
	virtual slot_id_type selected_switch_slot() const = 0;
	virtual slot_id_type selected_scs_slot() const = 0;

	virtual void build_menu() = 0;
};


class configuration_dialog
{
public:

	configuration_dialog(
		HINSTANCE module_handle,
		multipak_configuration& configuration,
		multipak_controller& mpi);

	configuration_dialog(const configuration_dialog&) = delete;
	configuration_dialog(configuration_dialog&&) = delete;

	configuration_dialog& operator=(const configuration_dialog&) = delete;
	configuration_dialog& operator=(configuration_dialog&&) = delete;

	void open();
	void close();


private:

	using slot_id_type = ::multipak_controller::slot_id_type;

	void select_new_cartridge(slot_id_type slot);
	void set_selected_slot(slot_id_type slot);
	void update_slot_details(slot_id_type slot);
	void eject_or_select_new_cartridge(slot_id_type slot);

	static INT_PTR CALLBACK callback_procedure(
		HWND hDlg,
		UINT message,
		WPARAM wParam,
		LPARAM lParam);

	INT_PTR process_message(
		HWND hDlg,
		UINT message,
		WPARAM wParam);

private:

	const HINSTANCE module_handle_;
	multipak_configuration& configuration_;
	multipak_controller& mpi_;
	HWND dialog_handle_ = nullptr;
	HWND parent_handle_ = nullptr;
};
