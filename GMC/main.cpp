#include "GMCCartridge.h"
#include <windows.h>

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
	OPENFILENAMEA ofn = { 0 };
	char selectedPathBuffer[MAX_PATH] = { 0 };

	ofn.lStructSize		  = sizeof(ofn);
	ofn.hwndOwner		  = NULL;
	ofn.lpstrFilter       =	"ROM Files\0*.ROM\0\0";			// filter string
	ofn.nFilterIndex      = 1 ;								// current filter index
	ofn.lpstrFile         = selectedPathBuffer;				// contains full path and filename on return
	ofn.nMaxFile          = MAX_PATH;						// sizeof lpstrFile
	ofn.lpstrFileTitle    = NULL;							// filename and extension only
	ofn.nMaxFileTitle     = MAX_PATH ;						// sizeof lpstrFileTitle
	ofn.lpstrInitialDir   = NULL;							// initial directory
	ofn.lpstrTitle        = "Select ROM file";				// title bar string
	ofn.Flags             = OFN_HIDEREADONLY;
	if (GetOpenFileNameA(&ofn)) {
		selectedPath = selectedPathBuffer;
	} else {
		selectedPath.clear();
	}

	return move(selectedPath);
}


BOOL WINAPI DllMain(HINSTANCE /*hinstDLL*/, DWORD /*fdwReason*/, LPVOID /*lpReserved*/)
{
	return TRUE;
}

