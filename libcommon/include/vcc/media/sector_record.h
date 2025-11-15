#pragma once
#include "vcc/media/sector_record_header.h"
#include <vector>


namespace vcc::media
{

	/// @brief Represents the contents of a sector stored in a disk image.
	struct sector_record
	{
		/// @brief The type of buffer used to store sector data.
		using buffer_type = std::vector<unsigned char>;

		/// @brief The core properties of the rector.
		sector_record_header header;
		/// @brief The sector data.
		buffer_type data;
	};

}
