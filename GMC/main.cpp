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
#include <Windows.h>
#include "GMCCartridge.h"
#include <vcc/core/DialogOps.h>

HINSTANCE gModuleInstance = nullptr;
GMCCartridge theCart;

std::string ExtractFilename(std::string path)
{
	char filename[MAX_PATH];
	char ext[MAX_PATH];

	_splitpath(path.c_str(), nullptr, nullptr, filename, ext);
	path = filename;
	if (ext[0])
	{
		path += ext;
	}

	return path;
}


std::string SelectROMFile()
{
	std::string selectedPath;

	FileDialog dlg;
	dlg.setFilter("ROM Files\0*.ROM\0\0");
	dlg.setDefExt("rom");
	dlg.setTitle("Select GMC Rom file");
	if (dlg.show()) {
		selectedPath = dlg.path();
	} else {
		selectedPath.clear();
	}
	return selectedPath;
}


BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD reason, LPVOID foo)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		gModuleInstance = hinst;
	}

	return TRUE;
}

