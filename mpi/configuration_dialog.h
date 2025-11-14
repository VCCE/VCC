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


class configuration_dialog
{
public:

	void open();
	void close();


private:

	using slot_id_type = std::size_t;

	void select_new_cartridge(slot_id_type slot);
	void set_selected_slot(slot_id_type slot);
	void update_slot_details(slot_id_type slot);
	void eject_or_select_new_cartridge(slot_id_type slot);
	void display_slot_description(slot_id_type slot);

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

	HWND dialog_handle_ = nullptr;
	HWND parent_handle_ = nullptr;
};
