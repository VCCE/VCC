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
#include "vcc/utils/critical_section.h"
#include "vcc/utils/filesystem.h"
#include <array>
#include <format>


namespace
{

	using cartridge_loader_status = vcc::utils::cartridge_loader_status;

	struct cartridge_ui_element_identifiers
	{
		UINT edit_box_id;
		UINT radio_button_id;
		UINT insert_button_id;
	};

	const std::array<cartridge_ui_element_identifiers, 4> gSlotUiElementIds = { {
		{ IDC_EDIT1, IDC_SELECT1, IDC_INSERT1 },
		{ IDC_EDIT2, IDC_SELECT2, IDC_INSERT2 },
		{ IDC_EDIT3, IDC_SELECT3, IDC_INSERT3 },
		{ IDC_EDIT4, IDC_SELECT4, IDC_INSERT4 }
	} };

}

configuration_dialog::configuration_dialog(
	HINSTANCE module_handle,
	multipak_configuration& configuration,
	std::shared_ptr<multipak_cartridge_driver> mpi,
	insert_function_type insert_callback,
	eject_function_type eject_callback)
	:
	module_handle_(module_handle),
	configuration_(configuration),
	mpi_(mpi),
	insert_callback_(move(insert_callback)),
	eject_callback_(move(eject_callback))
{}


void configuration_dialog::open()
{
	if (!dialog_handle_)
	{
		dialog_handle_ = CreateDialogParam(
			module_handle_,
			MAKEINTRESOURCE(IDD_SLOT_MANAGER),
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


void configuration_dialog::update_selected_slot()
{
	if (dialog_handle_ == nullptr || !IsWindow(dialog_handle_))
	{
		return;
	}

	// Get radio button IDs
	for (auto ndx(0u); ndx < gSlotUiElementIds.size(); ndx++)
	{
		SendDlgItemMessage(
			dialog_handle_,
			gSlotUiElementIds[ndx].radio_button_id,
			BM_SETCHECK,
			ndx == mpi_->selected_switch_slot(),
			0);
	}

	SetDlgItemText(
		dialog_handle_,
		IDC_SELECTED_SLOT_STATUS,
		std::format("Slot {} is selected as startup slot.", mpi_->selected_switch_slot() + 1).c_str());
}

void configuration_dialog::slot_changed(slot_id_type slot)
{
	if (dialog_handle_ == nullptr || !IsWindow(dialog_handle_))
	{
		return;
	}

	update_slot_details(slot);
}


void configuration_dialog::set_selected_slot(slot_id_type slot)
{
	// FIXME-CHET: Maube move this to the callsite or when the dialog closes or at least make it optional?
	mpi_->switch_to_slot(slot);
	configuration_.selected_slot(slot);

	update_selected_slot();
}

void configuration_dialog::update_slot_details(slot_id_type slot)
{
	SendDlgItemMessage(
		dialog_handle_,
		gSlotUiElementIds[slot].edit_box_id,
		WM_SETTEXT,
		0,
		reinterpret_cast<LPARAM>(mpi_->slot_name(slot).c_str()));

	SendDlgItemMessage(
		dialog_handle_,
		gSlotUiElementIds[slot].insert_button_id,
		WM_SETTEXT,
		0,
		reinterpret_cast<LPARAM>(mpi_->empty(slot) ? ">" : "X"));
}


void configuration_dialog::eject_or_select_new_cartridge(slot_id_type slot)
{
	// Disable Slot changes if parent is disabled.  This prevents user using the
	// config dialog to eject a cartridge while VCC main is using a modal dialog
	// Otherwise user can crash VCC by unloading a disk cart while inserting a disk
	if (!IsWindowEnabled(parent_handle_))
	{
		MessageBox(
			dialog_handle_,
			"Cannot change slot content with dialog open",
			"ERROR",
			MB_ICONERROR);
		return;
	}

	if (!mpi_->empty(slot))
	{
		eject_callback_(slot);
		configuration_.slot_cartridge_path(slot, {});
	}
	else
	{
		insert_callback_(slot);
	}
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
	case WM_CLOSE:
		DestroyWindow(hDlg);
		dialog_handle_ = nullptr;
		parent_handle_ = nullptr;
		return TRUE;

	case WM_INITDIALOG:
		dialog_handle_ = hDlg;
		parent_handle_ = GetParent(hDlg);
		CenterDialog(hDlg);
		for (slot_id_type slot(0); slot < gSlotUiElementIds.size(); slot++)
		{
			update_slot_details(slot);
		}

		update_selected_slot();
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_SELECT1:
			set_selected_slot(0);
			return TRUE;
		case IDC_SELECT2:
			set_selected_slot(1);
			return TRUE;
		case IDC_SELECT3:
			set_selected_slot(2);
			return TRUE;
		case IDC_SELECT4:
			set_selected_slot(3);
			return TRUE;
		case IDC_INSERT1:
			eject_or_select_new_cartridge(0);
			return TRUE;
		case IDC_INSERT2:
			eject_or_select_new_cartridge(1);
			return TRUE;
		case IDC_INSERT3:
			eject_or_select_new_cartridge(2);
			return TRUE;
		case IDC_INSERT4:
			eject_or_select_new_cartridge(3);
			return TRUE;
		} // End switch LOWORD
		break;
	} // End switch message

	return FALSE;
}
