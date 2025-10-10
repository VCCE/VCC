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
#include <vcc/core/detail/exports.h>
#include <Windows.h>

//-------------------------------------------------------------------------------------------
// CloseCartDialog closes DLL dialog or force exits Vcc if it can not be.
// It should be called by cartridge DLL's when they are unloaded.
//-------------------------------------------------------------------------------------------
LIBCOMMON_EXPORT void CloseCartDialog(HWND hDlg);

//-------------------------------------------------------------------------------------------
// FileDialog wraps dialogs for users to select files.
//
// show() displays the dialog. If Save is TRUE the save dialog is shown 
// otherwise the open dialog is shown. If "Owner" is NULL GetActiveWindow() is used.
// The selected filename is placed in "Path" 
//
// setpath() sets "Path" before calling show().
//
// setDefExt(), setInitialDir(), setFilter(), setFlags(), and setTitle() set the
// elements in the OPENFILENAME structure that is used by get save/open file()
//
// path() returns a pointer to Path.
// getpath gets a copy of "Path".
// getdir() returns a copy of the directory portion of "Path"
// getupath() gets a copy of "Path" with all '\' chars replaced by '/'
//
//-------------------------------------------------------------------------------------------
class LIBCOMMON_EXPORT FileDialog
{
public:

	FileDialog();

	bool show(BOOL Save = FALSE, HWND Owner = nullptr);
	void setpath(const char * Path);
	void setDefExt(const char * DefExt);
	void setInitialDir(const char * InitialDir);
	void setFilter(const char * Filter);
	void setFlags(unsigned int Flags);
	void setTitle(const char * Title);
	void getdir(char * Dir, int maxsize = MAX_PATH) const;
	void getpath(char * Path, int maxsize = MAX_PATH) const;
	void getupath(char * Path, int maxsize = MAX_PATH) const;
	const char *path() const;


private:

	OPENFILENAME ofn_;
	char path_[MAX_PATH] = {};
};

