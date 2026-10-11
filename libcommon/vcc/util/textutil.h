//======================================================================
// General purpose string utilities.  EJ Jaquay 2026
//
// This file is part of VCC (Virtual Color Computer).
// Vcc is Copyright 2015 by Joseph Forgione
//
// VCC (Virtual Color Computer) is free software, you can redistribute
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//
//======================================================================
#pragma once

#include <string>
#include <algorithm>
#include <Windows.h>

//=========================================================================
// Text utilities.  Most of these are general purpose
//=========================================================================

namespace VCC::Util {

	// Return string with case conversion
	inline std::string to_lower(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c) {
				return static_cast<char>(::tolower(c));
			});
		return s;
	}	

	inline std::string to_upper(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c) {
				return static_cast<char>(::toupper(c));
			});
		return s;
	}

	inline void make_lower(std::string& s) {
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c) {
				return static_cast<char>(::tolower(c));
			});
	}

	inline void make_lower(char* s) {
		if (!s) return;
		for (char* p = s; *p; ++p)
			*p = static_cast<char>(::tolower(static_cast<unsigned char>(*p)));
	}

	inline void make_upper(std::string& s) {
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c) {
				return static_cast<char>(::toupper(c));
			});
	}

	inline void make_upper(char* s) {
		if (!s) return;
		for (char* p = s; *p; ++p)
			*p = static_cast<char>(::toupper(static_cast<unsigned char>(*p)));
	}

	inline bool is_null_or_empty(const char* s) {
		return s == nullptr || *s == '\0';
	}

	inline bool is_null_or_empty(const std::string& s) {
		return s.empty();
	}

	inline void copy_to_char(const std::string& src, char* dst, size_t dst_size)
	{
		if (dst_size == 0) return;
		const size_t n = std::min(src.size(), dst_size - 1);
		memcpy(dst, src.data(), n);
		dst[n] = '\0';
	}

	inline std::string WideToUTF8(const wchar_t* w)
	{
		if (!w) return {};

		int len = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
		if (len <= 1) return {};

		std::string out(len - 1, 0);
		WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), len, nullptr, nullptr);
		return out;
	}

	inline std::string WideToUTF8(const std::wstring& ws)
	{
		return WideToUTF8(ws.c_str());
	}

	inline std::wstring UTF8ToWide(const std::string& s)
	{
		if (s.empty()) return std::wstring();

		int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
		if (len <= 1) return std::wstring();

		std::wstring out(len - 1, 0);
		MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
		return out;
	}
}

