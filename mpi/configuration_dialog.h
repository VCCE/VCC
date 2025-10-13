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
#include <Windows.h>


class configuration_dialog
{
public:

	explicit configuration_dialog(multipak_cartridge& mpi);

	configuration_dialog(const configuration_dialog&) = delete;
	configuration_dialog(configuration_dialog&&) = delete;

	configuration_dialog& operator=(const configuration_dialog&) = delete;
	configuration_dialog& operator=(configuration_dialog&&) = delete;

	void open();
	void close();


private:

	void select_new_cartridge(size_t slot);
	void set_selected_slot(size_t slot);
	void update_slot_details(size_t slot);
	void eject_or_select_new_cartridge(size_t slot);

	static INT_PTR CALLBACK process_message(
		HWND hDlg,
		UINT message,
		WPARAM wParam,
		LPARAM lParam);


private:

	multipak_cartridge& mpi_;
	HWND dialog_handle_ = nullptr;
	HWND parent_handle_ = nullptr;
};
