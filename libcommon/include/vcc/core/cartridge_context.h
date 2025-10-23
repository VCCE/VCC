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
#include <vcc/core/detail/exports.h>
#include <vcc/core/interrupts.h>
#include <string>


// FIXME: this needs to come from the common library but is currently part of the
// main vcc app. Update this when it is migrated.
enum MenuItemType;

namespace vcc { namespace core
{

	struct LIBCOMMON_EXPORT cartridge_context
	{
		using path_type = ::std::string;


		virtual ~cartridge_context() = default;

		virtual path_type configuration_path() const = 0;

		virtual void reset() = 0;

		virtual void assert_cartridge_line(bool line_state) = 0;
		virtual void assert_interrupt(Interrupt interrupt, InterruptSource interrupt_source) = 0;

		virtual void write_memory_byte(unsigned char value, unsigned short address) = 0;
		virtual unsigned char read_memory_byte(unsigned short address) = 0;

		virtual void add_menu_item(const char* menu_name, int menu_id, MenuItemType menu_type) = 0;
	};

} }
