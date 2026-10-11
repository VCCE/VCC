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

//===============================================================//
//    This defines the exports that can be used by plugins       //
//===============================================================//

#pragma once
#include <vcc/util/interrupts.h>
#include <vcc/bus/plugin_callbacks.h>
#include <vcc/bus/cartridge_menuitem.h>
#include <windows.h>

//----------------------------------------------------------------
// ABI identity is based on SlotId; 0 = Boot Slot, 1-4 = MPI slots
//----------------------------------------------------------------
extern "C"
{
	// Cartridge exports. At least PakInitilizeModuleFunction must be implimented
	using PakInitializeModuleFunction = void (*)(
		slot_id_type SlotId,                   // SlotId 
		const char* const configuration_path,  // Path of ini file
		HWND hVccWnd,                          // VCC proxy handle
		const cpak_callbacks* const context);  // Callbacks
	using PakTerminateModuleFunction       = void (*)();
	using PakGetNameModuleFunction         = const char* (*)();
	using PakGetCatalogIdModuleFunction    = const char* (*)();
	using PakGetDescriptionModuleFunction  = const char* (*)();
	using PakResetModuleFunction           = void (*)();
	using PakHeartBeatModuleFunction       = void (*)();
	using PakGetStatusModuleFunction       = void (*)(char* text_buffer, size_t buffer_size);
	using PakWritePortModuleFunction       = void (*)(unsigned char port, unsigned char value);
	using PakReadMemoryByteModuleFunction  = unsigned char (*)(unsigned short address);
	using PakReadPortModuleFunction        = unsigned char (*)(unsigned char port);
	using PakSampleAudioModuleFunction     = unsigned short (*)();
	using PakMenuItemClickedModuleFunction = void (*)(unsigned char itemId);
	using PakGetMenuItemFunction           = bool (*)(menu_item_entry* item, size_t index);
}
