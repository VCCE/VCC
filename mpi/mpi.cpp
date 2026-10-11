////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	This is an expansion module for the Vcc Emulator. It simulated the functions
//	of the TRS-80 Multi-Pak Interface.
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

#include "mpi.h"
#include "resource.h"
#include <vcc/util/DialogOps.h>
#include <vcc/util/limits.h>
#include <vcc/util/logger.h>
#include <vcc/bus/cartridge_menuitem.h>
#include <vcc/bus/cartridge_messages.h>

HINSTANCE gModuleInstance = nullptr;
static std::string gConfigurationFilename;
HWND gVccWnd;

slot_id_type SlotId = 0;

multipak_configuration gMultiPakConfiguration("MPI");
multipak_supervisor gMultiPakInterface(gMultiPakConfiguration);

// the config dialog
configuration_dialog gConfigurationDialog(gMultiPakConfiguration, gMultiPakInterface);

// DLL exports
extern "C"
{
	__declspec(dllexport) const char* PakGetName()
	{
		static char string_buffer[MAX_LOADSTRING];
		LoadString(gModuleInstance, IDS_MODULE_NAME, string_buffer, MAX_LOADSTRING);
		return string_buffer;
	}

	__declspec(dllexport) const char* PakGetCatalogId()
	{
		static char string_buffer[MAX_LOADSTRING];
		LoadString(gModuleInstance, IDS_CATNUMBER, string_buffer, MAX_LOADSTRING);
		return string_buffer;
	}

	//Initialize MPI -	capture callback addresses and build menus.
	__declspec(dllexport) void PakInitialize(
		slot_id_type SlotId,
		const char* const configuration_path,
		HWND hVccWnd,
		const cpak_callbacks* const callbacks)
	{
		//Prevent MPI load in a multipak slot
		//TODO: enum for VCC error codes
		if (SlotId != 0) {
			int32_t err = 1;
			SendFatalError(hVccWnd, err);
			return;
		}

		gMultiPakConfiguration.configuration_path(configuration_path);
		gConfigurationFilename = configuration_path;
		gVccWnd = hVccWnd;
		gMultiPakInterface.start();
	}

	__declspec(dllexport) void PakTerminate()
	{
		gConfigurationDialog.close();
		gMultiPakInterface.stop();
	}

	__declspec(dllexport) void PakMenuItemClicked(unsigned char menu_item_id)
	{
		gMultiPakInterface.menu_item_clicked(menu_item_id);
	}

	// Fetch menu item list for MPI and carts it has loaded
	__declspec(dllexport) bool PakGetMenuItem(menu_item_entry* item, size_t index)
	{
		return gMultiPakInterface.get_menu_item(item, index);
	}

	// Reset module
	__declspec(dllexport) unsigned char PakReset()
	{
		gMultiPakInterface.reset();
		return 0;
	}
}

// DLLMain
BOOL WINAPI DllMain(HINSTANCE module_instance, DWORD reason, LPVOID /*reserved*/)
{
	switch (reason) {
	case DLL_PROCESS_ATTACH:
		gModuleInstance = module_instance;
		break;
	case DLL_PROCESS_DETACH:
		PakTerminate();
		break;
	}
	return TRUE;
}

