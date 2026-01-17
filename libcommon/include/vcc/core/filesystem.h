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
#include <vcc/core/exports.h>  // defines LIBCOMMON_EXPORT if libcommon is a DLL
#include <string>
#include <Windows.h>

//TODO replace get_directory_from_path and get_filename with fileutil functions
//TODO move find_pak_module_path to point of use
namespace vcc::core::utils
{
	LIBCOMMON_EXPORT std::string find_pak_module_path(std::string path);
	LIBCOMMON_EXPORT std::string get_directory_from_path(std::string path);
	LIBCOMMON_EXPORT std::string get_filename(std::string path);
}
