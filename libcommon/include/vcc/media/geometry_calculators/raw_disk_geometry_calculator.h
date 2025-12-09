#pragma once
#include <vcc/media/geometry_calculator.h>


namespace vcc::media::geometry_calculators
{

	/// @brief Implementation of the Geometry Calculator used for determining the
	/// geometry of a raw disk image with no special formatting.
	class raw_disk_geometry_calculator : public ::vcc::media::geometry_calculator
	{
	public:

		/// @brief Specifies the number of header bytes in a RAW disk image.
		static constexpr auto header_size = 0u;


	public:

		using geometry_calculator::geometry_calculator;

		/// @brief Calculates the geometry of a disk image based on its size.
		/// 
		/// Calculate the geometry of a disk image based on its size. The calculator uses
		/// the size of the disk image file and the parameters of the default geometry.
		/// When calculating the geometry several properties of the file size are taken
		/// into account. First, the disk image size must be a multiple of the track size
		/// (sector size * sector count). Second, there must be enough data in the file
		/// to support at least 35 tracks (this is not compatible with some disk images
		/// that attempt to save space). Third, when determining if the disk is single or
		/// double sided the calculator will consider the total number of tracks - if the
		/// disk image has less than 70 total tracks or has odd number of tracks the
		/// disk image is marked as single sided. If the disk image has 70 or more total
		/// tracks and an even number of tracks the disk image is marked as double sided.
		/// 
		/// @param header_buffer The buffer containing the header data from the disk image file.
		/// @param file_size The number of bytes in the disk image file.
		/// 
		/// @return If the geometry can be calculated the results containing the geometry,
		/// file offset to the start image data, and other information; empty value otherwise.
		optional_calculated_geometry_type calculate(
			const header_buffer_type& header_buffer,
			size_type file_size) const override;
	};

}
