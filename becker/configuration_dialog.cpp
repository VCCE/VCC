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
#include "configuration_dialog.h"
#include "resource.h"
#include "vcc/common/DialogOps.h"
#include "vcc/utils/winapi.h"


configuration_dialog::configuration_dialog(
	HINSTANCE module_handle,
	controller_type& controller)
	:
	module_handle_(module_handle),
	controller_(controller)
{}


void configuration_dialog::open()
{
	if (!dialog_handle_)
	{
		dialog_handle_ = CreateDialogParam(
			module_handle_,
			MAKEINTRESOURCE(IDD_PROPPAGE),
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
	HWND hwndOwner;
	RECT rc, rcDlg, rcOwner;

	switch (message)
	{
	case WM_DESTROY:
		dialog_handle_ = nullptr;
		break;

	case WM_INITDIALOG:
		dialog_handle_ = hDlg;

		hwndOwner = GetParent(hDlg);
		if (hwndOwner == nullptr)
		{
			hwndOwner = GetDesktopWindow();
		}

		GetWindowRect(hwndOwner, &rcOwner);
		GetWindowRect(hDlg, &rcDlg);
		CopyRect(&rc, &rcOwner);

		OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
		OffsetRect(&rc, -rc.left, -rc.top);
		OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

		SetWindowPos(hDlg,
					 HWND_TOP,
					 rcOwner.left + (rc.right / 2),
					 rcOwner.top + (rc.bottom / 2),
					 0, 0,          // Ignores size arguments.
					 SWP_NOSIZE);

		SendDlgItemMessage(
			hDlg,
			IDC_TCPHOST,
			WM_SETTEXT,
			0,
			reinterpret_cast<LPARAM>(controller_.server_address().c_str()));
		SendDlgItemMessage(
			hDlg,
			IDC_TCPPORT,
			WM_SETTEXT,
			0,
			reinterpret_cast<LPARAM>(controller_.server_port().c_str()));
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			// Save config dialog data
			controller_.configure_server(
				::vcc::utils::get_dialog_item_text(hDlg, IDC_TCPHOST),
				::vcc::utils::get_dialog_item_text(hDlg, IDC_TCPPORT));
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;

		case IDHELP:
			return TRUE;

		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			break;
		}

		return TRUE;
	}

	return FALSE;
}
