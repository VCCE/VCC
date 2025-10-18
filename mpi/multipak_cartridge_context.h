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
#include "multipak_cartridge.h"
#include <vcc/core/cartridge_context.h>


class multipak_cartridge_context : public ::vcc::core::cartridge_context
{
public:

	multipak_cartridge_context(size_t slot_id, ::vcc::core::cartridge_context& parent_context, multipak_cartridge& multipak)
		:
		slot_id_(slot_id),
		parent_context_(parent_context),
		multipak_(multipak)
	{}

	path_type configuration_path() const override
	{
		return parent_context_.configuration_path();
	}

	void reset() override
	{
		parent_context_.reset();
	}

	void add_menu_item(const char* menu_name, int menu_id, MenuItemType menu_type) override
	{
		multipak_.append_menu_item(slot_id_, { menu_name, static_cast<unsigned int>(menu_id), menu_type });
	}

	void write_memory_byte(unsigned char value, unsigned short address) override
	{
		parent_context_.write_memory_byte(value, address);
	}

	unsigned char read_memory_byte(unsigned short address) override
	{
		return parent_context_.read_memory_byte(address);
	}

	void assert_cartridge_line(bool line_state) override
	{
		multipak_.assert_cartridge_line(slot_id_, line_state);
	}

	void assert_interrupt(Interrupt interrupt, InterruptSource interrupt_source) override
	{
		parent_context_.assert_interrupt(interrupt, interrupt_source);
	}


private:

	const size_t slot_id_;
	::vcc::core::cartridge_context& parent_context_;
	multipak_cartridge& multipak_;
};
