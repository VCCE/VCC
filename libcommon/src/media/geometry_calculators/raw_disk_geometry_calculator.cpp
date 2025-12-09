////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute it and/or
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
#include "vcc/media/geometry_calculators/raw_disk_geometry_calculator.h"
#include <limits>


namespace vcc::media::geometry_calculators
{

	raw_disk_geometry_calculator::optional_calculated_geometry_type raw_disk_geometry_calculator::calculate(
		const header_buffer_type& header_buffer,
		size_type file_size) const
	{
		auto geometry(default_geometry());

		// Calculate the size of each track and make sure that there are no extra bytes
		// since a raw disk image should have no header and each track is the same size.
		const auto track_size_in_bytes(geometry.sector_count * geometry.sector_size);
		if (file_size % track_size_in_bytes != 0)
		{
			return {};
		}

		// Calculate the total number of tracks and make sure that we have enough to
		// represent the minimum known/expected disk size.
		const auto total_track_count(file_size / track_size_in_bytes);
		if (total_track_count < 35)
		{
			return {};
		}

		// The minimum number of tracks per side is 35 so we assume that the minimum
		// number of tracks needed to support a standard double sided disk is twice that.
		// We also assume that double sided disk always have an even number of tracks and
		// if not we assume single sided. Since some floppy drives and disks can format
		// up to 41 or 42 tracks even though they are only support so support 40 we
		// accept track counts for single sided disks
		if(total_track_count < 70 || (total_track_count & 1) != 0)
		{
			geometry.track_count = total_track_count;
			geometry.head_count = 1;
		}
		else
		{
			geometry.track_count = total_track_count / 2;
			geometry.head_count = 2;
		}

		// Most older FDC's only use 8 bits to represent the track number when accessing
		// the disk so we limit the maximum number of tracks to fit in 8 bits.
		if (geometry.track_count > std::numeric_limits<std::uint8_t>::max())
		{
			return {};
		}

		return calculated_geometry_type{ 0, geometry };
	}

}
