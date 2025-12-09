#pragma once
#include <vcc/media/geometry_calculator.h>


namespace vcc::media::geometry_calculators
{

	/// @brief Implementation of the Geometry Calculator used for determining the
	/// geometry of a disk image in VDK format.
	class vdk_disk_geometry_calculator : public ::vcc::media::geometry_calculator
	{
	public:

		/// @brief Specifies the number of header bytes in a VDK disk image.
		static constexpr auto header_size = 12u;


	public:

		using geometry_calculator::geometry_calculator;

		/// @brief This calculator is not implemented and always returns an empty result.
		/// 
		/// @todo implement support for VDK disks.
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
			/// @brief The offset into the header data of the number of heads.
			static constexpr auto head_count = 9u;
		};
	};

}
