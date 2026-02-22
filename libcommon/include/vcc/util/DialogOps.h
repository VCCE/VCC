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

	FileDialog() {
		ZeroMemory(&ofn_, sizeof(ofn_));
		ofn_.lStructSize = sizeof(ofn_);
		flags_ = OFN_HIDEREADONLY;
	}

	void init() {
		ZeroMemory(&ofn_, sizeof(ofn_));
		ofn_.lStructSize = sizeof(ofn_);
		flags_ = OFN_HIDEREADONLY;
	}

	bool show(BOOL Save = FALSE, HWND Owner = nullptr);

	void setDefExt(const char * DefExt)
	{
		sDefext_.assign(DefExt ? DefExt : "");
	}

	void setInitialDir(const char * InitialDir)
	{
		sInitDir_.assign(InitialDir ? InitialDir : "");
	}

    //Set null terminated Filter items
	void setFilter(const char* Filter)
	{
        sFilter_.clear();
    	if (!Filter) return;
    	const char* p = Filter;
    	while (*p) p += std::strlen(p) + 1;
    	sFilter_.assign(Filter, p + 1 - Filter);
	}

	void setFlags(unsigned int Flags)
	{
		flags_ = Flags | OFN_HIDEREADONLY;
	}

	void setTitle(const char * Title)
	{
		sTitle_.assign(Title ? Title : "");
	}

	void setpath(const char * File)
	{
		sFile_.assign(File ? File : "");
	}

	// Return selected file
	std::string getpath() const
	{
		return sFile_;
	}

	// Return selected directory
	std::string getdir() const
	{
		return VCC::Util::GetDirectoryPart(sFile_);
	}

	// Return selected filetype
	std::string gettype() const
	{
		std::string s = VCC::Util::GetFileNamePart(sFile_);
		size_t pos = s.rfind('.');
		if (pos == std::string::npos || pos == s.size()-1) return {};
		return s.substr(pos+1);
	}

	// Get a copy of the selected file path
	void getpath(char * PathCopy, int maxsize=MAX_PATH) const
	{
    	if (PathCopy == nullptr) return;
		strncpy(PathCopy,sFile_.c_str(),maxsize);
	}

	// Copy of the selected file path
	void getupath(char * PathCopy, int maxsize=MAX_PATH) const
	{
    	if (PathCopy == nullptr) return;
		strncpy(PathCopy,sFile_.c_str(),maxsize);
	}

	// copy the directory to c string
	void getdir(char * Dir, int maxsize=MAX_PATH) const
	{
    	if (Dir == nullptr) return;
		VCC::Util::copy_to_char(getdir(),Dir,maxsize);
	}

	// copy the file type to c string
	void gettype(char * Type, int maxsize=16) const
	{
    	if (Type == nullptr) return;
		VCC::Util::copy_to_char(gettype(),Type,maxsize);
	}

	// Get a pointer to the selected file path
	const char *path() const
	{
		return sFile_.c_str();
	}

	const char *upath() const
	{
		return sFile_.c_str();
	}

private:

	OPENFILENAME ofn_;
	std::string sFile_ {};
	std::string sInitDir_ {};
	std::string sTitle_ {};
	std::string sFilter_ {};
	std::string sDefext_ {};
	DWORD flags_;
};

