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


class multipak_expansion_port : public ::vcc::bus::expansion_port
{
public:

	using label_type = std::string;


public:

	using expansion_port::expansion_port;

	multipak_expansion_port(const multipak_expansion_port&) = delete;
	multipak_expansion_port(multipak_expansion_port&&) = default;

	multipak_expansion_port& operator=(const multipak_expansion_port& other) = delete;
	multipak_expansion_port& operator=(multipak_expansion_port&& other) noexcept;

	void line_state(bool state)
	{
		line_state_ = state;
	}

	bool line_state() const
	{
		return line_state_;
	}

	label_type label() const;


private:

	bool line_state_ = false;
};
