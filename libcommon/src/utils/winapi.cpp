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

#include "vcc/utils/winapi.h"
#include <string>
#include <windows.h>


namespace vcc::utils
{

	LIBCOMMON_EXPORT std::string load_string(HINSTANCE instance, UINT id)
	{
		const wchar_t* buffer_ptr;
		// Get len of string to load
		const int length = LoadStringW(instance, id, reinterpret_cast<LPWSTR>(&buffer_ptr), 0);
		if (length == 0)
			return {};

		// Copy load string to wide_str
		const std::wstring wide_str(buffer_ptr, length);

		// Get len of string when converted
		const int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide_str.data(), wide_str.size(), nullptr, 0, nullptr, nullptr);
		if (utf8_len == 0)
		{
			return {};
		}

		// Convert string from wide_str to utf8_str
		std::string utf8_str(utf8_len, '\0');
		WideCharToMultiByte(CP_UTF8, 0, wide_str.data(), wide_str.size(), utf8_str.data(), utf8_len, nullptr, nullptr);

		return utf8_str;
	}

	LIBCOMMON_EXPORT std::string get_dialog_item_text(HWND window, UINT id)
	{
		const auto length(SendDlgItemMessage(window, id, WM_GETTEXTLENGTH, 0, 0));

		std::string text(length, 0);
		SendDlgItemMessage(window, id, WM_GETTEXT, MAX_PATH, reinterpret_cast<LPARAM>(text.data()));

		return text;
	}

}

