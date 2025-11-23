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
#include <Windows.h>
#include <string>
#include <functional>


class configuration_dialog
{
public:

	using string_type = std::string;
	using set_server_function_type = std::function<void(
		const string_type& server_address,
		const string_type& server_port)>;


public:

	configuration_dialog(
		HINSTANCE module_handle,
		set_server_function_type set_server_callback);

	configuration_dialog(const configuration_dialog&) = delete;
	configuration_dialog(configuration_dialog&&) = delete;

	configuration_dialog& operator=(const configuration_dialog&) = delete;
	configuration_dialog& operator=(configuration_dialog&&) = delete;

	void open(string_type server_address, string_type server_port);
	void close();


private:

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

	const HINSTANCE module_handle_;
	const set_server_function_type set_server_callback_;
	HWND dialog_handle_ = nullptr;
	HWND parent_handle_ = nullptr;

	string_type server_address_;
	string_type server_port_;
};
