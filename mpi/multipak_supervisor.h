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
#include "multipak_configuration.h"
#include <vcc/bus/basic_cartridge.h>
#include <vcc/bus/cartridge_loader.h>
#include <vcc/bus/cartridge_menuitem.h>
#include <vcc/util/critical_section.h>

constexpr size_t NUMSLOTS = 4u;

class multipak_supervisor
{
public:

	using callbacks_type = ::VCC::Core::cartridge_callbacks;
	using mount_status_type = ::VCC::Core::cartridge_loader_status;

	// TODO someday replace these stupid type defs with size_t and string
	using slot_id_type = std::size_t;
	using path_type = std::string;
	using label_type = std::string;
	using description_type = std::string;

public:

	multipak_supervisor( multipak_configuration& configuration );

	multipak_supervisor(const multipak_supervisor&) = delete;
	multipak_supervisor(multipak_supervisor&&) = delete;

	multipak_supervisor& operator=(const multipak_supervisor&) = delete;
	multipak_supervisor& operator=(multipak_supervisor&&) = delete;

	void start();
	void stop();
	void reset();
	void menu_item_clicked(unsigned char menu_item_id);
	bool get_menu_item(menu_item_entry* item, size_t index);

	bool empty(slot_id_type slot) const;

	mount_status_type mount_cartridge(slot_id_type slot, const path_type& filename);

	void switch_to_slot(slot_id_type slot);
	slot_id_type selected_switch_slot() const;

private:
	
	static const size_t default_switch_slot_value = 0x03;

	VCC::Util::critical_section mutex_;
	multipak_configuration& configuration_;
	slot_id_type switch_slot_ = default_switch_slot_value;
};
