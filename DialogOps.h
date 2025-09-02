//-------------------------------------------------------------------------------------------
//
//  DialogOps.h
//
//    Functions common to VCC configuration dialogs
//
//    This file is part of VCC (Virtual Color Computer Copyright 2015 by Joseph Forgione
//
//    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
//
//-------------------------------------------------------------------------------------------
#pragma once

// CloseCartDialog closes DLL dialog or asserts that it can not be.
// It should be called by cartridge DLL's when they are unloaded.
void CloseCartDialog(HWND hDlg);

// FileDialog defines a dialog for users to select a file.
//
// "show" displays the dialog. If Save is TRUE the save dialog is shown 
// otherwise the open dialog is shown. If "Owner" is NULL GetActiveWindow() is used.
// The selected filename is placed in "Path" 
//
// "getdir" returns the directory portion of the choosen path
//
// "setpath" modifies "Path".
// 
// Most ofn (type OPENFILENAME) members can be set before running "show"
// "ofn.hwndOwner", "ofn.lpstrFile", and "ofn.nMaxFile" are overwitten 
// by "show" and setting them will have no effect.
//
class FileDialog {
public:
	FileDialog();
	~FileDialog();
	bool show(BOOL Save = FALSE, HWND Owner = NULL);
	void getdir(char * Dir, int maxsize = MAX_PATH);
	void setpath(const char * Path, int maxsize = MAX_PATH);
	OPENFILENAME ofn;
	char Path[MAX_PATH] = {};
};

