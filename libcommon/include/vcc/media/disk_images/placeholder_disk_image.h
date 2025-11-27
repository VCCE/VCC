////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute itand/or
//	modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your
//	option) any later version.
//	
//	VCC (Virtual Color Computer) is distributed in the hope that it will be
//	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
//	Public License for more details.
//	
//	You should have received a copy of the GNU General Public License along with
//	VCC (Virtual Color Computer). If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "vcc/media/disk_image.h"
#include "vcc/media/geometry/generic_disk_geometry.h"
#include <Windows.h>
#include <iostream>
#include <memory>


namespace vcc::media::disk_images
{

	/// @brief Disk Image implementation used as a placeholder when a disk image is
	/// needed but not available.
	/// 
	/// This implementation of the Disk Image interface provides support for use-cases
	/// where a disk image is needed but one is not available such as an empty floppy disk
	/// drive. It provides the same property and validation functionality as other disk
	/// image types allowing for checking operation parameters used to access the disk.
	/// 
	/// Some behavior such as reading and writing data always throw an IO exception since
	/// those operations are not possible. Adequate checking of operational parameters must
	/// be performed prior to invoking read and write functions.
	class placeholder_disk_image : public disk_image
	{
	public:

		/// @brief The type used to hold geometry describing the disk capacity.
		using geometry_type = ::vcc::media::geometry::generic_disk_geometry;

	public:

		/// @brief Constructs a Placeholder Disk Image.
		/// 
		/// @param geometry The geometry of the disk.
		/// @param first_valid_sector_id The first identifier that can be used for a sector.
		/// @param write_protected Specifies if the disk is write protected or not.
		LIBCOMMON_EXPORT explicit placeholder_disk_image(
			const geometry_type& geometry = {},
			size_type first_valid_sector_id = 1,
			bool write_protected = false);


		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT bool is_valid_sector_record(
			size_type disk_head,
			size_type disk_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id) const noexcept override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT size_type get_sector_size(
			size_type disk_head,
			size_type disk_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id) const override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT std::optional<sector_record_header_type> query_sector_header_by_index(
			size_type disk_head,
			size_type disk_track,
			size_type disk_sector) const override;

		/// @inheritdoc
		LIBCOMMON_EXPORT void read_sector(
			size_type disk_head,
			size_type disk_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id,
			buffer_type& data_buffer) override;

		/// @inheritdoc
		LIBCOMMON_EXPORT void write_sector(
			size_type disk_head,
			size_type disk_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id,
			const buffer_type& data_buffer) override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT sector_record_vector read_track(
			size_type disk_head,
			size_type disk_track) override;

		/// @inheritdoc
		LIBCOMMON_EXPORT void write_track(
			size_type disk_head,
			size_type disk_track,
			const sector_record_vector& sectors) override;


	protected:

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT virtual size_type get_sector_count_unchecked(
			size_type disk_head,
			size_type disk_track) const noexcept override;


	private:

		/// @brief The geometry of the fake disk.
		const geometry_type geometry_;
	};

}
