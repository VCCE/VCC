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
#pragma once
#include "vcc/bus/expansion_port_ui.h"
#include <Windows.h>
#include <memory>
#include <string>
#include <functional>


namespace vcc::cartridges::becker_port
{

	/// @brief Configuration/settings dialog
	class configuration_dialog
	{
	public:

		/// @brief Defines the type used to hold a variable length string.
		using string_type = std::string;
		/// @brief Type alias for the component providing user interface services to the
		/// cartridge plugin.
		using expansion_port_ui_type = ::vcc::bus::expansion_port_ui;

		/// @brief Defines the type and signature of the function called when
		/// the connection settings change.
		using update_connection_settings_type = std::function<void(
			const string_type& server_address,
			const string_type& server_port)>;


	public:

		/// @brief Construct a Becker Port settings dialog.
		/// 
		/// @param module_handle The module instance of the DLL containing the dialog
		/// resources.
		/// @param ui A pointer to the UI services interface.
		/// @param update_connection_settings A callback function that will be invoked
		/// when the settings change.
		/// 
		/// @throws std::invalid_argument if `module_handle` is null.
		/// @throws std::invalid_argument if `ui` is null.
		/// @throws std::invalid_argument if `update_connection_settings` is null.
		configuration_dialog(
			HINSTANCE module_handle,
			std::shared_ptr<expansion_port_ui_type> ui,
			update_connection_settings_type update_connection_settings);

		/// @brief Open the settings dialog
		/// 
		/// Opens the settings dialog and populates the property values with those passed in the
		/// parameters. If the settings dialog is already open it will be moved to the highest
		/// Z-order possible and displayed to the user.
		/// 
		/// @param server_address The address of the DriveWire server to connect to.
		/// @param server_port The port of the DriveWire server to connect to.
		void open(string_type server_address, string_type server_port);

		/// @brief Close the settings dialog
		void close();


	private:

		/// @brief Process window messages for the settings dialog.
		/// 
		/// @param hDlg The handle to the dialog the message is for.
		/// @param message The identifier of the message.
		/// @param wParam The word parameter of the message. The contents of this parameter
		/// depend on the message received.
		/// 
		/// @return The return value depends on the message being processed.
		INT_PTR process_message(
			HWND hDlg,
			UINT message,
			WPARAM wParam);

		/// @brief The procedure that receives window messages for the dialog
		/// 
		/// This function receives window messages for the settings dialog and passes them
		/// to an instance of the configuration dialog. When the dialog is created a
		/// pointer to the instance of the configuration dialog is passed to the create
		/// function. When the `WM_INITDIALOG` message is received by this function that
		/// pointer is retrieved from the `lParam` parameter and stored in the dialog
		/// windows user data. On all subsequent calls, the pointer will be retrieved from
		/// the user data and the message forwarded to the `process_message` function.
		/// 
		/// @param hDlg The handle to the dialog the message is for.
		/// @param message The identifier of the message.
		/// @param wParam The word parameter of the message. The contents of this parameter
		/// depend on the message received.
		/// @param lParam The long parameter of the message. The contents of this parameter
		/// depend on the message received.
		/// 
		/// @return The return value depends on the message being processed.
		static INT_PTR CALLBACK callback_procedure(
			HWND hDlg,
			UINT message,
			WPARAM wParam,
			LPARAM lParam);


	private:

		/// @brief The handle to the module instance containing the dialog's resources.
		const HINSTANCE module_handle_;
		/// @brief The expansion port UI service provider.
		const std::shared_ptr<expansion_port_ui_type> ui_;
		/// @brief A function that will be invoked if/when the connection settings are
		/// changed by the user.
		const update_connection_settings_type update_connection_settings_;
		/// @brief The handle to the current dialog or `null` if no dialog is open.
		HWND dialog_handle_ = nullptr;
		/// @brief The server address the dialog will show when initially created.
		string_type server_address_;
		/// @brief The server port the dialog will show when initially created.
		string_type server_port_;
	};

}
