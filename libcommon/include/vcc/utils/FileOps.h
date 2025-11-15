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
#include "vcc/detail/exports.h"
#include <Windows.h>


LIBCOMMON_EXPORT void PathStripPath(char*);
LIBCOMMON_EXPORT void ValidatePath(char* Path);
LIBCOMMON_EXPORT [[nodiscard]] int CheckPath(char*);
LIBCOMMON_EXPORT [[nodiscard]] BOOL PathRemoveFileSpec(char*);
LIBCOMMON_EXPORT [[nodiscard]] BOOL PathRemoveExtension(char*);
LIBCOMMON_EXPORT [[nodiscard]] char* PathFindExtension(char*);
LIBCOMMON_EXPORT [[nodiscard]] DWORD WritePrivateProfileInt(LPCTSTR, LPCTSTR, int, LPCTSTR);
LIBCOMMON_EXPORT [[nodiscard]] BOOL FilePrintf(HANDLE, const char*, ...);

