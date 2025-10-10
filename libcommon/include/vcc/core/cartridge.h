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
#include <string>


namespace vcc { namespace core
{

	struct LIBCOMMON_EXPORT cartridge
	{
	public:

		using name_type = std::string;
		using catalog_id_type = std::string;


	public:

		cartridge() = default;
		cartridge(const cartridge&) = delete;
		cartridge(cartridge&&) = delete;

		virtual ~cartridge() = default;

		virtual void start();
		virtual void reset();
		virtual void heartbeat();
		virtual void write_port(unsigned char portId, unsigned char value);
		virtual unsigned char read_port(unsigned char portId);
		virtual unsigned char read_memory_byte(unsigned short memoryAddress);
		virtual void status(char* status);
		virtual unsigned short sample_audio();
		virtual void menu_item_clicked(unsigned char menuItemId);


	protected:

		virtual void initialize_pak();
		virtual void initialize_bus();
	};

} }
