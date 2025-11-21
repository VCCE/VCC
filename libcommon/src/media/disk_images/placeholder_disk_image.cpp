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
#include "vcc/media/disk_images/placeholder_disk_image.h"
#include "vcc/media/exceptions.h"
#include "vcc/utils/streams.h"
#include <fstream>


namespace vcc::media::disk_images
{

	placeholder_disk_image::placeholder_disk_image(
		const geometry_type& geometry,
		size_type first_valid_sector_id,
		bool write_protected)
		:
		disk_image(geometry, first_valid_sector_id, write_protected),
		geometry_(geometry)
	{}


	bool placeholder_disk_image::is_valid_sector_record(
		size_type disk_head,
		size_type disk_track,
		size_type head_id,
		size_type track_id,
		size_type sector_id) const noexcept
	{
		if (disk_head != head_id || disk_track != track_id)
		{
			return false;
		}

		if (sector_id < first_valid_sector_id())
		{
			return false;
		}

		// TODO: add get_sector_count_unchecked() since this function shouldn't throw and
		// sector_count does.
		return sector_id - first_valid_sector_id() < get_sector_count(disk_head, disk_track);
	}


	placeholder_disk_image::size_type placeholder_disk_image::get_sector_size(
		size_type disk_head,
		size_type disk_track,
		size_type head_id,
		size_type track_id,
		size_type sector_id) const
	{
		if (!is_valid_disk_head(disk_head))
		{
			throw std::invalid_argument("Cannot retrieve the size of a sector. Specified drive head is invalid.");
		}

		if (!is_valid_disk_track(disk_track))
		{
			throw std::invalid_argument("Cannot retrieve the size of a sector. Specified drive track is invalid.");
		}

		if (!is_valid_sector_record(disk_head, disk_track, head_id, track_id, sector_id))
		{
			throw std::invalid_argument("Cannot retrieve the size of a sector. Specified sector id is invalid.");
		}

		return geometry_.sector_size;
	}

	std::optional<placeholder_disk_image::sector_record_header_type> placeholder_disk_image::query_sector_header_by_index(
		size_type disk_head,
		size_type disk_track,
		size_type disk_sector) const
	{
		if (!is_valid_disk_head(disk_head))
		{
			throw std::invalid_argument("Cannot retrieve the record header by index. Specified drive head is invalid.");
		}

		if (!is_valid_disk_track(disk_track))
		{
			throw std::invalid_argument("Cannot retrieve the record header by index. Specified drive track is invalid.");
		}

		if (disk_sector >= get_sector_count(disk_head, disk_track) + first_valid_sector_id())
		{
			throw std::invalid_argument("Cannot retrieve the record header by index. Specified drive sector is invalid.");
		}

		return sector_record_header_type{
			disk_head,
			disk_track,
			disk_sector,
			geometry_.sector_size
		};
	}

	void placeholder_disk_image::read_sector(
		[[maybe_unused]] size_type disk_head,
		[[maybe_unused]] size_type disk_track,
		[[maybe_unused]] size_type head_id,
		[[maybe_unused]] size_type track_id,
		[[maybe_unused]] size_type sector_id,
		[[maybe_unused]] buffer_type& data_buffer)
	{
		throw fatal_io_error("Cannot read sector. Disk does not exist.");
	}

	void placeholder_disk_image::write_sector(
		[[maybe_unused]] size_type disk_head,
		[[maybe_unused]] size_type disk_track,
		[[maybe_unused]] size_type head_id,
		[[maybe_unused]] size_type track_id,
		[[maybe_unused]] size_type sector_id,
		[[maybe_unused]] const buffer_type& data_buffer)
	{
		throw fatal_io_error("Cannot write sector. Disk does not exist.");
	}

	placeholder_disk_image::sector_record_vector placeholder_disk_image::read_track(
		[[maybe_unused]] size_type disk_head,
		[[maybe_unused]] size_type disk_track)
	{
		throw fatal_io_error("Cannot read track. Disk does not exist.");
	}

	void placeholder_disk_image::write_track(
		[[maybe_unused]] size_type disk_head,
		[[maybe_unused]] size_type disk_track,
		[[maybe_unused]] const sector_record_vector& sectors)
	{
		throw fatal_io_error("Cannot write track. Disk does not exist.");
	}

	placeholder_disk_image::size_type placeholder_disk_image::get_sector_count_unchecked(
		[[maybe_unused]] size_type disk_head,
		[[maybe_unused]] size_type disk_track) const noexcept
	{
		return geometry_.sector_count;
	}

}