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
