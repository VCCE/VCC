/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include "vcc/ui/menu/menu_item_collection.h"
#include "vcc/utils/cartridge_loader.h"
#include <vcc/utils/cartridge_catalog.h>
#include <string>
#include <filesystem>

std::filesystem::path PakGetSystemCartridgePath();
std::filesystem::path PakGetLastAccessedRomPakPath();
void PakSetLastAccessedRomPakPath(const std::filesystem::path& path);
::vcc::utils::cartridge_loader_status PakInsertRom(const std::filesystem::path& filename);
::vcc::utils::cartridge_loader_status PakInsertCartridge(const ::vcc::utils::cartridge_catalog::guid_type& id);
void PakEjectCartridge(bool reset_machine);
vcc::utils::cartridge_loader_status PakLoadCartridge(const char* filename);

std::string PakGetName();
void PakTimer();
unsigned char PakReadPort (unsigned char);
void PakWritePort(unsigned char,unsigned char);
unsigned char PackMem8Read (unsigned short);
void GetModuleStatus( SystemState *);
unsigned short PackAudioSample();
void ResetBus();
void UpdateBusPointer();
void CartMenuActivated(unsigned int);
::vcc::ui::menu::menu_item_collection PakGetMenuItems();

extern const vcc::utils::cartridge_catalog cartridge_catalog_;
