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
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_port.h"
#include "vcc/bus/expansion_port_ui.h"
#include "vcc/bus/expansion_port_bus.h"
#include "vcc/utils/cartridge_loader.h"


namespace vcc::modules::mpi
{

	class cartridge_slot : public ::vcc::bus::expansion_port
	{
	public:

		using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
		using expansion_port_ui_type = ::vcc::bus::expansion_port_ui;
		using cartridge_type = ::vcc::bus::cartridge;
		using cartridge_ptr_type = std::unique_ptr<cartridge_type>;
		using name_type = cartridge_type::name_type;
		using catalog_id_type = cartridge_type::catalog_id_type;
		using description_type = cartridge_type::description_type;
		using menu_item_collection_type = cartridge_type::menu_item_collection_type;
		using path_type = std::string;
		using label_type = std::string;
		using handle_type = ::vcc::utils::cartridge_loader_result::handle_type;
		using size_type = std::size_t;


	public:

		using expansion_port::expansion_port;
		cartridge_slot(path_type path, handle_type handle, cartridge_ptr_type cartridge);

		cartridge_slot& operator=(const cartridge_slot& other) = delete;
		cartridge_slot& operator=(cartridge_slot&& other) noexcept;

		label_type label() const;

		void line_state(bool state)
		{
			line_state_ = state;
		}

		bool line_state() const
		{
			return line_state_;
		}


	private:

		path_type path_;
		bool line_state_ = false;
	};

}
