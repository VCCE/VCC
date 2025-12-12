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
#include "vcc/bus/cartridge.h"


namespace vcc::bus
{

	void cartridge::start()
	{}

	void cartridge::stop()
	{}

	cartridge::status_type cartridge::status() const
	{
		return {};
	}

	void cartridge::menu_item_clicked([[maybe_unused]] menu_item_id_type menu_item_id)
	{}

	cartridge::menu_item_collection_type cartridge::get_menu_items() const
	{
		return {};
	}

}
