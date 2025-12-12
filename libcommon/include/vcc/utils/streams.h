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
/// @file
/// 
/// @brief A collection of utility functions to supplement existing IO stream
/// functionality.
#include "vcc/detail/exports.h"
#include <iostream>


namespace vcc::utils
{

	/// @brief Retrieve the size of an opened stream.
	/// 
	/// @param stream The stream to determine the size of.
	/// @return If the stream is open and no errors occur, the size in bytes of the
	/// stream; otherwise `pos_type(-1)`.
	[[nodiscard]] LIBCOMMON_EXPORT std::iostream::pos_type get_stream_size(std::iostream& stream);

}
