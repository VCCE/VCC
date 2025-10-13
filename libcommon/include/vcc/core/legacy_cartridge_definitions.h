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
#pragma once
#include <vcc/core/interrupts.h>

// FIXME: this needs to come from the common library but is currently part of the
// main vcc app. Update this when it is migrated.
enum MenuItemType;

using WriteMemoryByteModuleCallback = void (*)(unsigned char value, unsigned short address);
using ReadMemoryByteModuleCallback = unsigned char (*)(unsigned short address);
using AssertCartridgeLineModuleCallback = void (*)(bool lineState);
using AssertInteruptModuleCallback = void (*)(Interrupt interrupt, InterruptSource interrupt_source);
using AppendCartridgeMenuModuleCallback = void (*)(const char* menu_name, int menu_id, MenuItemType menu_type);

using GetNameModuleFunction = void (*)(char* name_text, char* catalog_id_text, AppendCartridgeMenuModuleCallback addMenuItemCallback);
using SetDMACallbacksModuleFunction = void (*)(ReadMemoryByteModuleCallback, WriteMemoryByteModuleCallback);
using SetAssertInterruptCallbackModuleFunction = void (*)(AssertInteruptModuleCallback callback);
using SetConfigurationPathModuleFunction = void (*)(const char* path);
using SetAssertCartridgeLineCallbackModuleFunction = void (*)(AssertCartridgeLineModuleCallback callback);
using ResetModuleFunction = void (*)();
using HeartBeatModuleFunction = void (*)();
using GetStatusModuleFunction = void (*)(char* status_text);
using WritePortModuleFunction = void (*)(unsigned char port, unsigned char value);
using ReadMemoryByteModuleFunction = unsigned char (*)(unsigned short address);
using ReadPortModuleFunction = unsigned char (*)(unsigned char port);
using SampleAudioModuleFunction = unsigned short (*)();
using MenuItemClickedModuleFunction = void (*)(unsigned char itemId);
