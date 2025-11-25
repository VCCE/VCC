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
#include "vcc/common/DialogOps.h"
#include "vcc/utils/winapi.h"


configuration_dialog::configuration_dialog(
	HINSTANCE module_handle,
	update_connection_settings_type update_connection_settings)
	:
	module_handle_(module_handle),
	update_connection_settings(move(update_connection_settings))
{
}


void configuration_dialog::open(string_type server_address, string_type server_port)
{
	if (!dialog_handle_)
	{
		server_address_ = move(server_address);
		server_port_ = move(server_port);

		dialog_handle_ = CreateDialogParam(
			module_handle_,
			MAKEINTRESOURCE(IDD_SETTINGS_DIALOG),
			GetActiveWindow(),
			callback_procedure,
			reinterpret_cast<LPARAM>(this));
	}

	ShowWindow(dialog_handle_, SW_SHOWNORMAL);
}

void configuration_dialog::close()
{
	CloseCartDialog(dialog_handle_);
}


INT_PTR CALLBACK configuration_dialog::callback_procedure(
	HWND hDlg,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	if (message == WM_INITDIALOG)
	{
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
	}

	auto dialog(reinterpret_cast<configuration_dialog*>(GetWindowLongPtr(hDlg, GWLP_USERDATA)));

	return dialog->process_message(hDlg, message, wParam);
}

INT_PTR configuration_dialog::process_message(
	HWND hDlg,
	UINT message,
	WPARAM wParam)
{
	switch (message)
	{
	case WM_DESTROY:
		dialog_handle_ = nullptr;
		break;

	case WM_INITDIALOG:
		dialog_handle_ = hDlg;

		CenterDialog(hDlg);

		SendDlgItemMessage(
			hDlg,
			IDC_TCPHOST,
			WM_SETTEXT,
			0,
			reinterpret_cast<LPARAM>(server_address_.c_str()));
		SendDlgItemMessage(
			hDlg,
			IDC_TCPPORT,
			WM_SETTEXT,
			0,
			reinterpret_cast<LPARAM>(server_port_.c_str()));
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			// FIXME-CHET: This should validate that the address is in a valid format and
			// the port is an integer and in a valid range.
			update_connection_settings(
				::vcc::utils::get_dialog_item_text(hDlg, IDC_TCPHOST),
				::vcc::utils::get_dialog_item_text(hDlg, IDC_TCPPORT));
			EndDialog(hDlg, IDOK);
			break;

		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			break;
		}

		return TRUE;
	}

	return FALSE;
}
