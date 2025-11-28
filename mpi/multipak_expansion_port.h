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
#include "vcc/bus/expansion_port_ui.h"
#include "vcc/bus/expansion_port_bus.h"
#include "vcc/bus/expansion_port.h"
#include "vcc/utils/cartridge_loader.h"


namespace vcc::cartridges::multipak
{

	/// @brief Extends `expansion_port` to add support for the cartridge line select of
	/// cartridges inserted into the Multi-Pak.
	class multipak_expansion_port : public ::vcc::bus::expansion_port
	{
	public:

		using expansion_port::expansion_port;

		/// @brief Sets the cartridge port line state.
		/// 
		/// @param state The new state.
		void cartridge_select_line(bool state)
		{
			line_state_ = state;
		}

		/// @brief Retrieves the cartridge port line state.
		/// 
		/// @return The cartridge port line state.
		[[nodiscard]] bool cartridge_select_line() const
		{
			return line_state_;
		}


	private:

		/// @brief The line state.
		bool line_state_ = false;
	};

}
