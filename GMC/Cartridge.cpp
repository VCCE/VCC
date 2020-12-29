#include "Cartridge.h"


namespace detail
{
	void NullAssetCartridgeLine(unsigned char) {}
	void NullAddMenuItem(const char *, int, int) {}
}


Cartridge* Cartridge::m_Singleton(nullptr);


Cartridge::Cartridge(std::string name, std::string catalogId)
	:
	m_Name(move(name)),
	m_CatalogId(move(catalogId)),
	AssetCartridgeLinePtr(detail::NullAssetCartridgeLine),
	AddMenuItemPtr(detail::NullAddMenuItem)
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


void Cartridge::LoadMenu()
{
}




std::string Cartridge::GetName() const
{
	return m_Name;
}


std::string Cartridge::GetCatalogId() const
{
	return m_CatalogId;
}




void Cartridge::SetCartLineAssertCallback(SETCART callback)
{
	AssetCartridgeLinePtr = callback;
}


void Cartridge::SetConfigurationPath(std::string path)
{
	m_ConfigurationPath = move(path);
	LoadConfiguration(m_ConfigurationPath);
	LoadMenu();
}


void Cartridge::SetMenuBuilderCallback(DYNAMICMENUCALLBACK callback)
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


