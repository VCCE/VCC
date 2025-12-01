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
#include <vcc/core/cartridges/basic_cartridge.h>


namespace vcc::core::cartridges
{

	basic_cartridge::name_type basic_cartridge::name() const
	{
		return {};
	}
	
	basic_cartridge::catalog_id_type basic_cartridge::catalog_id() const
	{
		return {};
	}

	basic_cartridge::description_type basic_cartridge:: description() const
	{
		return {};
	}


	void basic_cartridge::start()
	{
		initialize_pak();
		initialize_bus();
	}

	void basic_cartridge::stop()
	{}

	void basic_cartridge::reset()
	{}

	void basic_cartridge::process_horizontal_sync()
	{}

	void basic_cartridge::write_port(unsigned char port_id, unsigned char value)
	{}

	unsigned char basic_cartridge::read_port(unsigned char port_id)
	{ 
		return {};
	}

	unsigned char basic_cartridge::read_memory_byte(unsigned short memory_address)
	{
		return {};
	}

	void basic_cartridge::status(char* text_buffer, size_t buffer_size)
	{
		*text_buffer = 0;
	}

	unsigned short basic_cartridge::sample_audio()
	{
		return {};
	}

	void basic_cartridge::menu_item_clicked(unsigned char menu_item_id)
	{}

	void basic_cartridge::initialize_pak()
	{}

	void basic_cartridge::initialize_bus()
	{}

}
