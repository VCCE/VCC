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
#include <Windows.h>
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
		/// @param update_connection_settings A callback function that will be invoked
		/// when the settings change.
		configuration_dialog(
			HINSTANCE module_handle,
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

		INT_PTR process_message(
			HWND hDlg,
			UINT message,
			WPARAM wParam);

		static INT_PTR CALLBACK callback_procedure(
			HWND hDlg,
			UINT message,
			WPARAM wParam,
			LPARAM lParam);


	private:

		const HINSTANCE module_handle_;
		const update_connection_settings_type update_connection_settings;
		HWND dialog_handle_ = nullptr;

		string_type server_address_;
		string_type server_port_;
	};

}
