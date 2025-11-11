#pragma once
#include <cstddef>


namespace vcc::media
{

	/// @brief Represents the properties of a disk sector
	struct sector_record_header
	{
		/// @brief Specifies the type used to store header properties.
		using size_type = ::std::size_t;

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
