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

extern "C"
{

	// FIXME: this needs to come from the common library but is currently part of the
	// main vcc app. Update this when it is migrated.
	enum MenuItemType;

	using PakWriteMemoryByteHostCallback = void (*)(void* host_key, unsigned char value, unsigned short address);
	using PakReadMemoryByteHostCallback = unsigned char (*)(void* host_key, unsigned short address);
	using PakAssertCartridgeLineHostCallback = void (*)(void* host_key, bool lineState);
	using PakAssertInteruptHostCallback = void (*)(void* host_key, Interrupt interrupt, InterruptSource interrupt_source);
	using PakAppendCartridgeMenuHostCallback = void (*)(void* host_key, const char* menu_name, int menu_id, MenuItemType menu_type);

	struct pak_initialization_parameters
	{
		const PakAssertInteruptHostCallback assert_interrupt;
		const PakAssertCartridgeLineHostCallback assert_cartridge_line;
		const PakWriteMemoryByteHostCallback write_memory_byte;
		const PakReadMemoryByteHostCallback read_memory_byte;
		const PakAppendCartridgeMenuHostCallback add_menu_item;
	};

	using PakInitializeModuleFunction = void (*)(
		void* host_key,
		const char* const configuration_path,
		const pak_initialization_parameters* const parameters);
	using PakGetNameModuleFunction = const char* (*)();
	using PakGetCatalogIdModuleFunction = const char* (*)();
	using PakGetDescriptionModuleFunction = const char* (*)();
	using PakResetModuleFunction = void (*)();
	using PakHeartBeatModuleFunction = void (*)();
	using PakGetStatusModuleFunction = void (*)(char* text_buffer, size_t buffer_size);
	using PakWritePortModuleFunction = void (*)(unsigned char port, unsigned char value);
	using PakReadMemoryByteModuleFunction = unsigned char (*)(unsigned short address);
	using PakReadPortModuleFunction = unsigned char (*)(unsigned char port);
	using PakSampleAudioModuleFunction = unsigned short (*)();
	using PakMenuItemClickedModuleFunction = void (*)(unsigned char itemId);

}
