#include "vcc/ui/dialog_window.h"
#include <vcc/utils/winapi.h>
#include <vcc/common/DialogOps.h>


namespace vcc::ui
{

	dialog_window::dialog_window(HINSTANCE module_handle, UINT dialog_resource_id)
		:
		module_handle_(module_handle),
		dialog_resource_id_(dialog_resource_id)
	{
		if (module_handle_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Dialog Window. Module handle is null.");
		}
	}


	HWND dialog_window::handle() const
	{
		return dialog_handle_;
	}

	bool dialog_window::is_open() const
	{
		return handle() != nullptr;
	}


	INT_PTR dialog_window::do_modal(HWND owner)
	{
		if (is_open())
		{
			throw std::runtime_error("Cannot open a modal instance of the dialog window. It is already open.");
		}

		is_modal_ = true;

		return DialogBoxParam(
			module_handle_,
			MAKEINTRESOURCE(dialog_resource_id_),
			owner,
			callback_procedure,
			reinterpret_cast<LPARAM>(this));
	}

	void dialog_window::create(HWND owner)
	{
		if (is_open())
		{
			throw std::runtime_error("Cannot open dialog window. Dialog is already open.");
		}

		is_modal_ = false;
		CreateDialogParam(
			module_handle_,
			MAKEINTRESOURCE(dialog_resource_id_),
			owner,
			callback_procedure,
			reinterpret_cast<LPARAM>(this));
	}

	void dialog_window::close()
	{
		if (is_open())
		{
			destroy_window();
		}
	}


	INT_PTR CALLBACK dialog_window::callback_procedure(
		HWND hDlg,
		UINT message,
		WPARAM wParam,
		LPARAM lParam)
	{
		if (message == WM_INITDIALOG)
		{
			SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		}

		auto dialog(reinterpret_cast<dialog_window*>(GetWindowLongPtr(hDlg, GWLP_USERDATA)));

		// FIXME-CHET: There are messages coming before WM_INITDIALOG which shouldn't happen.
		// one message is WM_SETFONT which might be getting dispatched while initializing
		// the controls.
		if (dialog == nullptr)
		{
			return 0;
		}

		return dialog->process_message(hDlg, message, wParam, lParam);
	}

	INT_PTR dialog_window::process_message(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CLOSE:
			on_close();
			return TRUE;

		case WM_INITDIALOG:
			dialog_handle_ = hDlg;
			return on_init_dialog();

		case WM_DESTROY:
			on_destroy();
			dialog_handle_ = nullptr;
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDOK:
				on_ok();
				return TRUE;

			case IDCANCEL:
				on_cancel();
				return TRUE;
			}
			return on_command(wParam, lParam);
		}

		return on_message(message, wParam, lParam);
	}


	bool dialog_window::show_window(int show_command)
	{
		if (is_open())
		{
			return false;
		}

		return ShowWindow(handle(), show_command) != FALSE;
	}

	void dialog_window::destroy_window()
	{
		DestroyWindow(handle());
	}

	void dialog_window::enable_control(UINT control_id, bool is_enabled) const
	{
		const auto control(GetDlgItem(handle(), control_id));
		if (control == nullptr)
		{
			return;
		}

		EnableWindow(control, is_enabled);
	}

	dialog_window::string_type dialog_window::get_control_text(UINT control_id) const
	{
		return ::vcc::utils::get_dialog_item_text(handle(), control_id);
	}

	void dialog_window::set_control_text(UINT control_id, const string_type& text) const
	{
		SendDlgItemMessageA(
			handle(),
			control_id,
			WM_SETTEXT,
			0,
			reinterpret_cast<LPARAM>(static_cast<LPCSTR>(text.c_str())));
	}

	void dialog_window::set_control_text(UINT control_id, const path_type& path) const
	{
		SendDlgItemMessageW(
			handle(),
			control_id,
			WM_SETTEXT,
			0,
			reinterpret_cast<const LPARAM>(static_cast<LPCWSTR>(path.c_str())));
	}

	bool dialog_window::is_button_checked(UINT control_id) const
	{
		return IsDlgButtonChecked(handle(), control_id) != BST_UNCHECKED;
	}

	void dialog_window::set_button_check(UINT control_id, bool is_checked) const
	{
		SendDlgItemMessageA(
			handle(),
			control_id,
			BM_SETCHECK,
			is_checked ? BST_CHECKED : BST_UNCHECKED,
			0);
	}

	void dialog_window::do_destroy_window(UINT return_code)
	{
		if (is_modal_)
		{
			EndDialog(handle(), return_code);
		}
		else
		{
			destroy_window();
		}
	}


	INT_PTR dialog_window::on_message(
		[[maybe_unused]] UINT message,
		[[maybe_unused]] WPARAM wParam,
		[[maybe_unused]] LPARAM lParam)
	{
		return FALSE;
	}

	INT_PTR dialog_window::on_command(
		[[maybe_unused]] WPARAM wParam,
		[[maybe_unused]] LPARAM lParam)
	{
		// Return non-zero to indicate message has not been processed
		return 1;
	}


	bool dialog_window::on_init_dialog()
	{
		return true;
	}

	void dialog_window::on_close()
	{
		do_destroy_window(IDCANCEL);
	}

	void dialog_window::on_destroy()
	{
	}


	void dialog_window::on_ok()
	{
		do_destroy_window(IDOK);
	}

	void dialog_window::on_cancel()
	{
		do_destroy_window(IDCANCEL);
	}

}
