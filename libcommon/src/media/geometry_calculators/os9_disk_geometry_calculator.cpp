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
#include "vcc/media/geometry_calculators/os9_disk_geometry_calculator.h"


namespace vcc::media::geometry_calculators
{

	os9_disk_geometry_calculator::optional_calculated_geometry_type os9_disk_geometry_calculator::calculate(
		const header_buffer_type& header_buffer,
		[[maybe_unused]] size_type file_size) const
	{
		const auto total_sectors
			= (header_buffer[0] << 16)
			+ (header_buffer[1] << 8)
			+ (header_buffer[2]);
		const auto head_count = (header_buffer[16] & 1) + 1;
		const auto sectors_per_track = header_buffer[3];

		// We always expect there to be 18 sectors on an OS-9 formatted floppy disk image
		// with no left over sectors or incomplete tracks.
		if (sectors_per_track != 18 || total_sectors % (head_count * sectors_per_track) != 0)
		{
			return {};
		}

		auto geometry(default_geometry());

		geometry.head_count = head_count;
		geometry.track_count = total_sectors / sectors_per_track;
		geometry.sector_count = sectors_per_track;

		return calculated_geometry_type{ 0u, geometry };
	}

}
