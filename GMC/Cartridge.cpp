////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
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
#include "Cartridge.h"
#include <vcc/util/winapi.h>


namespace detail
{
	void NullAssetCartridgeLine(slot_id_type, bool) {}
	void NullAddMenuItem(slot_id_type, const char *, int, MenuItemType) {}
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


void Cartridge::SetSlotId(slot_id_type SlotId)
{
	m_SlotId = SlotId;
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

bool Cartridge::GetMenuItem(menu_item_entry* /*item*/, size_t /*index*/)
{
	return false;
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


