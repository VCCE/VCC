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
#include <algorithm>
#include <cctype>
#include <vcc/util/logger.h>
#include <vcc/util/fileutil.h>
#include <Windows.h>

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
	
	//-------------------------------------------------------------------
	// Get the file path of a loaded module
	// Current exe path is returned if module_handle is null
	//-------------------------------------------------------------------
	std::string ModulePath(HMODULE module_handle)
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
	// If file directory matches application directory strip directory
	//-------------------------------------------------------------------
	std::string StripModPath(const std::string file)
	{
		// Strip path from file if it matches the exe or dll path
		auto app_dir =  GetDirectoryPart(ModulePath(nullptr));
		auto file_dir = GetDirectoryPart(file);
		std::error_code ec;
		if (std::filesystem::equivalent(app_dir, file_dir, ec)) {
			return GetFileNamePart(file);
		}
		return file;
	}

	//---------------------------------------------------------------
	// Fully qualify a file based on current execution directory
	//---------------------------------------------------------------
	std::string QualifyModPath(const std::string& path)
	{
		if (path.empty()) return {};

		std::string mod = path;
		FixDirSlashes(mod);
		if (mod.find('/') != std::string::npos) return mod;

		std::string exe = Util::ModulePath(NULL);
		std::string dir = Util::GetDirectoryPart(exe);
		return dir + '/' + mod;
	}

	//---------------------------------------------------------------
	// Fully qualify a file based on current execution directory
	//---------------------------------------------------------------
//	std::string QualifyPath(const std::string& path)
//	{
//		if (path.empty()) return {};
//
//		std::string mod = path;
//		FixDirSlashes(mod);
//		if (mod.find('/') != std::string::npos) return mod;
//
//		std::string exe = Util::ModulePath(NULL);
//		std::string dir = Util::GetDirectoryPart(exe);
//		return dir + '/' + mod;
//	}
}
