#include "Configuration.h"
#include <io.h>
#include <Windows.h>



Configuration::Configuration(std::string filename)
	: m_Filename(move(filename))
{
	static const auto moduleName = "GMC-SN74689";
	char pathBuffer[MAX_PATH];
	GetPrivateProfileStringA(moduleName, "ROM", "", pathBuffer, MAX_PATH, m_Filename.c_str());
	std::string activeRom(pathBuffer);


	MRUListType mruROMList;
	for (auto i(0U); ; ++i)
	{
		const auto keyName("ROM" + std::to_string(i));

		if (!GetPrivateProfileStringA(moduleName, keyName.c_str(), "", pathBuffer, MAX_PATH, m_Filename.c_str()))
		{
			break;
		}

		mruROMList.emplace_back(pathBuffer);
	}

	m_ActiveROM = move(activeRom);
	m_MRURoms = move(mruROMList);

}



std::string Configuration::GetActiveRom() const
{
	return m_ActiveROM;
}


void Configuration::SetActiveRom(std::string filename)
{
	m_MRURoms.emplace_back(m_ActiveROM);
	m_ActiveROM = filename;

	static const auto moduleName = "GMC-SN74689";
	WritePrivateProfileStringA(moduleName, "ROM", filename.data(), m_Filename.c_str());
}
