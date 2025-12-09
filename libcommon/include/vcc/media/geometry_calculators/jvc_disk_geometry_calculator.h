#pragma once
#include <vcc/media/geometry_calculator.h>


namespace vcc::media::geometry_calculators
{

	/// @brief Implementation of the Geometry Calculator used for determining the
	/// geometry of a disk image in JVC format.
	class jvc_disk_geometry_calculator : public ::vcc::media::geometry_calculator
	{
	public:

		/// @brief Specifies the number of header bytes in a JVC disk image.
		static constexpr auto header_size = 2u;


	public:

		using geometry_calculator::geometry_calculator;

		optional_calculated_geometry_type calculate(
			const header_buffer_type& header_buffer,
			size_type file_size) const override;


	private:

		/// @brief Defines the offsets into the header data of different disk image
		/// properties
		struct header_elements
		{
			/// @brief The offset into the header data of the number of sectors per track.
			static constexpr auto sector_count = 0u;
			/// @brief The offset into the header data of the number of heads.
			static constexpr auto head_count = 1u;
		};
	};

}
