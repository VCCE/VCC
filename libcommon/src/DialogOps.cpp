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
#include <vcc/common/DialogOps.h>
#include <Windows.h>

//-------------------------------------------------------------------------------------------
// FileDialog class shows a dialog for user to select a file for open or save.
//-------------------------------------------------------------------------------------------

// FileDialog constructor initializes open file name structure.
FileDialog::FileDialog() {
	ZeroMemory(&ofn_, sizeof(ofn_));
	ofn_.lStructSize = sizeof(ofn_);
	ofn_.Flags = OFN_HIDEREADONLY;
}

// FileDialog::show calls GetOpenFileName() or GetSaveFileName()
bool FileDialog::show(BOOL Save, HWND Owner) {

	// instance is that of the current module
	ofn_.hInstance = GetModuleHandle(nullptr);

	// use active window if owner is null
	if (Owner != nullptr) {
		ofn_.hwndOwner = Owner;
	} else {
		ofn_.hwndOwner = GetActiveWindow();
	}

	ofn_.nMaxFile = sizeof(path_);
	ofn_.lpstrFile = path_;

	// Call Save or Open per boolean
	int rc;
	if (Save) {
		rc = GetSaveFileName(&ofn_);
	} else {
		rc = GetOpenFileName(&ofn_) ;
	}
	return ((rc == 1) && (*path_ != '\0'));
}

void FileDialog::setDefExt(const char * DefExt) {
	ofn_.lpstrDefExt = DefExt;
}

void FileDialog::setInitialDir(const char * InitialDir) {
	ofn_.lpstrInitialDir = InitialDir;
}

void FileDialog::setFilter(const char * Filter) {
	ofn_.lpstrFilter = Filter;
}

void FileDialog::setFlags(unsigned int Flags) {
	ofn_.Flags |= Flags;
}

void FileDialog::setTitle(const char * Title) {
	ofn_.lpstrTitle = Title;
}

// Overwrite what is currently in path_
void FileDialog::setpath(const char * NewPath) {
    if (NewPath == nullptr) return;
	strncpy(path_,NewPath,MAX_PATH);
}

// Get a copy of the selected file path
void FileDialog::getpath(char * PathCopy, int maxsize) const {
    if (PathCopy == nullptr || path_ == nullptr || maxsize < 1) return;
	strncpy(PathCopy,path_,maxsize);
}

// Get a copy of the selected file path with unix dir delimiters
void FileDialog::getupath(char * PathCopy, int maxsize) const {
    if (PathCopy == nullptr || path_ == nullptr || maxsize < 1) return;
    int i = 0;
    while (path_[i] != '\0' && i < maxsize - 1) {
        if (path_[i] == '\\') {
            PathCopy[i] = '/';
        } else {
            PathCopy[i] = path_[i];
        }
        i++;
    }
    PathCopy[i] = '\0';
}

// Get a pointer to the selected file path
const char *FileDialog::path() const
{
	return path_;
}

// FileDialog::getdir() returns the directory portion of the file path
void FileDialog::getdir(char * Dir, int maxsize) const {
    if (Dir == nullptr || path_ == nullptr || maxsize < 1) return;
	strncpy(Dir,path_,maxsize);
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
