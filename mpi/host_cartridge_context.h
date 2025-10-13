#pragma once
//#include <vcc/core/cartridge_context.h>
#include <vcc/core/legacy_cartridge_definitions.h>

extern "C" __declspec(dllexport) void ModuleName(char*, char*, AppendCartridgeMenuModuleCallback);
extern "C" __declspec(dllexport) void MemPointers(ReadMemoryByteModuleCallback, WriteMemoryByteModuleCallback);
extern "C" __declspec(dllexport) void AssertInterupt(AssertInteruptModuleCallback);
extern "C" __declspec(dllexport) void SetCart(AssertCartridgeLineModuleCallback);

class host_cartridge_context //: public ::vcc::core::cartridge_context
{
public:

	void write_memory_byte(unsigned char value, unsigned short address) //override
	{
		write_memory_byte_(value, address);
	}

	unsigned char read_memory_byte(unsigned short address) //override
	{
		return read_memory_byte_(address);
	}

	void assert_cartridge_line(bool line_state) //override
	{
		assert_cartridge_line_(line_state);
	}

	void assert_interrupt(Interrupt interrupt, InterruptSource interrupt_source) //override
	{
		assert_interrupt_(interrupt, interrupt_source);
	}

	void add_menu_item(const char* menu_name, int menu_id, MenuItemType menu_type) //override
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

	WriteMemoryByteModuleCallback		write_memory_byte_ = [](unsigned char, unsigned short) {};
	ReadMemoryByteModuleCallback		read_memory_byte_ = [](unsigned short) -> unsigned char { return 0; };
	AssertCartridgeLineModuleCallback	assert_cartridge_line_ = [](bool) {};
	AssertInteruptModuleCallback		assert_interrupt_ = [](Interrupt, InterruptSource) {};
	AppendCartridgeMenuModuleCallback	add_menu_item_ = [](const char*, int, MenuItemType) {};
};
