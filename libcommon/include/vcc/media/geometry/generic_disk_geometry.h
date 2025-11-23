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
#include "vcc/media/disk_geometry.h"


namespace vcc::media::geometry
{

	/// @brief Describes the geometry of a disk device or image.
	struct generic_disk_geometry : disk_geometry
	{
		/// @brief The number of sectors per track on the drive or image.
		/// 
		/// Specifies the number of sectors per track on the drive or image. This value is not
		/// absolute and may vary from track to track and from device to device. This member is
		/// initialized with a default value of 18.
		size_type sector_count = 18;
		/// @brief The number of bytes per sector. This value is not absolute and may vary from
		/// sector to sector, track to track, and device to device. This member is initialized
		/// with a default value of 256.
		size_type sector_size = 256;
	};


}
