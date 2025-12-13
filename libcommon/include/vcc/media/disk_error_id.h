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
/// Contains definitions for disk errors.
#include <cstddef>


namespace vcc::media
{

	/// @brief Defines a set of status codes returned from disk drive and disk image
	/// related functions.
	enum class disk_error_id
	{
		/// @brief The function was successful.
		success,
		/// @brief The disk drive or image is empty.
		empty,
		/// @brief The specified head parameter is not valid for the operation or does
		/// not exist.
		invalid_head,
		/// @brief The specified track parameter is not valid for the operation or does
		/// not exist.
		invalid_track,
		/// @brief The specified sector parameter is not valid for the operation or does
		/// not exist.
		invalid_sector,
		/// @brief The operation failed because the disk is write protected.
		write_protected
	};

}
