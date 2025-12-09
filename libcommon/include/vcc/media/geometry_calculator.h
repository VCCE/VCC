#pragma once
#include <vcc/media/geometry/generic_disk_geometry.h>
#include <array>
#include <optional>


namespace vcc::media
{

	/// @brief The results of determining the geometry of a disk image.
	struct calculated_geometry
	{
		/// @brief Type alias to lengths, 1 dimension sizes, and indexes.
		using size_type = std::size_t;
		/// @brief Type alias for geometry.
		using geometry_type = ::vcc::media::geometry::generic_disk_geometry;

		/// @brief The 0 based offset into the disk image file where the image data start.
		size_type image_file_data_offset;
		/// @brief The geometry of the disk image.
		geometry_type geometry;
	};


	/// @brief BAse class for Geometry Calculators.
	///
	/// This abstract class defines the interface and base behavior of Geometry
	/// Calculators used to determine the layout details of a disk image file.
	class geometry_calculator
	{
	public:

		/// @brief Type alias to lengths, 1 dimension sizes, and indexes.
		using size_type = std::size_t;
		/// @brief Type alias for the result of geometry calculations.
		using calculated_geometry_type = calculated_geometry;
		/// @brief Type alias for an optional result of geometry calculations.
		using optional_calculated_geometry_type = std::optional<calculated_geometry_type>;
		/// @brief Type alias for geometry.
		using geometry_type = calculated_geometry_type::geometry_type;
		/// @brief Type alias for the buffer that holds the disk image header data.
		using header_buffer_type = std::array<unsigned char, 32>;


	public:

		/// @brief Construct the Geometry Calculator.
		/// 
		/// @param default_geometry The default geometry. The values in the default
		/// geometry are used for the base of the calculations and may be used in whole
		/// or in part when no geometry information is available.
		explicit geometry_calculator(const geometry_type& default_geometry)
			: default_geometry_(default_geometry)
		{ }

		virtual ~geometry_calculator() = default;


		/// @brief Calculate the geometry of a disk image.
		/// 
		/// Calculate the geometry of a disk image based on the size of the image and
		/// the first several bytes of the file (header). The calculator uses the size of
		/// the disk image file and optionally the information in any header data to
		/// determine the geometry of the disk image. If the information available is
		/// insufficient an empty value is returned.
		/// 
		/// @param header_buffer The buffer containing the header data from the disk image file.
		/// @param file_size The number of bytes in the disk image file.
		/// 
		/// @return If the geometry can be calculated the results containing the geometry,
		/// file offset to the start image data, and other information; empty value otherwise.
		virtual optional_calculated_geometry_type calculate(
			const header_buffer_type& header_buffer,
			size_type file_size) const = 0;


	protected:

		/// @brief Retrieve the default geometry.
		/// 
		/// @return The default geometry.
		const geometry_type& default_geometry() const noexcept
		{
			return default_geometry_;
		}


	private:

		/// @brief The default geometry.
		const geometry_type default_geometry_;
	};

}
