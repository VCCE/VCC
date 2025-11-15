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
#include <string>
#include <Windows.h>


class cartridge_controller
{
public:

	using string_type = std::string;


public:

	virtual ~cartridge_controller() = default;

	virtual string_type server_address() const = 0;
	virtual string_type server_port() const = 0;
	virtual void set_server_address(
		const string_type& server_address,
		const string_type& server_port) = 0;
};


class configuration_dialog
{
public:

	using controller_type = cartridge_controller;


public:

	configuration_dialog(HINSTANCE module_handle, controller_type& controller);

	configuration_dialog(const configuration_dialog&) = delete;
	configuration_dialog(configuration_dialog&&) = delete;

	configuration_dialog& operator=(const configuration_dialog&) = delete;
	configuration_dialog& operator=(configuration_dialog&&) = delete;

	void open();
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
	controller_type& controller_;
	HWND dialog_handle_ = nullptr;
	HWND parent_handle_ = nullptr;
};
