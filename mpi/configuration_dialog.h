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
#include "multipak_cartridge_driver.h"
#include "vcc/utils/cartridge_loader.h"
#include <Windows.h>
#include <functional>


class configuration_dialog
{
public:

	using cartridge_type = ::vcc::bus::cartridge;
	using slot_id_type = ::multipak_cartridge_driver::slot_id_type;
	using insert_cartridge_status_type = ::vcc::utils::cartridge_loader_status;
	using path_type = std::string;
	using insert_function_type = std::function<insert_cartridge_status_type(slot_id_type, const path_type&)>;
	using eject_function_type = std::function<void(slot_id_type)>;


public:

	configuration_dialog(
		HINSTANCE module_handle,
		multipak_configuration& configuration,
		std::shared_ptr<multipak_cartridge_driver> mpi,
		insert_function_type insert_callback,
		eject_function_type eject_callback);

	configuration_dialog(const configuration_dialog&) = delete;
	configuration_dialog(configuration_dialog&&) = delete;

	configuration_dialog& operator=(const configuration_dialog&) = delete;
	configuration_dialog& operator=(configuration_dialog&&) = delete;

	void open();
	void close();


private:


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
	const std::shared_ptr<multipak_cartridge_driver> mpi_;
	const insert_function_type insert_callback_;
	const eject_function_type eject_callback_;
	HWND dialog_handle_ = nullptr;
	HWND parent_handle_ = nullptr;	//	FIXME-CHET: This is only used for one thing. Delete it!
};
