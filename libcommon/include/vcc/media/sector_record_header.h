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
#include <cstddef>


namespace vcc::media
{

	/// @brief Represents the properties of a disk sector
	struct sector_record_header
	{
		/// @brief Type alias to lengths, 1 dimension sizes, and indexes.
		using size_type = std::size_t;

		/// @brief The identifier of the drive stored in the sector header. This value may be
		/// different than the physical head used to read the header. This member is
		/// initialized to a default value of 0 (zero).
		const size_type head_id = {};
		/// @brief The identifier of the track stored in the sector header. This value may be
		/// different than the physical track number the sector was stored in. This member is
		/// initialized to a default value of 0 (zero).
		const size_type track_id = {};
		/// @brief The identifier of the sector the header represents. This member is
		/// initialized to a default value of 0 (zero).
		const size_type sector_id = {};
		/// @brief The number of bytes in the sector. This member is initialized to a default
		/// value of 0 (zero).
		const size_type sector_length = {};
	};

}
