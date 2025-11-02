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
#pragma once
#include "fdc_command_id.h"
#include <array>
#include <optional>
#include <chrono>


namespace vcc::cartridges::fd502::detail
{

	// TODO-CHET: place in ::vcc::devices::storage::wd1793::detail?? or is this 
	// something the derived classes need (probably not)?

	/// @brief Represents a FDC command and its attributes.
	class fdc_command
	{
		/// @brief Type alias for command identifiers
		using command_id_type = ::vcc::cartridges::fd502::detail::fdc_command_id;
		/// @brief Type alias for flags container.
		using flags_type = unsigned char;
		/// @brief Type alias for track step time values.
		///
		/// @todo maybe convert this to something in std::chrono?
		using step_time_type = std::size_t;


	public:

		/// @brief Construct the Command.
		/// 
		/// @param id Command identifier.
		/// @param flags Command flags.
		fdc_command(command_id_type id, flags_type flags);

		/// @brief Retrieves the command identifier.
		/// @return The command identifier.
		command_id_type id() const;

		/// @brief Retrieve the step time in milliseconds.
		/// 
		/// @return The track step time in milliseconds.
		step_time_type step_time() const;


	private:

		/// @brief The command identifier.
		command_id_type id_;
		/// @brief The (optional) track step time in milliseconds.
		std::optional<step_time_type> step_time_;
	};

}
