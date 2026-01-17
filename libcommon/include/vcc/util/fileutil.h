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
	const char * LastErrorTxt();

	// Convert backslashes to slashes in directory string
	LIBCOMMON_EXPORT void FixDirSlashes(std::string &dir);

	// Return copy of string with spaces trimmed from end of a string
	LIBCOMMON_EXPORT std::string trim_right_spaces(const std::string &s);

	// Return slash normalized directory part of a path
	LIBCOMMON_EXPORT std::string GetDirectoryPart(const std::string& input);

	// Return filename part of a path
	LIBCOMMON_EXPORT std::string GetFileNamePart(const std::string& input);

	// Determine if path is a direcory
	LIBCOMMON_EXPORT bool IsDirectory(const std::string& path);

	// Get path of loaded module or current application
	LIBCOMMON_EXPORT std::string get_module_path(HMODULE module_handle);

	// If path is in the application directory strip directory
	LIBCOMMON_EXPORT std::string strip_application_path(std::string path);
}
