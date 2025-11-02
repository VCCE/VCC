////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute it and/or
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
#include "fd502_control_register.h"


namespace vcc::cartridges::fd502
{

	namespace
	{
		constexpr auto CTRL_DRIVE0 = 0x01u;
		constexpr auto CTRL_DRIVE1 = 0x02u;
		constexpr auto CTRL_DRIVE2 = 0x04u;
		constexpr auto CTRL_ENABLE_MOTOR = 0x08u;
		constexpr auto CTRL_ENABLE_WRITE_PRECOMP = 0x10u;
		constexpr auto CTRL_DENSITY_SELECT = 0x20u;
		constexpr auto CTRL_SIDE_SELECT = 0x40u;
		constexpr auto CTRL_ENABLE_HALT = 0x80u;
	}

	fd502_control_register& fd502_control_register::operator=(register_value_type register_value) noexcept
	{
		head_ = 0;
		drive_.reset();
		motor_enabled_ = false;
		interupt_enabled_ = false;
		halt_enabled_ = false;

		// The FD502 uses the density bit to control whether interrupts raised by the WD197x
		// are passed to the CPU. 
		if (register_value & CTRL_DENSITY_SELECT)
		{
			interupt_enabled_ = true;
		}

		if (register_value & CTRL_ENABLE_HALT)
		{
			halt_enabled_ = true;
		}

		if (register_value & CTRL_ENABLE_MOTOR)
		{
			motor_enabled_ = true;
		}

		if (register_value & CTRL_SIDE_SELECT)
		{
			head_ = 1;
		}

		if (register_value & CTRL_DRIVE0)
		{
			drive_ = 0;
		}

		if (register_value & CTRL_DRIVE1)
		{
			drive_ = 1;
		}

		if (register_value & CTRL_DRIVE2)
		{
			drive_ = 2;
		}

		// The FD502 can select one of three disk drives
		// DRIVE3 Select in "all single sided" systems
		if (head_ && !drive_.has_value())
		{
			drive_ = 3;
			head_ = 0;
		}

		return *this;
	}

}
