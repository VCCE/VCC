#include "CartridgeTrampolines.h"
#include "Cartridge.h"
#include "resource.h"
#include <vcc/util/limits.h>
#include <vcc/util/logger.h>

//#define USE_LOGGING

GMC_EXPORT const char* PakGetName()
{
	static char string_buffer[MAX_LOADSTRING];

	LoadString(gModuleInstance, IDS_MODULE_NAME, string_buffer, MAX_LOADSTRING);

	return string_buffer;
}

GMC_EXPORT const char* PakGetCatalogId()
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
	slot_id_type SlotId,
	const char* const configuration_path,
	HWND hVccWnd,
	const cpak_callbacks* const callbacks)
{
    DLOG_C("GMC %p %p %p %p %p\n",
            callbacks->assert_interrupt,
            callbacks->assert_cartridge_line,
            callbacks->write_memory_byte,
            callbacks->read_memory_byte,
            callbacks->add_menu_item);

	Cartridge::m_Singleton->SetSlotId(SlotId);
	Cartridge::m_Singleton->SetMenuBuilderCallback(callbacks->add_menu_item);
	Cartridge::m_Singleton->SetCartLineAssertCallback(callbacks->assert_cartridge_line);
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

// Return menu
GMC_EXPORT bool PakGetMenuItem(menu_item_entry* item, size_t index)
{
	return Cartridge::m_Singleton->GetMenuItem(item, index);
}

