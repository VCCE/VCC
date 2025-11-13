#pragma once
#include <cstddef>


namespace vcc::media
{

	/// @brief Describes the geometry of a disk device or image.
	struct disk_geometry
	{
		/// @brief Specifies the type used to store geometry properties.
		using size_type = ::std::size_t;

		/// @brief The number of heads on the drive or image. This member is initialized with a
		/// value of 1.
		size_type head_count = 1;
		/// @brief The number of tracks on the drive or image. This member is initialized with
		/// a default value of 35.
		size_type track_count = 35;
	};

}
