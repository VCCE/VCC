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
/// @file
/// 
/// @brief Defines disk image format identifiers.


namespace vcc::cartridges::fd502::detail
{

	/// @brief Defines unique identifiers for the disk images formats.
	enum class disk_image_format_id
	{
		/// @brief Raw format arranged by track and sector ordered sequentially.
		JVC,
		/// @brief ?Virtual Disk?
		VDK,
		/// @brief Raw format arranged by track and sector ordered sequentially formatted
		/// for OS-9.
		OS9,
		/// @brief Rich disk format that includes sector records and other detailed data.
		DMK
	};

}
