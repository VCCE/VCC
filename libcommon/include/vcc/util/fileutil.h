//======================================================================
// General purpose Host file utilities.  EJ Jaquay 2026
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
#include <filesystem>
#include <Windows.h>

#include <vcc/util/exports.h>

//=========================================================================
// Host file utilities.  Most of these are general purpose
//=========================================================================

namespace VCC::Util {

	// Get most recent windows error text
	LIBCOMMON_EXPORT std::string LastErrorString();
	LIBCOMMON_EXPORT const char* LastErrorTxt();

	// Get path of loaded module or current application
	LIBCOMMON_EXPORT std::string get_module_path(HMODULE module_handle);

	// If path is in the application directory strip directory
	LIBCOMMON_EXPORT std::string strip_application_path(std::string path);

	// Fully qualify a file based on execution directory
	LIBCOMMON_EXPORT std::string QualifyPath(const std::string& path);

	//------------------------------------------------------------------------
	// In line functions
	//------------------------------------------------------------------------

	// Return copy of string with spaces trimmed from end of a string
	inline std::string trim_right_spaces(const std::string& s)
	{
		size_t end = s.find_last_not_of(' ');
		if (end == std::string::npos) return {};
		return s.substr(0, end + 1);
	}
	
	// Return filename part of a path
	inline std::string GetFileNamePart(const std::string& input)
	{
		std::filesystem::path p(input);
		return p.filename().string();
	}
	
	// Determine if path is a direcory
	inline bool IsDirectory(const std::string& path)
	{
		std::error_code ec;
		return std::filesystem::is_directory(path, ec) && !ec;
	}

	// Verify that a file can be opened for read/write
	inline bool ValidateRWFile(const std::string& path)
	{
		HANDLE h = CreateFile
			(path.c_str(), GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ, nullptr, OPEN_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL, nullptr);
		if (h==INVALID_HANDLE_VALUE) return false;
		CloseHandle(h);
		return true;
	}

	// Verify that a file can be opened for read
	inline bool ValidateRDFile(const std::string& path)
	{
		HANDLE h = CreateFile
			(path.c_str(), GENERIC_READ,
			FILE_SHARE_READ, nullptr, OPEN_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL, nullptr);
		if (h==INVALID_HANDLE_VALUE) return false;
		CloseHandle(h);
		return true;
	}

	// Convert backslashes to slashes within string
	inline void FixDirSlashes(std::string& dir)
	{
		if (dir.empty()) return;
		std::replace(dir.begin(), dir.end(), '\\', '/');
		if (dir.back() == '/') dir.pop_back();
	}

	// Convert backslashes to slashes within char *
	inline void FixDirSlashes(char* dir)
	{
		if (!dir || !*dir) return;
			for (char* p = dir; *p; ++p)
				if (*p == '\\') *p = '/';
	}

	// Convert slashes to backslashes within char * (for legacy win calls)
	inline void RevDirSlashes(char* dir)
	{
		if (!dir || !*dir) return;
			for (char* p = dir; *p; ++p)
				if (*p == '/') *p = '\\';
	}

	// Return copy of path with backslashes converterd
	inline std::string FixDirSlashes(const std::string& dir)
	{
		std::string s = dir;
		FixDirSlashes(s);
		return s;
	}

	// Strip trailing backslash from directory or path 
	inline void StripTrailingSlash(std::string& dir)
	{
		if (dir.back() == '/') dir.pop_back();
	}

	// Return copy with trailing backslash stripped
	inline std::string StripTrailingSlash(const std::string& dir)
	{
		std::string s = dir;
		StripTrailingSlash(s);
		return s;
	}

	// Conditionally append trailing backslash to directory
	inline void AppendTrailingSlash(std::string dir)
	{
		if (dir.back() != '/') dir += '/';
	}
	
	// Return slash normalized directory part of a path
	inline std::string GetDirectoryPart(const std::string& input)
	{
		std::filesystem::path p(input);
		std::string out = p.parent_path().string();
		FixDirSlashes(out);
		return out;
	}

	//------------------------------------------------------------------------
	// TODO: In line functions that should go elsewhere
	//------------------------------------------------------------------------

	// Return string with case conversion	
	inline std::string to_lower(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c){ return std::tolower(c); });
		return s;
	}

	inline std::string to_upper(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c){ return std::toupper(c); });
		return s;
	}

	// Convert case of string inplace
	inline void make_lower(std::string& s) {
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c){ return std::tolower(c); });
	}

	inline void make_lower(char* s) {
		if (!s) return;
		for (char* p = s; *p; ++p)
			*p = static_cast<char>(std::tolower(static_cast<unsigned char>(*p)));
	}

	inline void make_upper(std::string& s) {
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c){ return std::toupper(c); });
	}

	inline void make_upper(char* s) {
		if (!s) return;
		for (char* p = s; *p; ++p)
			*p = static_cast<char>(std::toupper(static_cast<unsigned char>(*p)));
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
}
