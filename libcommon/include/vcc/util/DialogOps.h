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
#include <vcc/util/fileutil.h>
#include <vcc/util/logger.h>
#include <Windows.h>
#include <string>

//#ifndef LEGACY_BUILD
#include <shobjidl.h>
//#endif

//-------------------------------------------------------------------------------------------
// CloseCartDialog closes DLL dialog or force exits Vcc if it can not be.
// It should be called by cartridge DLL's when they are unloaded.
//-------------------------------------------------------------------------------------------
void CloseCartDialog(HWND hDlg);
void CenterDialog(HWND hDlg);

//-------------------------------------------------------------------------------------------
// FileDialog wraps dialogs for users to select files.
//
// show() displays the dialog. If Save is TRUE the save dialog is shown
// otherwise the open dialog is shown. If "Owner" is NULL GetActiveWindow() is used.
// The selected filename is placed in "Path"
//
//-------------------------------------------------------------------------------------------

class FileDialog
{
public:
    FileDialog();
    void init();

    // File selection
    bool show(BOOL Save = FALSE, HWND Owner = nullptr);

#ifndef _LEGACY_VCC
    // Folder selection (Win8+ only)
    bool show_folder(HWND Owner = nullptr);
#endif

    void setDefExt(const char* DefExt);
    void setInitialDir(const char* InitialDir);
    void setInitialDir(const std::string& InitialDir);
    void setFilter(const char* Filter);
    void setFlags(unsigned int Flags);
    void setTitle(const char* Title);
    void setpath(const char* File);

    const std::string& getpath() const;
    std::string getdir() const;
    std::string gettype() const;

    void getpath(char* pathCopy, int maxsize) const;
    void getupath(char* pathCopy, int maxsize) const;
    void getdir(char* dir, int maxsize) const;
    void gettype(char* type, int maxsize) const;

    template <size_t S> void getpath(char (&pathCopy)[S]) const { getpath(pathCopy, S); }
    template <size_t S> void getupath(char (&pathCopy)[S]) const { getupath(pathCopy, S); }
    template <size_t S> void getdir(char (&dir)[S]) const { getdir(dir, S); }
    template <size_t S> void gettype(char (&type)[S]) const { gettype(type, S); }

    const char* path() const;
    const char* upath() const;

private:
    OPENFILENAME ofn_;
    std::string sFile_;
    std::string sInitDir_;
    std::string sTitle_;
    std::string sFilter_;
    std::string sDefext_;
    DWORD flags_;
};

