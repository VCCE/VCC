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
#include "vcc/media/disk_image.h"
#include "vcc/media/exceptions.h"


namespace vcc::media
{

	disk_image::disk_image(
		const geometry_type& geometry,
		size_type first_valid_sector_id,
		bool write_protected)
		:
		head_count_(geometry.head_count),
		track_count_(geometry.track_count),
		first_valid_sector_id_(first_valid_sector_id),
		write_protected_(write_protected)
	{
		if (!head_count_)
		{
			throw std::invalid_argument("Cannot construct Disk Image. Number of heads is 0.");
		}

		if (!track_count_)
		{
			throw std::invalid_argument("Cannot construct Disk Image. Number of tracks is 0.");
		}
	}


	bool disk_image::is_write_protected() const noexcept
	{
		return write_protected_;
	}

	disk_image::size_type disk_image::head_count() const noexcept
	{
		return head_count_;
	}

	disk_image::size_type disk_image::track_count() const noexcept
	{
		return track_count_;
	}


	disk_image::size_type disk_image::get_sector_count(
		size_type disk_head,
		size_type disk_track) const
	{
		if (!is_valid_disk_head(disk_head))
		{
			throw std::invalid_argument("Cannot retrieve number of sectors. Specified drive head is invalid.");
		}

		if (!is_valid_disk_track(disk_track))
		{
			throw std::invalid_argument("Cannot retrieve number of sectors. Specified drive track is invalid.");
		}

		return get_sector_count_unchecked(disk_head, disk_track);
	}

	bool disk_image::is_valid_track_sector(
		size_type disk_head,
		size_type disk_track,
		size_type disk_sector) const noexcept
	{
		return is_valid_disk_head(disk_head)
			&& is_valid_disk_track(disk_track)
			&& disk_sector < get_sector_count_unchecked(disk_head, disk_track);
	}


	bool disk_image::is_valid_disk_head(size_type disk_head) const noexcept
	{
		return disk_head < head_count();
	}

	bool disk_image::is_valid_disk_track([[maybe_unused]] size_type disk_track) const noexcept
	{
		return disk_track < track_count();
	}


	disk_image::size_type disk_image::first_valid_sector_id() const noexcept
	{
		return first_valid_sector_id_;
	}

}
