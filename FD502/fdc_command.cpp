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
#include "fdc_command.h"


namespace vcc::cartridges::fd502::detail
{

	fdc_command::fdc_command(command_id_type id, flags_type flags)
		: id_(id)
	{
		static const std::array<step_time_type, 4> track_step_time_table_ =
		{
			step_time_type(6),
			step_time_type(12),
			step_time_type(20),
			step_time_type(30)
		};

		switch (id_)
		{
		case command_id_type::restore:
		case command_id_type::seek:
		case command_id_type::step:
		case command_id_type::step_upd:
		case command_id_type::step_in:
		case command_id_type::step_in_upd:
		case command_id_type::step_out:
		case command_id_type::step_out_upd:
			//HeadLoad = (flags >> 3) & 1;
			//TrackVerify = (flags >> 2) & 1;
			step_time_ = track_step_time_table_[flags & 3];
			break;

		case command_id_type::read_sector:
		case command_id_type::read_sector_m:
		case command_id_type::write_sector:
		case command_id_type::write_sector_m:
			//SideCompare = (flags >> 3) & 1;
			//SideCompareEnable = (flags >> 1) & 1;
			//Delay15ms = (flags >> 2) & 1;
			break;

		case command_id_type::read_address:
		case command_id_type::read_track:
		case command_id_type::write_track:
			//Delay15ms = (flags >> 2) & 1;
			break;

		case command_id_type::force_interupt:
			break;
		}
	}

	fdc_command::command_id_type fdc_command::id() const
	{
		return id_;
	}

	fdc_command::step_time_type fdc_command::step_time() const
	{
		// TODO-CHET: need to provide a way to check if the step time has been set
		return step_time_.value();
	}

}
