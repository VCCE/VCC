//-------------------------------------------------------------------------------------------
//
//  DialogOps.c
//
//    Functions common to VCC configuration dialogs should go here.
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

#include <Windows.h>
#include "DialogOps.h"

//-------------------------------------------------------------------------------------------
// FileDialog class shows a dialog for user to select a file for open or save.
//-------------------------------------------------------------------------------------------

// FileDialog constructor initializes open file name structure.
FileDialog::FileDialog() {
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.Flags = OFN_HIDEREADONLY;
}

// FileDialog::show calls GetOpenFileName() or GetSaveFileName()
bool FileDialog::show(BOOL Save, HWND Owner) {

	// instance is that of the current module
	ofn.hInstance = GetModuleHandle(nullptr);

	// use active window if owner is null
	if (Owner != nullptr) {
		ofn.hwndOwner = Owner;
	} else {
		ofn.hwndOwner = GetActiveWindow();
	}

	ofn.nMaxFile = sizeof(Path);
	ofn.lpstrFile = Path;

	// Call Save or Open per boolean
	int rc;
	if (Save) {
		rc = GetSaveFileName(&ofn);
	} else {
		rc = GetOpenFileName(&ofn) ;
	}
	return ((rc == 1) && (*Path != '\0'));
}

void FileDialog::setDefExt(const char * DefExt) {
	ofn.lpstrDefExt = DefExt;
}

void FileDialog::setInitialDir(const char * InitialDir) {
	ofn.lpstrInitialDir = InitialDir;
}

void FileDialog::setFilter(const char * Filter) {
	ofn.lpstrFilter = Filter;
}

void FileDialog::setFlags(unsigned int Flags) {
	ofn.Flags |= Flags;
}

void FileDialog::setTitle(const char * Title) {
	ofn.lpstrTitle = Title;
}

// Overwrite what is currently in Path
void FileDialog::setpath(const char * NewPath) {
    if (NewPath == nullptr) return;
	strncpy(Path,NewPath,MAX_PATH);
}

// Get a copy of the selected file path
void FileDialog::getpath(char * PathCopy, int maxsize) const {
    if (PathCopy == nullptr || Path == nullptr || maxsize < 1) return;
	strncpy(PathCopy,Path,maxsize);
}

// Get a copy of the selected file path with unix dir delimiters
void FileDialog::getupath(char * PathCopy, int maxsize) const {
    if (PathCopy == nullptr || Path == nullptr || maxsize < 1) return;
    int i = 0;
    while (Path[i] != '\0' && i < maxsize - 1) {
        if (Path[i] == '\\') {
            PathCopy[i] = '/';
        } else {
            PathCopy[i] = Path[i];
        }
        i++;
    }
    PathCopy[i] = '\0';
}

// Get a pointer to the selected file path
char * FileDialog::path() {
	return Path;
}

// FileDialog::getdir() returns the directory portion of the file path
void FileDialog::getdir(char * Dir, int maxsize) const {
    if (Dir == nullptr || Path == nullptr || maxsize < 1) return;
	strncpy(Dir,Path,maxsize);
	if (char * p = strrchr(Dir,'\\')) *p = '\0';
}

//-------------------------------------------------------------------------------------------
// CloseCartDialog should be called by cartridge DLL's when they are unloaded.
//-------------------------------------------------------------------------------------------
void CloseCartDialog(HWND hDlg)
{
	// Send a close message to a cart configuration dialog if it is Enabled. If the dialog
	// is disabled it is assumed that VCC must be terminated to avoid a crash. A common
	// reason a configuration dialog is disabled is it has a modal dialog window open.
	if (hDlg) {
		if (IsWindowEnabled(hDlg)) {
			SendMessage(hDlg,WM_CLOSE,0,0);
		} else {
			MessageBox(nullptr,"A system dialog was left open. VCC will close",
				"Unload Cartridge Error",MB_ICONEXCLAMATION);
			DWORD pid = GetCurrentProcessId();
			HANDLE h = OpenProcess(PROCESS_TERMINATE,FALSE,pid);
			TerminateProcess(h,0);
		}
	}
}
