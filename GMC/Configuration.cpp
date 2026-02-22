#include "Configuration.h"
#include <io.h>
#include <Windows.h>
#include <vcc/util/settings.h>

using settings = ::VCC::Util::settings;

Configuration::Configuration(std::string filename)
	: m_Filename(move(filename))
{
	m_ActiveROM = settings(m_Filename).read("GMC-SN74689","ROM");

//Chet had plans for an MRU list.  maybe someday...
//	MRUListType mruROMList;
//	for (auto i(0U); ; ++i)
//	{
//		const auto keyName("ROM" + std::to_string(i));
//		if (!GetPrivateProfileStringA(moduleName, keyName.c_str(), "",
//						pathBuffer, MAX_PATH, m_Filename.c_str()))
//			break;
//		mruROMList.emplace_back(pathBuffer);
//	}
//	m_MRURoms = move(mruROMList);
}

std::string Configuration::GetActiveRom() const
{
	return m_ActiveROM;
}

void Configuration::SetActiveRom(std::string filename)
{
	settings(m_Filename).write("GMC-SN74689","ROM",filename);

//	m_MRURoms.emplace_back(m_ActiveROM);
//	m_ActiveROM = filename;
//	static const auto moduleName = "GMC-SN74689";
//	WritePrivateProfileStringA(moduleName, "ROM", filename.data(), m_Filename.c_str());

}
