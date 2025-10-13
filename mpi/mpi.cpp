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
#include <vcc/common/DialogOps.h>
#include <vcc/core/limits.h>


#define EXPORT_PUBLIC_API extern "C" __declspec(dllexport)

HINSTANCE gModuleInstance = nullptr;
const std::shared_ptr<host_cartridge_context> gHostContext(std::make_shared<host_cartridge_context>());
multipak_cartridge gMultiPakInterface(gHostContext);
configuration_dialog gConfigurationDialog(gMultiPakInterface);
std::string gConfigurationFilename;


BOOL WINAPI DllMain(HINSTANCE module_instance, DWORD reason, LPVOID /*reserved*/)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		gModuleInstance = module_instance;
		break;

	case DLL_PROCESS_DETACH:
		gConfigurationDialog.close();
		gMultiPakInterface.eject_all();
		break;
	}

	return TRUE;
}


EXPORT_PUBLIC_API void ModuleName(
	char* module_name_buffer,
	char* catalog_id_buffer,
	AppendCartridgeMenuModuleCallback append_menu_item_callback)
{
	LoadString(gModuleInstance, IDS_MODULE_NAME, module_name_buffer, MAX_LOADSTRING);
	LoadString(gModuleInstance, IDS_CATNUMBER, catalog_id_buffer, MAX_LOADSTRING);
	gHostContext->add_menu_item_ = append_menu_item_callback;
}

// This captures the pointers to the HostReadMemoryByte and HostWriteMemoryByte functions.
// This allows the DLL to do DMA xfers with CPU ram.
EXPORT_PUBLIC_API void MemPointers(
	ReadMemoryByteModuleCallback read_memory_byte_callback,
	WriteMemoryByteModuleCallback write_memory_byte_callback)
{
	gHostContext->read_memory_byte_ = read_memory_byte_callback;
	gHostContext->write_memory_byte_ = write_memory_byte_callback;
}

// This captures the Function transfer point for the CPU assert interupt
EXPORT_PUBLIC_API void AssertInterupt(AssertInteruptModuleCallback assert_interupt_callback)
{
	gHostContext->assert_interrupt_ = assert_interupt_callback;
}

EXPORT_PUBLIC_API void SetCart(AssertCartridgeLineModuleCallback callback)
{
	gHostContext->assert_cartridge_line_ = callback;
}

EXPORT_PUBLIC_API void SetIniPath(const char* configuration_path)
{
	gConfigurationFilename = configuration_path;
	gMultiPakInterface.start();
}

EXPORT_PUBLIC_API void ModuleConfig(unsigned char menu_item_id)
{
	gMultiPakInterface.menu_item_clicked(menu_item_id);
}

EXPORT_PUBLIC_API void PackPortWrite(unsigned char port_id, unsigned char value)
{
	gMultiPakInterface.write_port(port_id, value);
}

EXPORT_PUBLIC_API unsigned char PackPortRead(unsigned char port_id)
{
	return gMultiPakInterface.read_port(port_id);
}

EXPORT_PUBLIC_API void HeartBeat()
{
	gMultiPakInterface.heartbeat();
}

EXPORT_PUBLIC_API unsigned char PakMemRead8(unsigned short memory_address)
{
	return gMultiPakInterface.read_memory_byte(memory_address);
}

EXPORT_PUBLIC_API void ModuleStatus(char* status_buffer)
{
	gMultiPakInterface.status(status_buffer);
}

EXPORT_PUBLIC_API unsigned short ModuleAudioSample()
{
	return gMultiPakInterface.sample_audio();
}

EXPORT_PUBLIC_API unsigned char ModuleReset()
{
	gMultiPakInterface.reset();

	return 0;
}
