#include "CartridgeTrampolines.h"
#include "Cartridge.h"
#include "resource.h"
#include <vcc/core/limits.h>


GMC_EXPORT const char* PakGetName()
{
	static char string_buffer[MAX_LOADSTRING];

	LoadString(gModuleInstance, IDS_MODULE_NAME, string_buffer, MAX_LOADSTRING);

	return string_buffer;
}

GMC_EXPORT const char* PakCatalogName()
{
	static char string_buffer[MAX_LOADSTRING];

	LoadString(gModuleInstance, IDS_CATNUMBER, string_buffer, MAX_LOADSTRING);

	return string_buffer;
}

GMC_EXPORT const char* PakGetDescription()
{
	static char string_buffer[MAX_LOADSTRING];

	LoadString(gModuleInstance, IDS_DESCRIPTION, string_buffer, MAX_LOADSTRING);

	return string_buffer;
}


GMC_EXPORT void PakInitialize(
	void* const host_key,
	const char* const configuration_path,
	const cpak_cartridge_context* const context)
{
	Cartridge::m_Singleton->SetHostKey(host_key);
	Cartridge::m_Singleton->SetMenuBuilderCallback(context->add_menu_item);
	Cartridge::m_Singleton->SetCartLineAssertCallback(context->assert_cartridge_line);
	Cartridge::m_Singleton->SetConfigurationPath(configuration_path);
}


GMC_EXPORT void PakMenuItemClicked(unsigned char menuId)
{
	Cartridge::m_Singleton->OnMenuItemSelected(menuId);
}


GMC_EXPORT void PakReset()
{
	Cartridge::m_Singleton->OnReset();
}


GMC_EXPORT void PakGetStatus(char* text_buffer, size_t buffer_size)
{
	auto message(Cartridge::m_Singleton->GetStatusMessage());
	if (message.size() > 63)
	{
		message.resize(63);
	}

	strcpy(text_buffer, message.c_str());
}


GMC_EXPORT unsigned char PakReadMemoryByte(unsigned short address)
{
	return Cartridge::m_Singleton->OnReadMemory(address);
}




GMC_EXPORT void PakWritePort(unsigned char port, unsigned char data)
{
	Cartridge::m_Singleton->OnWritePort(port, data);
}


GMC_EXPORT unsigned char PakReadPort(unsigned char port)
{
	return Cartridge::m_Singleton->OnReadPort(port);
}


// This gets called at the end of every scan line 262 Lines * 60 Frames = 15780 Hz 15720
GMC_EXPORT unsigned short PakSampleAudio()
{
	return Cartridge::m_Singleton->UpdateAudio();
}

