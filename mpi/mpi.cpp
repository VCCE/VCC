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
#include <vcc/core/DialogOps.h>
#include <vcc/core/limits.h>
#include <vcc/core/logger.h>

HINSTANCE gModuleInstance = nullptr;
static std::string gConfigurationFilename;
HWND gVccWnd;

// host_catridge_context (remove someday)
const std::shared_ptr<host_cartridge_context> gHostContext(std::make_shared<host_cartridge_context>(nullptr, gConfigurationFilename));

// Instatiate mpi configuration object (remove someday)
multipak_configuration gMultiPakConfiguration("MPI");

// Instatiate mpi cartridge object (remove someday)
multipak_cartridge gMultiPakInterface(gMultiPakConfiguration, gHostContext);

// Instatiate the config dialog
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
		void* const callback_context,
		const char* const configuration_path,
		HWND hVccWnd,
		const cpak_callbacks* const callbacks)
	{
		gMultiPakConfiguration.configuration_path(configuration_path);
		gConfigurationFilename = configuration_path;
		gVccWnd = hVccWnd;
		gHostContext->add_menu_item_ = callbacks->add_menu_item;
		gHostContext->read_memory_byte_ = callbacks->read_memory_byte;
		gHostContext->write_memory_byte_ = callbacks->write_memory_byte;
		gHostContext->assert_interrupt_ = callbacks->assert_interrupt;
		gHostContext->assert_cartridge_line_ = callbacks->assert_cartridge_line;
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

	// Write to port
	__declspec(dllexport) void PakWritePort(unsigned char port_id,unsigned char value)
	{
		gMultiPakInterface.write_port(port_id, value);
		return;
	}

	// Read from port
	__declspec(dllexport) unsigned char PakReadPort(unsigned char port_id)
	{
		return gMultiPakInterface.read_port(port_id);
	}

	// Reset module
	__declspec(dllexport) unsigned char PakReset()
	{
		gMultiPakInterface.reset();
		return 0;
	}

	__declspec(dllexport)  void PakProcessHorizontalSync()
	{
		gMultiPakInterface.process_horizontal_sync();
	}

	__declspec(dllexport)  unsigned char PakReadMemoryByte(unsigned short memory_address)
	{
		return gMultiPakInterface.read_memory_byte(memory_address);
	}

	// Return MPI status.
	__declspec(dllexport) void PakGetStatus(char* text_buffer, size_t buffer_size)
	{
		gMultiPakInterface.status(text_buffer, buffer_size);
	}

	__declspec(dllexport) unsigned short PakSampleAudio()
	{
		return gMultiPakInterface.sample_audio();
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


