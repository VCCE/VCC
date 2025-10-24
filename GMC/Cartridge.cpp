#include "Cartridge.h"
#include <vcc/core/utils/winapi.h>


namespace detail
{
	void NullAssetCartridgeLine(void*, bool) {}
	void NullAddMenuItem(void*, const char *, int, MenuItemType) {}
}


Cartridge* Cartridge::m_Singleton(nullptr);


Cartridge::Cartridge()
{
	if (m_Singleton)
	{
//		throw std::runtime_error("Cartridge instance already created.");
	}

	m_Singleton = this;
}



Cartridge::~Cartridge()
{
	m_Singleton = nullptr;
}




void Cartridge::LoadConfiguration(const std::string& /*filename*/)
{
}


void Cartridge::LoadMenuItems()
{
}



void Cartridge::SetHostKey(void* key)
{
	m_HostKey = key;
}


void Cartridge::SetCartLineAssertCallback(PakAssertCartridgeLineHostCallback callback)
{
	AssetCartridgeLinePtr = callback;
}


void Cartridge::SetConfigurationPath(std::string path)
{
	m_ConfigurationPath = move(path);
	LoadConfiguration(m_ConfigurationPath);
	LoadMenuItems();
}


void Cartridge::SetMenuBuilderCallback(PakAppendCartridgeMenuHostCallback callback)
{
	AddMenuItemPtr = callback;
}



std::string Cartridge::GetStatusMessage() const
{
	return std::string();
}


void Cartridge::OnMenuItemSelected(unsigned char /*menuId*/)
{

}




void Cartridge::OnReset()
{}




unsigned short Cartridge::UpdateAudio()
{
	return 0;
}




unsigned char Cartridge::OnReadMemory(unsigned short /*address*/) const
{
	return 0;
}


void Cartridge::OnWritePort(unsigned char /*port*/, unsigned char /*data*/)
{}


unsigned char Cartridge::OnReadPort(unsigned char /*port*/) const
{
	return 0;
}


