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
#include <vcc/common/std.h>
#include <Windows.h>
#include <string>
#include <codecvt>


namespace vcc { namespace common
{

	LIBCOMMON_EXPORT std::string LoadStdString(HINSTANCE instance, UINT id)
	{
		LPWSTR buffer_ptr = nullptr;
		const auto buffer_length(LoadStringW(instance, id, reinterpret_cast<LPWSTR>(&buffer_ptr), 0));
		if (buffer_length < 1 || buffer_ptr == nullptr)
		{
			return { };
		}

		return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(buffer_ptr, buffer_ptr + buffer_length);
	}

} }
