////////////////////////////////////////////////////////////////////////////////
//    Copyright 2015 by Joseph Forgione
//    This file is part of VCC (Virtual Color Computer).
//
//    VCC (Virtual Color Computer) is free software: you can redistribute itand/or
//    modify it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or (at your
//    option) any later version.
//
//    VCC (Virtual Color Computer) is distributed in the hope that it will be
//    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
//    Public License for more details.
//
//    You should have received a copy of the GNU General Public License along with
//    VCC (Virtual Color Computer). If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////
#include <vcc/util/DialogOps.h>
#include <vcc/util/logger.h>
#include <vcc/util/fileutil.h>
#include <vcc/util/textutil.h>
#include <string>
#include <filesystem>

#ifndef _LEGACY_VCC
#include <shobjidl.h>
#endif

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


using namespace VCC::Util;

FileDialog::FileDialog()
{
    init();
}

void FileDialog::init()
{
    ZeroMemory(&ofn_, sizeof(ofn_));
    ofn_.lStructSize = sizeof(ofn_);
    flags_ = OFN_HIDEREADONLY;
}

void FileDialog::setDefExt(const char* DefExt)
{
    sDefext_.assign(DefExt ? DefExt : "");
}

void FileDialog::setInitialDir(const char* InitialDir)
{
    sInitDir_.assign(InitialDir ? InitialDir : "");
}

void FileDialog::setInitialDir(const std::string& InitialDir)
{
    sInitDir_ = InitialDir;
}

void FileDialog::setFilter(const char* Filter)
{
    sFilter_.clear();
    if (!Filter) return;

    const char* p = Filter;
    while (*p) p += std::strlen(p) + 1;
    sFilter_.assign(Filter, p + 1 - Filter);
}

void FileDialog::setFlags(unsigned int Flags)
{
    flags_ = Flags | OFN_HIDEREADONLY;
}

void FileDialog::setTitle(const char* Title)
{
    sTitle_.assign(Title ? Title : "");
}

void FileDialog::setpath(const char* File)
{
    sFile_.assign(File ? File : "");
}

std::string FileDialog::getpath() const
{
    return sFile_;
}

std::string FileDialog::getdir() const
{
    return GetDirectoryPart(sFile_);
}

std::string FileDialog::gettype() const
{
    std::string s = sFile_;
    size_t pos = s.rfind('.');
    if (pos == std::string::npos || pos == s.size() - 1) return {};
    return s.substr(pos + 1);
}

void FileDialog::getpath(char* PathCopy, int maxsize) const
{
    if (!PathCopy) return;
    strncpy(PathCopy, sFile_.c_str(), maxsize);
}

void FileDialog::getupath(char* PathCopy, int maxsize) const
{
    if (!PathCopy) return;
    strncpy(PathCopy, sFile_.c_str(), maxsize);
}

void FileDialog::getdir(char* Dir, int maxsize) const
{
    if (!Dir) return;
    std::string d = getdir();
    strncpy(Dir, d.c_str(), maxsize);
}

void FileDialog::gettype(char* Type, int maxsize) const
{
    if (!Type) return;
    std::string t = gettype();
    strncpy(Type, t.c_str(), maxsize);
}

const char* FileDialog::path() const
{
    return sFile_.c_str();
}

const char* FileDialog::upath() const
{
    return sFile_.c_str();
}

//=============================================
// FileDialog::show calls GetOpenFileName() or GetSaveFileName()
//=============================================

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

//==================================================
// FileDialog::show_folder is defined if not legacy
//==================================================

#ifndef _LEGACY_VCC
bool FileDialog::show_folder(HWND Owner)
{
    HRESULT hr = CoInitializeEx(nullptr,
        COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (FAILED(hr))
        return false;

    IFileOpenDialog* dlg = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&dlg));

    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    DWORD opts = 0;
    dlg->GetOptions(&opts);
    dlg->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

    if (!sTitle_.empty()) {
        dlg->SetTitle(UTF8ToWide(sTitle_).c_str());
    }

    if (!sInitDir_.empty()) {
        IShellItem* folder = nullptr;
        std::wstring w = UTF8ToWide(sInitDir_);
        RevDirSlashes(w);
        if (SUCCEEDED(SHCreateItemFromParsingName(w.c_str(), nullptr,
                                          IID_PPV_ARGS(&folder)))) {
            dlg->SetFolder(folder);
            folder->Release();
        }
    }

    hr = dlg->Show(Owner);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        dlg->Release();
        CoUninitialize();
        return false;
    }

    if (FAILED(hr)) {
        dlg->Release();
        CoUninitialize();
        return false;
    }

    IShellItem* item = nullptr;
    hr = dlg->GetResult(&item);
    if (SUCCEEDED(hr)) {
        PWSTR pszPath = nullptr;
        if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
            sFile_ = WideToUTF8(pszPath);
            CoTaskMemFree(pszPath);
        }
        item->Release();
    }

    dlg->Release();
    CoUninitialize();
    return !sFile_.empty();
}
#endif

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
