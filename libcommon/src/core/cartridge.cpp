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
#include <vcc/core/cartridge.h>


namespace vcc { namespace core
{

	const cartridge::name_type& cartridge::name() const
	{
		static const name_type local_name;

		return local_name;
	}
	
	const cartridge::catalog_id_type& cartridge::catalog_id() const
	{
		static const catalog_id_type local_id;

		return local_id;
	}


	void cartridge::start()
	{
		initialize_pak();
		initialize_bus();
	}

	void cartridge::reset()
	{}

	void cartridge::heartbeat()
	{}

	void cartridge::write_port(unsigned char portId, unsigned char value)
	{}

	unsigned char cartridge::read_port(unsigned char portId)
	{ 
		return {};
	}

	unsigned char cartridge::read_memory_byte(unsigned short memoryAddress)
	{
		return {};
	}

	void cartridge::status(char* status_text)
	{
		*status_text = 0;
	}

	unsigned short cartridge::sample_audio()
	{
		return {};
	}

	void cartridge::menu_item_clicked(unsigned char menuItemId)
	{}

	void cartridge::initialize_pak()
	{}

	void cartridge::initialize_bus()
	{}

} }
