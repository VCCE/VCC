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
#include "vcc/utils/filesystem.h"
#include <array>
#include <format>


namespace vcc::cartridges::multipak
{

	namespace
	{

		using cartridge_loader_status = vcc::utils::cartridge_loader_status;

		struct cartridge_ui_element_identifiers
		{
			UINT edit_box_id;
			UINT radio_button_id;
			UINT eject_button_id;
			UINT insert_rompak_button_id;
			UINT insert_device_button_id;
		};

		const std::array<cartridge_ui_element_identifiers, 4> gSlotUiElementIds = { {
			{ IDC_EDIT1, IDC_SELECT_SLOT1, IDC_EJECT_SLOT1, IDC_INSERT_ROMPAK_INTO_SLOT1, IDC_INSERT_DEVICE_INTO_SLOT1 },
			{ IDC_EDIT2, IDC_SELECT_SLOT2, IDC_EJECT_SLOT2, IDC_INSERT_ROMPAK_INTO_SLOT2, IDC_INSERT_DEVICE_INTO_SLOT2 },
			{ IDC_EDIT3, IDC_SELECT_SLOT3, IDC_EJECT_SLOT3, IDC_INSERT_ROMPAK_INTO_SLOT3, IDC_INSERT_DEVICE_INTO_SLOT3 },
			{ IDC_EDIT4, IDC_SELECT_SLOT4, IDC_EJECT_SLOT4, IDC_INSERT_ROMPAK_INTO_SLOT4, IDC_INSERT_DEVICE_INTO_SLOT4 }
		} };

	}

	configuration_dialog::configuration_dialog(
		// TODO-CHET: globally rename module_handle to library_handle
		HINSTANCE module_handle,
		std::shared_ptr<multipak_configuration> configuration,
		std::shared_ptr<expansion_port_bus_type> bus,
		std::shared_ptr<expansion_port_ui_type> ui,
		cartridge_controller_type& controller)
		:
		module_handle_(module_handle),
		configuration_(move(configuration)),
		bus_(move(bus)),
		ui_(move(ui)),
		controller_(controller)
	{
		if (module_handle_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Multi-Pak Cartridge. Module handle is null.");
		}

		if (configuration_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Multi-Pak Cartridge. Configuration is null.");
		}

		if (bus_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Multi-Pak Cartridge. Bus is null.");
		}

		if (ui_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Multi-Pak Cartridge. UI is null.");
		}
	}


	void configuration_dialog::open()
	{
		if (!dialog_handle_)
		{
			dialog_handle_ = CreateDialogParam(
				module_handle_,
				MAKEINTRESOURCE(IDD_SLOT_MANAGER),
				ui_->app_window(),
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
				ndx == controller_.selected_switch_slot(),
				0);
		}

		SetDlgItemText(
			dialog_handle_,
			IDC_SELECTED_SLOT_STATUS,
			std::format("Slot {} is selected as startup slot.", controller_.selected_switch_slot() + 1).c_str());
	}


	void configuration_dialog::set_selected_slot(slot_id_type slot_id)
	{
		controller_.switch_to_slot(slot_id, false);
		configuration_->selected_slot(slot_id);

		update_selected_slot();
	}

	void configuration_dialog::update_slot_details(slot_id_type slot)
	{
		if (dialog_handle_ == nullptr || !IsWindow(dialog_handle_))
		{
			return;
		}

		SendDlgItemMessage(
			dialog_handle_,
			gSlotUiElementIds[slot].edit_box_id,
			WM_SETTEXT,
			0,
			reinterpret_cast<LPARAM>(controller_.get_cartridge_slot_name(slot).c_str()));

		EnableWindow(
			GetDlgItem(dialog_handle_, gSlotUiElementIds[slot].eject_button_id),
			!controller_.is_cartridge_slot_empty(slot));
	}

	void configuration_dialog::eject_cartridge(slot_id_type slot_id)
	{
		controller_.eject_cartridge(slot_id, true, true);
		configuration_->slot_path(slot_id, {});
	}

	void configuration_dialog::insert_rompak_cartridge(slot_id_type slot_id)
	{
		controller_.select_and_insert_rompak_cartridge(slot_id);
	}

	void configuration_dialog::insert_device_cartridge(slot_id_type slot_id)
	{
		controller_.select_and_insert_device_cartridge(slot_id);
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

		return dialog->process_message(hDlg, message, wParam, lParam);
	}


	INT_PTR configuration_dialog::process_message(
		HWND hDlg,
		UINT message,
		WPARAM wParam,
		LPARAM lParam)
	{
		switch (message)
		{
		case WM_CLOSE:
			DestroyWindow(hDlg);
			dialog_handle_ = nullptr;
			return TRUE;

		case WM_INITDIALOG:
			dialog_handle_ = hDlg;
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
			case IDOK:
				SendMessage(hDlg,WM_CLOSE,0,0);
				return TRUE;

			case IDC_RESET_COMPUTER:
				bus_->reset();
				return TRUE;

			case IDC_SELECT_SLOT1:
				set_selected_slot(0);
				return TRUE;
			case IDC_SELECT_SLOT2:
				set_selected_slot(1);
				return TRUE;
			case IDC_SELECT_SLOT3:
				set_selected_slot(2);
				return TRUE;
			case IDC_SELECT_SLOT4:
				set_selected_slot(3);
				return TRUE;

			case IDC_EJECT_SLOT1:
				eject_cartridge(0);
				return TRUE;
			case IDC_EJECT_SLOT2:
				eject_cartridge(1);
				return TRUE;
			case IDC_EJECT_SLOT3:
				eject_cartridge(2);
				return TRUE;
			case IDC_EJECT_SLOT4:
				eject_cartridge(3);
				return TRUE;

			case IDC_INSERT_ROMPAK_INTO_SLOT1:
				insert_rompak_cartridge(0);
				return TRUE;
			case IDC_INSERT_ROMPAK_INTO_SLOT2:
				insert_rompak_cartridge(1);
				return TRUE;
			case IDC_INSERT_ROMPAK_INTO_SLOT3:
				insert_rompak_cartridge(2);
				return TRUE;
			case IDC_INSERT_ROMPAK_INTO_SLOT4:
				insert_rompak_cartridge(3);
				return TRUE;

			case IDC_INSERT_DEVICE_INTO_SLOT1:
				insert_device_cartridge(0);
				return TRUE;
			case IDC_INSERT_DEVICE_INTO_SLOT2:
				insert_device_cartridge(1);
				return TRUE;
			case IDC_INSERT_DEVICE_INTO_SLOT3:
				insert_device_cartridge(2);
				return TRUE;
			case IDC_INSERT_DEVICE_INTO_SLOT4:
				insert_device_cartridge(3);
				return TRUE;
			} // End switch LOWORD
			break;
		} // End switch message

		return FALSE;
	}

}
