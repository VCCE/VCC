#include <Windows.h>
#include "GMCCartridge.h"
#include "../DialogOps.h"

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
/*
	OPENFILENAMEA ofn = { 0 };
	char selectedPathBuffer[MAX_PATH] = { 0 };

	ofn.lStructSize		  = sizeof(ofn);
	ofn.hwndOwner		  = GetActiveWindow();
	ofn.lpstrFilter       =	"ROM Files\0*.ROM\0\0";			// filter string
	ofn.nFilterIndex      = 1 ;								// current filter index
	ofn.lpstrFile         = selectedPathBuffer;				// contains full path and filename on return
	ofn.nMaxFile          = MAX_PATH;						// sizeof lpstrFile
	ofn.lpstrFileTitle    = nullptr;						// filename and extension only
	ofn.nMaxFileTitle     = MAX_PATH ;						// sizeof lpstrFileTitle
	ofn.lpstrInitialDir   = nullptr;						// initial directory
	ofn.lpstrTitle        = "Select GMC ROM file";			// title bar string
	ofn.Flags             = OFN_HIDEREADONLY;
	if (GetOpenFileNameA(&ofn)) {
		selectedPath = selectedPathBuffer;
	} else {
		selectedPath.clear();
	}
*/
	return selectedPath;
}


BOOL WINAPI DllMain(HINSTANCE /*hinstDLL*/, DWORD /*fdwReason*/, LPVOID /*lpReserved*/)
{
	return TRUE;
}

