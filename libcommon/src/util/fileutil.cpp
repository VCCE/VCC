//#define USE_LOGGING
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

#include <string>
#include <filesystem>
#include <Shlwapi.h>
#include <vcc/util/logger.h>
#include <vcc/util/fileutil.h>

namespace VCC::Util
{
	//----------------------------------------------------------------------
	// Get most recent windows error text
	//----------------------------------------------------------------------
	std::string LastErrorString()
	{
		DWORD error_code = GetLastError();
		char msg[256];
		DWORD len = FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, error_code,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			msg, sizeof(msg), nullptr
		);
		if (len == 0) return "Unknown error";
		return std::string(msg, len);
	}

	// Only use this overload as argument to debug logger (DLOG)
	const char* LastErrorTxt()
	{
		thread_local static std::string buffer;
		buffer = LastErrorString();
		return buffer.c_str();
	}
	
	//TODO: This is generic, move to something for generic utils
	//----------------------------------------------------------------------
	// Return copy of string with spaces trimmed from end of a string
	//----------------------------------------------------------------------
	std::string trim_right_spaces(const std::string& s)
	{
		size_t end = s.find_last_not_of(' ');
		if (end == std::string::npos)
			return {};
		return s.substr(0, end + 1);
	}
	
	//----------------------------------------------------------------------
	// Return slash normalized directory part of a path
	//----------------------------------------------------------------------
	std::string GetDirectoryPart(const std::string& input)
	{
		std::filesystem::path p(input);
		std::string out = p.parent_path().string();
		FixDirSlashes(out);
		return out;
	}
	
	//----------------------------------------------------------------------
	// Return filename part of a path
	//----------------------------------------------------------------------
	std::string GetFileNamePart(const std::string& input)
	{
		std::filesystem::path p(input);
		return p.filename().string();
	}
	
	//----------------------------------------------------------------------
	// Determine if path is a direcory
	//----------------------------------------------------------------------
	bool IsDirectory(const std::string& path)
	{
		std::error_code ec;
		return std::filesystem::is_directory(path, ec) && !ec;
	}
	
	//-------------------------------------------------------------------
	// Convert path directory backslashes to forward slashes
	//-------------------------------------------------------------------
	void FixDirSlashes(std::string &dir)
	{
		if (dir.empty()) return;
		std::replace(dir.begin(), dir.end(), '\\', '/');
		if (dir.back() == '/') dir.pop_back();
	}
	
	//-------------------------------------------------------------------
	// Get the file path of a loaded module
	// Current exe path is returned if module_handle is null
	//-------------------------------------------------------------------
	std::string get_module_path(HMODULE module_handle)
	{
    	std::string text(MAX_PATH, '\0');
    	for (;;) {
			DWORD ret = GetModuleFileName(module_handle, &text[0], text.size());
        	if (ret == 0)
				return {}; // error

        	if (ret < text.size() - 1) {
            	text.resize(ret);
            	return text;
        	}
        	// truncated grow and retry
        	text.resize(text.size() * 2);
    	}
	}

	//-------------------------------------------------------------------
	// If path directory matches application directory strip directory
	//-------------------------------------------------------------------
	std::string strip_application_path(std::string path)
	{
		auto app = get_module_path(nullptr);
		auto app_dir =  GetDirectoryPart(app);
		auto path_dir = GetDirectoryPart(path);
		if (path_dir == app_dir) {
			path = GetFileNamePart(path);
		}
		return path;
	}
}
