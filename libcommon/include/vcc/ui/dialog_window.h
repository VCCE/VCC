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
#include "vcc/detail/exports.h"
#include <string>
#include <filesystem>
#include <Windows.h>


namespace vcc::ui
{

	/// @brief Simple dialog window
	///
	/// @todo mode detailed description
	class LIBCOMMON_EXPORT dialog_window
	{
	public:

		/// @brief Type alias to lengths, 1 dimension sizes, and indexes.
		using size_type = std::size_t;
		/// @brief Type alias for variable length strings.
		using string_type = std::string;
		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;


	public:

		/// @brief Construct the Dialog
		/// 
		/// @param module_handle A handle to the instance of the module containing the
		/// resources for the configuration dialog.
		/// @param dialog_resource_id The identifier of the template resource used to
		/// create the dialog.
		/// 
		/// @throws std::invalid_argument if `module_instance` is null.
		dialog_window(HINSTANCE module_handle, UINT dialog_resource_id);

		virtual ~dialog_window() = default;

		/// @brief Returns the open status of the dialog.
		/// 
		/// @return `true` if the dialog is open; `false` otherwise.
		[[nodiscard]] bool is_open() const;


		/// @brief 
		/// 
		/// Creates a modal dialog box from a dialog box template resource. Before
		/// displaying the dialog box.
		/// 
		/// @param owner A handle to the window that owns the dialog box.
		/// 
		/// @return 
		[[nodiscard]] virtual INT_PTR do_modal(HWND owner);

		/// @brief Opens the dialog.
		///
		/// Opens the configuration dialog. If the dialog is already open, it is brought to the
		/// top of the Z-order and presented to the user.
		virtual void create(HWND owner);

		/// @brief Closes the dialog.
		///
		/// Closes the dialog if it is open.
		virtual void close();

		/// @brief Shows or hides the window.
		/// 
		/// | Show Command			| Description |
		/// |-----------------------|-------------------------------------------------------------|
		///	| SW_HIDE				| Hides the window and activates another window. |
		///	| SW_SHOWNORMAL			| Hides the window and activates another window. |
		///	| SW_NORMAL				| Activates and displays a window. |
		///	| SW_SHOWMINIMIZED		| Activates the window and displays it as a minimized window. |
		///	| SW_SHOWMAXIMIZED		| Activates the window and displays it as a minimized window. |
		///	| SW_MAXIMIZE			| Activates the window and displays it as a maximized window. |
		///	| SW_SHOWNOACTIVATE		| Displays a window in its most recent size and position. |
		///	| SW_SHOW				| Activates the window and displays it in its current size and position. |
		///	| SW_MINIMIZE			| Minimizes the specified window and activates the next top-level window in the Z order. |
		///	| SW_SHOWMINNOACTIVE	| Displays the window as a minimized window. |
		///	| SW_SHOWNA				| Displays the window in its current size and position. |
		///	| SW_RESTORE			| Activates and displays the window. |
		///	| SW_SHOWDEFAULT		| Sets the show state based on the SW_ value specified to CreateProcess. |
		///	| SW_FORCEMINIMIZE		| Minimizes a window. |
		/// 
		/// For more information on the behavior of show_window and the above commands
		/// see https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
		/// 
		/// @param show_command The command to apply when showing or hiding the window.
		/// @return `true` if the operation was successful; `false` otherwise.
		bool show_window(int show_command);

		/// @brief Destroys the dialog window.
		///
		/// Destroys the specified window. The function sends WM_DESTROY and WM_NCDESTROY
		/// messages to the window to deactivate it and remove the keyboard focus from
		/// it. The function also destroys the window's menu, destroys timers, removes
		/// clipboard ownership, and breaks the clipboard viewer chain (if the window is
		/// at the top of the viewer chain).
		/// 
		/// If the specified window is a parent or owner window, DestroyWindow
		/// automatically destroys the associated child or owned windows when it destroys
		/// the parent or owner window. The function first destroys child or owned
		/// windows, and then it destroys the parent or owner window.
		void destroy_window();

		/// @brief Enables or disables mouse and keyboard input to the specified control.
		/// 
		/// Enables or disables mouse and keyboard input to the specified control. When
		/// input is disabled, the window does not receive input such as mouse clicks and
		/// key presses. When input is enabled, the window receives all input.
		/// 
		/// @param control_id The identifier of the control to enable or disable.
		/// @param is_enabled Flag specifying if the control should be disabled or
		/// enabled. If this argument is `true` the control is enabled; otherwise it is
		/// disabled.
		void enable_control(UINT control_id, bool is_enabled) const;

		/// @brief retrieves the controls text.
		/// 
		/// Returns the text that corresponds to a control.
		/// 
		/// @param control_id The identifier of the control to retrieve the text from.
		/// 
		/// @return A string containing the controls text.
		[[nodiscard]] string_type get_control_text(UINT control_id) const;

		/// @brief Sets the text of a window.
		/// 
		/// @param control_id The identifier of the control to set the text for.
		/// @param text The text to set.
		void set_control_text(UINT control_id, const string_type& text) const;

		/// @brief Sets the text of a window.
		/// 
		/// @param control_id The identifier of the control to set the text for.
		/// @param path text The path to use as the text to set.
		void set_control_text(UINT control_id, const path_type& path) const;

		/// @brief Determines if a button control is checked.
		/// 
		/// @param control_id The identifier of the control to get the check
		/// status of.
		/// 
		/// @return `true` if the control is checked; `false` otherwise.
		[[nodiscard]] bool is_button_checked(UINT control_id) const;

		/// @brief Sets the check state of a control.
		/// 
		/// Sets the check state of a radio button or check box control.
		/// 
		/// @param control_id The identifier of the control that will have its check
		/// state set.
		/// @param is_checked Specifies if the control should be appear in a checked
		/// state. If the argument is `true` the control will appear checked.
		void set_button_check(UINT control_id, bool is_checked) const;


	protected:

		/// @brief Retrieve the dialogs window handle.
		/// 
		/// @return The dialogs window handle.
		[[nodiscard]] HWND handle() const;

		/// @brief Process a dialog message.
		/// 
		/// @param message The message.
		/// @param wParam The word parameter for the message. The contents of this parameter
		/// depend on the message received.
		/// @param lParam The long parameter for the message. The contents of this parameter
		/// depend on the message received.
		/// 
		/// @return The return value depends on the message being processed.
		[[nodiscard]] virtual INT_PTR on_message(
			UINT message,
			WPARAM wParam,
			LPARAM lParam);

		/// @brief 
		/// 
		/// Called when the user invokes a command item from a menu, when a control sends
		/// a notification message to its parent window, or when an accelerator keystroke
		/// is translated.
		/// 
		/// @param wParam The word parameter for the message. The contents of this parameter
		/// depend on the message received.
		/// @param lParam The long parameter for the message. The contents of this parameter
		/// depend on the message received.
		/// 
		/// @return '0' (zero) if the function processes the command; non-zero otherwise.
		[[nodiscard]] virtual INT_PTR on_command(WPARAM wParam, LPARAM lParam);


		/// @brief Initialize the dialog window.
		/// 
		/// Called after the dialog window has been created and immediately before it is
		/// displayed. This function is typically overridden to initialize controls and
		/// carry out any other initialization tasks that affect the appearance of the
		/// dialog window.
		/// 
		/// @return `true` if the windowing system should set the keyboard focus to the
		/// default control.
		[[nodiscard]] virtual bool on_init_dialog();

		/// @brief Close the window.
		///
		/// This function is called in response to a message requesting that the dialog
		/// window close.
		virtual void on_close();

		/// @brief Called when the window is being destroyed.
		/// 
		/// Called when the window is being destroyed after the window is removed from the
		/// screen.
		/// 
		/// This is first called on the window being destroyed and then forwarded to the
		/// child windows (if any) as they are destroyed. During the processing of this
		/// call, it can be assumed that all child windows still exist.
		virtual void on_destroy();

		/// @brief Called when an OK button is pressed.
		///
		/// Called when button assigned the identifier `IDOK` is pressed.
		virtual void on_ok();

		/// @brief Called when an OK button is pressed.
		///
		/// Called when button assigned the identifier `IDCANCEL` is pressed.
		virtual void on_cancel();

		/// @brief Destroys the window.
		/// 
		/// This is usually invoked by functions like on_destroy, ok_ok, and on_cancel
		/// to close the dialog with a status code that will be returned if the dialog
		/// is modal.
		/// 
		/// @param return_code The value to return when ending a modal dialog.
		virtual void do_destroy_window(UINT return_code);


	private:

		/// @brief Process a dialog message.
		/// 
		/// @param hDlg A handle to the dialog window the message is for.
		/// @param message The message.
		/// @param wParam The word parameter for the message. The contents of this parameter
		/// depend on the message received.
		/// @param lParam The long parameter for the message. The contents of this parameter
		/// depend on the message received.
		/// 
		/// @return The return value depends on the message being processed.
		[[nodiscard]] INT_PTR process_message(
			HWND hDlg,
			UINT message,
			WPARAM wParam,
			LPARAM lParam);

		/// @brief Callback procedure that processes dialog messages.
		/// 
		/// @param hDlg A handle to the dialog window the message is for.
		/// @param message The message.
		/// @param wParam The word parameter for the message. The contents of this parameter
		/// depend on the message received.
		/// @param lParam The long parameter for the message. The contents of this parameter
		/// depend on the message received.
		/// 
		/// @return The return value depends on the message being processed.
		[[nodiscard]] static INT_PTR CALLBACK callback_procedure(
			HWND hDlg,
			UINT message,
			WPARAM wParam,
			LPARAM lParam);


	private:

		/// @brief The handle to the module instance containing the dialog resources.
		const HINSTANCE module_handle_;
		/// @brief The identifier of the template resource used to create the dialog.
		const UINT dialog_resource_id_;
		/// @brief The handle to the currently opened dialog, or null if no dialog is open.
		HWND dialog_handle_ = nullptr;
		/// @brief Flag indicating if the open dialog is modal or modaless.
		bool is_modal_ = false;
	};

}
