#pragma once
#include <vcc/media/geometry_calculator.h>


namespace vcc::media::geometry_calculators
{

	/// @brief Implementation of the Geometry Calculator used for determining the
	/// geometry of a disk image containing an OS-9 formatted volume.
	class os9_disk_geometry_calculator : public ::vcc::media::geometry_calculator
	{
	public:

		using geometry_calculator::geometry_calculator;

		/// @brief Calculates the geometry of a disk image containing an OS_9 formatted
		/// volume.
		/// 
		/// Calculate the geometry based on the parameters in the volume header, the size
		/// of the disk image file, and the parameters of the default geometry. This
		/// calculator uses the total number of sectors, the number of heads, and the
		/// number of sectors per track to verify that the disk image is large enough to
		/// contain the volume and the file size is a multiple of the total size of a disk
		/// track.
		/// 
		/// @param header_buffer The buffer containing the header data from the disk image file.
		/// @param file_size The number of bytes in the disk image file.
		/// 
		/// @return If the geometry can be calculated the results containing the geometry,
		/// file offset to the start image data, and other information; empty value otherwise.
		optional_calculated_geometry_type calculate(
			const header_buffer_type& header_buffer,
			size_type file_size) const override;


	private:

		/// @brief Defines the offsets into the header data of different disk image
		/// properties
		struct header_elements
		{
			/// @brief The offset into the header data of the number of the most
			/// significant byte of the total number of sectors in the disk image.
			static constexpr auto sector_count_msb = 0u;
			/// @brief The offset into the header data of the number of the middle byte
			/// (bits 8 thrus 15) of the total number of sectors in the disk image.
			static constexpr auto sector_count_csb = 1u;
			/// @brief The offset into the header data of the number of the least
			/// significant byte of the total number of sectors in the disk image.
			static constexpr auto sector_count_lsb = 2u;
			/// @brief The offset into the header data of the number of sectors per track.
			static constexpr auto sector_per_track = 3u;
			/// @brief The offset into the header data of various attribute flags describing
			/// additional information about the disk image (i.e. number of heads).
			static constexpr auto format_flags = 16u;
		};
	};

}
