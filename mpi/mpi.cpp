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
static std::string gConfigurationFilename;
// FIXME: The host context will be provided by VCC once the full implementation is complete
const std::shared_ptr<host_cartridge_context> gHostContext(std::make_shared<host_cartridge_context>(nullptr, gConfigurationFilename));
multipak_configuration gMultiPakConfiguration("MPI");
multipak_cartridge gMultiPakInterface(gMultiPakConfiguration, gHostContext);
configuration_dialog gConfigurationDialog(gMultiPakConfiguration, gMultiPakInterface);


BOOL WINAPI DllMain(HINSTANCE module_instance, DWORD reason, LPVOID /*reserved*/)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		gModuleInstance = module_instance;
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}


extern "C"
{

	EXPORT_PUBLIC_API const char* PakGetName()
	{
		static char string_buffer[MAX_LOADSTRING];

		LoadString(gModuleInstance, IDS_MODULE_NAME, string_buffer, MAX_LOADSTRING);

		return string_buffer;
	}

	EXPORT_PUBLIC_API const char* PakGetCatalogId()
	{
		static char string_buffer[MAX_LOADSTRING];

		LoadString(gModuleInstance, IDS_CATNUMBER, string_buffer, MAX_LOADSTRING);

		return string_buffer;
	}

	EXPORT_PUBLIC_API void PakInitialize(
		void* const host_key,
		const char* const configuration_path,
		const cpak_cartridge_context* const context)
	{
		gMultiPakConfiguration.configuration_path(configuration_path);
		gConfigurationFilename = configuration_path;
		gHostContext->add_menu_item_ = context->add_menu_item;
		gHostContext->read_memory_byte_ = context->read_memory_byte;
		gHostContext->write_memory_byte_ = context->write_memory_byte;
		gHostContext->assert_interrupt_ = context->assert_interrupt;
		gHostContext->assert_cartridge_line_ = context->assert_cartridge_line;

		gMultiPakInterface.start();
	}

	__declspec(dllexport) void PakTerminate()
	{
		gConfigurationDialog.close();
		gMultiPakInterface.stop();
	}

}

EXPORT_PUBLIC_API void PakMenuItemClicked(unsigned char menu_item_id)
{
	gMultiPakInterface.menu_item_clicked(menu_item_id);
}

EXPORT_PUBLIC_API void PakWritePort(unsigned char port_id, unsigned char value)
{
	gMultiPakInterface.write_port(port_id, value);
}

EXPORT_PUBLIC_API unsigned char PakReadPort(unsigned char port_id)
{
	return gMultiPakInterface.read_port(port_id);
}

EXPORT_PUBLIC_API void PakProcessHorizontalSync()
{
	gMultiPakInterface.process_horizontal_sync();
}

EXPORT_PUBLIC_API unsigned char PakReadMemoryByte(unsigned short memory_address)
{
	return gMultiPakInterface.read_memory_byte(memory_address);
}

EXPORT_PUBLIC_API void PakGetStatus(char* text_buffer, size_t buffer_size)
{
	gMultiPakInterface.status(text_buffer, buffer_size);
}

EXPORT_PUBLIC_API unsigned short PakSampleAudio()
{
	return gMultiPakInterface.sample_audio();
}

EXPORT_PUBLIC_API unsigned char PakReset()
{
	gMultiPakInterface.reset();

	return 0;
}
