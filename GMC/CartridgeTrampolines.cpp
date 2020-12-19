#include "CartridgeTrampolines.h"
#include "Cartridge.h"


GMC_EXPORT void ModuleName(char *moduleName, char *catalogId, DYNAMICMENUCALLBACK addMenuCallback)
{
	Cartridge::m_Singleton->SetMenuBuilderCallback(addMenuCallback);

	strcpy(moduleName, Cartridge::m_Singleton->GetName().c_str());
	strcpy(catalogId, Cartridge::m_Singleton->GetCatalogId().c_str());
}


GMC_EXPORT void ModuleConfig(unsigned char menuId)
{
	Cartridge::m_Singleton->OnMenuItemSelected(menuId);
}


GMC_EXPORT void SetIniPath(const char *iniFilePath)
{
	Cartridge::m_Singleton->SetConfigurationPath(iniFilePath);
}


GMC_EXPORT void ModuleReset()
{
	Cartridge::m_Singleton->OnReset();
}


GMC_EXPORT void SetCart(SETCART callback)
{
	Cartridge::m_Singleton->SetCartLineAssertCallback(callback);
}



GMC_EXPORT void ModuleStatus(char *statusBuffer)
{
	auto message(Cartridge::m_Singleton->GetStatusMessage());
	if (message.size() > 63)
	{
		message.resize(63);
	}

	strcpy(statusBuffer, message.c_str());
}


GMC_EXPORT unsigned char PakMemRead8(unsigned short address)
{
	return Cartridge::m_Singleton->OnReadMemory(address);
}




GMC_EXPORT void PackPortWrite(unsigned char port, unsigned char data)
{
	Cartridge::m_Singleton->OnWritePort(port, data);
}


GMC_EXPORT unsigned char PackPortRead(unsigned char port)
{
	return Cartridge::m_Singleton->OnReadPort(port);
}


// This gets called at the end of every scan line 262 Lines * 60 Frames = 15780 Hz 15720
GMC_EXPORT unsigned short ModuleAudioSample()
{
	return Cartridge::m_Singleton->UpdateAudio();
}

