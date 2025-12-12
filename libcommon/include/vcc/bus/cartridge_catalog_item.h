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
#include <vcc/detail/exports.h>
#include <vcc/utils/basic_guid.h>
#include <string>
#include <array>


namespace vcc::bus
{

	/// @brief Defines the same of a cartridge catalog item
	struct cartridge_catalog_item
	{
		/// @brief Type alias for variable length strings.
		using string_type = std::string;
		/// @brief Type alias for file paths.
		using path_type = std::string;
		/// @brief Type alias for a globally unique identifier.
		using guid_type = ::vcc::utils::basic_guid;

		/// @brief The unique identifier of the cartridge.
		guid_type	id;
		/// @brief The human friendly name of the cartridge.
		string_type name;
		/// @brief The filename, without the path, of the cartridge.
		string_type filename;
	};

}
