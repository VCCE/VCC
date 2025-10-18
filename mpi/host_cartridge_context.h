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
#include <vcc/core/cartridge_context.h>
#include <vcc/core/legacy_cartridge_definitions.h>

extern "C" __declspec(dllexport) void ModuleName(char*, char*, AppendCartridgeMenuModuleCallback);
extern "C" __declspec(dllexport) void MemPointers(ReadMemoryByteModuleCallback, WriteMemoryByteModuleCallback);
extern "C" __declspec(dllexport) void AssertInterupt(AssertInteruptModuleCallback);
extern "C" __declspec(dllexport) void SetCart(AssertCartridgeLineModuleCallback);

class host_cartridge_context : public ::vcc::core::cartridge_context
{
public:

	// FIXME: Remove this when we derive from cartridge_context
	using path_type = ::std::string;


public:

	explicit host_cartridge_context(const path_type& configuration_filename)
		: configuration_filename_(configuration_filename)
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
		write_memory_byte_(value, address);
	}

	unsigned char read_memory_byte(unsigned short address) override
	{
		return read_memory_byte_(address);
	}

	void assert_cartridge_line(bool line_state) override
	{
		assert_cartridge_line_(line_state);
	}

	void assert_interrupt(Interrupt interrupt, InterruptSource interrupt_source) override
	{
		assert_interrupt_(interrupt, interrupt_source);
	}

	void add_menu_item(const char* menu_name, int menu_id, MenuItemType menu_type) override
	{
		add_menu_item_(menu_name, menu_id, menu_type);
	}

private:

	friend void ModuleName(char*, char*, AppendCartridgeMenuModuleCallback);
	friend void MemPointers(ReadMemoryByteModuleCallback, WriteMemoryByteModuleCallback);
	friend void AssertInterupt(AssertInteruptModuleCallback);
	friend void SetCart(AssertCartridgeLineModuleCallback);

	friend class multipak_cartridge;

private:

	const path_type&					configuration_filename_;
	WriteMemoryByteModuleCallback		write_memory_byte_ = [](unsigned char, unsigned short) {};
	ReadMemoryByteModuleCallback		read_memory_byte_ = [](unsigned short) -> unsigned char { return 0; };
	AssertCartridgeLineModuleCallback	assert_cartridge_line_ = [](bool) {};
	AssertInteruptModuleCallback		assert_interrupt_ = [](Interrupt, InterruptSource) {};
	AppendCartridgeMenuModuleCallback	add_menu_item_ = [](const char*, int, MenuItemType) {};
};
