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
#include "vcc/detail/exports.h"
#include <string>


namespace vcc::bus
{

	/// @brief Interface to the system managing the expansion port.
	///
	/// The Expansion Port Host abstract class defines the services and properties provided
	/// by the system managing the expansion port and exposed to external devices.
	class LIBCOMMON_EXPORT expansion_port_host
	{
	public:

		/// @brief The type used to represent paths.
		using path_type = std::string;


	public:

		virtual ~expansion_port_host() = default;

		/// @brief Retrieve the path to the configuration file.
		/// 
		/// @return A copy of the path to the configuration file.
		virtual [[nodiscard]] path_type configuration_path() const = 0;

		/// @brief Retrieves the path to where system ROMS are stored.
		/// 
		/// This function returns a path to where ROMS used by the system and by cartridges are
		/// stored.
		/// 
		/// @return A path to where system ROMS are stored.
		virtual [[nodiscard]] path_type system_rom_path() const = 0;
	};

}
