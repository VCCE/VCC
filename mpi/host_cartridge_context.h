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
#include <vcc/bus/cpak_cartridge_definitions.h>

// Define the CPAK interface in yet another place but call it something else.

extern "C" __declspec(dllexport) void PakInitialize(
	void* const callback_context,
	const char* const configuration_path,
	HWND hVccWnd,
	const cpak_callbacks* const callbacks);

// FIXME: this should be unnecessary here. VCC (or the 'host') should provide it
class host_cartridge_context : public ::vcc::core::cartridge_context
{
public:

	// FIXME: Remove this when we derive from cartridge_context
	using path_type = std::string;


public:

	explicit host_cartridge_context(void* callback_context, const path_type& configuration_filename)
		:
		callback_context_(callback_context),
		configuration_filename_(configuration_filename)
	{}

	path_type configuration_path() const override
	{
		return configuration_filename_;
	}

	void reset() override
	{
		// FIXME-CHET: this needs to do something
	}

	void write_memory_byte(unsigned char value, unsigned short address) override
	{
		write_memory_byte_(callback_context_, value, address);
	}

	unsigned char read_memory_byte(unsigned short address) override
	{
		return read_memory_byte_(callback_context_, address);
	}

	void assert_cartridge_line(bool line_state) override
	{
		assert_cartridge_line_(callback_context_, line_state);
	}

	void assert_interrupt(Interrupt interrupt, InterruptSource interrupt_source) override
	{
		assert_interrupt_(callback_context_, interrupt, interrupt_source);
	}

	void add_menu_item(const char* menu_name, int menu_id, MenuItemType menu_type) override
	{
		add_menu_item_(callback_context_, menu_name, menu_id, menu_type);
	}

private:

	friend void PakInitialize(
		void* const callback_context,
		const char* const configuration_path,
		HWND hVccWnd,
		const cpak_callbacks* const callbacks);
	friend class multipak_cartridge;


private:

	void* const							callback_context_;
	const path_type&					configuration_filename_;
	PakWriteMemoryByteHostCallback		write_memory_byte_ = [](void*, unsigned char, unsigned short) {};
	PakReadMemoryByteHostCallback		read_memory_byte_ = [](void*, unsigned short) -> unsigned char { return 0; };
	PakAssertCartridgeLineHostCallback	assert_cartridge_line_ = [](void*, bool) {};
	PakAssertInteruptHostCallback		assert_interrupt_ = [](void*, Interrupt, InterruptSource) {};
	PakAppendCartridgeMenuHostCallback	add_menu_item_ = [](void*, const char*, int, MenuItemType) {};
};
