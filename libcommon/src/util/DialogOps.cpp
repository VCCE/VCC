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
#include <vcc/util/DialogOps.h>
#include <vcc/util/logger.h>
#include <vcc/util/fileutil.h>
#include <string>
#include <filesystem>

//-------------------------------------------------------------------------------------------
// FileDialog class shows a dialog for user to select a file for open or save.
//
// There is an effort to standardize VCC using slashes as directory seperators
// but GetSaveFileName() and GetOpenFileName() are old windows functions that
// do not play well with slashes so there is a bit of converting in and out;
//
//  "path"  refers to reverse slash path
//
//-------------------------------------------------------------------------------------------

// FileDialog::show calls GetOpenFileName() or GetSaveFileName()
bool FileDialog::show(BOOL Save, HWND Owner) {

	char file[MAX_PATH];
	::VCC::Util::copy_to_char(sFile_,file,sizeof(file));
	::VCC::Util::RevDirSlashes(file);
	ofn_.nMaxFile = sizeof(file);
	ofn_.lpstrFile = file;

	char idir[MAX_PATH];
	::VCC::Util::copy_to_char(sInitDir_,idir,sizeof(idir));
	::VCC::Util::RevDirSlashes(idir);
	ofn_.lpstrInitialDir = idir;

	// instance is that of the current module
	ofn_.hInstance = GetModuleHandle(nullptr);

	// use active window if owner is null
	if (Owner != nullptr) {
		ofn_.hwndOwner = Owner;
	} else {
		ofn_.hwndOwner = GetActiveWindow();
	}

	ofn_.lpstrDefExt = sDefext_.c_str();
	ofn_.lpstrFilter = sFilter_.c_str();
	ofn_.lpstrTitle = sTitle_.c_str();
	ofn_.Flags |= flags_;

	// Call Save or Open per boolean
	int rc;
	if (Save) {
		rc = GetSaveFileName(&ofn_);
	} else {
		rc = GetOpenFileName(&ofn_) ;
	}

	// Copy file selected back
	if ((rc == 1) && (*file != '\0')) {
		sFile_.assign(file ? file : "");
		VCC::Util::FixDirSlashes(sFile_);
		return true;
	};

	return false;
}

//------------------------------------------------------------
// Center a dialog box in parent window
//------------------------------------------------------------
void CenterDialog(HWND hDlg)
{
	RECT rPar;
	GetWindowRect(GetParent(hDlg), &rPar);

	RECT rDlg;
	GetWindowRect(hDlg, &rDlg);

	const auto x = rPar.left + (rPar.right - rPar.left - (rDlg.right - rDlg.left)) / 2;
	const auto y = rPar.top + (rPar.bottom - rPar.top - (rDlg.bottom - rDlg.top)) / 2;
	SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
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
