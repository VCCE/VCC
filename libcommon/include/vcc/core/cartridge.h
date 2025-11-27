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


namespace vcc::core
{

	struct LIBCOMMON_EXPORT cartridge
	{
	public:

		using name_type = ::std::string;
		using catalog_id_type = ::std::string;
		using description_type = ::std::string;
		using size_type = ::std::size_t;


	public:

		cartridge() = default;
		cartridge(const cartridge&) = delete;
		cartridge(cartridge&&) = delete;

		virtual ~cartridge() = default;

		virtual [[nodiscard]] name_type name() const = 0;
		virtual [[nodiscard]] catalog_id_type catalog_id() const = 0;
		virtual [[nodiscard]] description_type description() const = 0;

		virtual void start();
		virtual void stop();
		virtual void reset();

		virtual [[nodiscard]] unsigned char read_memory_byte(size_type memory_address);

		virtual void write_port(unsigned char port_id, unsigned char value);
		virtual [[nodiscard]] unsigned char read_port(unsigned char port_id);

		virtual void process_horizontal_sync();

		virtual [[nodiscard]] unsigned short sample_audio();

		virtual void status(char* text_buffer, size_type buffer_size);
		virtual void menu_item_clicked(unsigned char menu_item_id);
	};

}
