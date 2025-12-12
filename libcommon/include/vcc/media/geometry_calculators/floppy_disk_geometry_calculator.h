#pragma once
#include <vcc/media/geometry_calculator.h>
#include <vcc/media/geometry_calculators/os9_disk_geometry_calculator.h>
#include <vcc/media/geometry_calculators/raw_disk_geometry_calculator.h>
#include <vcc/media/geometry_calculators/jvc_disk_geometry_calculator.h>
#include <vcc/media/geometry_calculators/vdk_disk_geometry_calculator.h>
#include <map>
#include <memory>


namespace vcc::media::geometry_calculators
{

	/// @brief Geometry Calculator that recognizes and supports raw, OS_9, JFC, and VDK
	/// disk images.
	class floppy_disk_geometry_calculator : public ::vcc::media::geometry_calculator
	{
	public:


		/// @inheritdoc
		LIBCOMMON_EXPORT explicit floppy_disk_geometry_calculator(const geometry_type& default_geometry);

		/// @brief Calculates the geometry for a variety of disk image formats.
		/// 
		/// The calculator will examine the disk image header data, file size, and other
		/// attributes of the image file to determine which format it may be stored in
		/// and which calculator should be used to determine the geometry. If a suitable
		/// geometry calculator cannot be determined or if the chosen calculator fails,
		/// the function returns an empty result.
		/// 
		/// @param header_buffer A reference to the first several bytes of the disk image
		/// files header.
		/// @param file_size The size of the disk image file.
		/// 
		/// @return If the geometry can be calculated the results containing the geometry,
		/// file offset to the start image data, and other information; empty value otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT optional_calculated_geometry_type calculate(
			const header_buffer_type& header_buffer,
			size_type file_size) const override;

	private:

		/// @brief The geometry calculator for raw disk image files.
		const ::vcc::media::geometry_calculators::raw_disk_geometry_calculator raw_geometry_calculator_;
		/// @brief The geometry calculator for disk image files containing an OS_9 formatted volume.
		const ::vcc::media::geometry_calculators::os9_disk_geometry_calculator os9_geometry_calculator_;
		/// @brief The geometry calculator for JVC disk image files.
		const ::vcc::media::geometry_calculators::jvc_disk_geometry_calculator jvc_geometry_calculator_;
		/// @brief The geometry calculator for VDK disk image files.
		const ::vcc::media::geometry_calculators::vdk_disk_geometry_calculator vdk_geometry_calculator_;
	};

}
