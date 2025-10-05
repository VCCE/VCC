#include <Windows.h>
#include "GMCCartridge.h"
#include <vcc/common/DialogOps.h>

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


BOOL WINAPI DllMain(HINSTANCE /*hinstDLL*/, DWORD /*fdwReason*/, LPVOID /*lpReserved*/)
{
	return TRUE;
}

