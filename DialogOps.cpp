//-------------------------------------------------------------------------------------------
//
//  DialogOps.c
//
//    Functions common to VCC DLL configuration dialogs should go here.
//
//    This function can be compiled as C++.  It's file extension is .c to avoid confusing
//    the C compiler when compiling it for modules not written for C++. Maybe Someday 
//    there will be no more modules compiled with the C compiler.
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

#include <windows.h>
#include "DialogOps.h"

// FileDialog shows a dialog for user to select a file for open or save.
FileDialog::FileDialog() {
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lpstrTitle = "Choose File";
	ofn.Flags |= OFN_HIDEREADONLY;
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = Path;
	ofn.nMaxFile = sizeof(Path);
}

// FileDialog destructor does nothing
FileDialog::~FileDialog() { }

// FileDialog::show calls GetOpenFileName()
bool FileDialog::show(BOOL Save, HWND Owner) {

	// instance is that of the current module
	ofn.hInstance = GetModuleHandle(0);

	// use active window if owner is null
	if (Owner != NULL) {
		ofn.hwndOwner = Owner;
	} else {
		ofn.hwndOwner = GetActiveWindow();
	}

	// Call Save or Open per boolean
	if (Save) {
		return GetSaveFileName(&ofn) == 1;
	} else {
		return GetOpenFileName(&ofn) == 1;
	}
}

// FileDialog::getdir() returns the directory portion of the path found
void FileDialog::getdir(char * Dir, int maxsize) {
	strncpy(Dir,Path,maxsize);
	if (char * p = strrchr(Dir,'\\')) *p = '\0';
}


// CloseCartDialog should be called by cartridge DLL's when they are unloaded.
void CloseCartDialog(HWND hDlg)
{
	// Send a close message to a cart configuration dialog if it is Enabled. If the dialog
	// is disabled it is assumed that VCC must be terminated to avoid a crash. A common reason
	// a configuration dialog is disabled is it has a modal dialog window open.
	if (hDlg) {
		if (IsWindowEnabled(hDlg)) {
			SendMessage(hDlg,WM_CLOSE,0,0);
		} else {
			MessageBox(0,"A system dialog was left open. VCC will close",
				"Unload Cartridge Error",MB_ICONEXCLAMATION);
			DWORD pid = GetCurrentProcessId();
			HANDLE h = OpenProcess(PROCESS_TERMINATE,FALSE,pid);
			TerminateProcess(h,0);
		}
	}
}
