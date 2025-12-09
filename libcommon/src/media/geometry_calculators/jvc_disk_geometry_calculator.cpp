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
#include "vcc/media/geometry_calculators/jvc_disk_geometry_calculator.h"
#include <limits>


namespace vcc::media::geometry_calculators
{

	jvc_disk_geometry_calculator::optional_calculated_geometry_type jvc_disk_geometry_calculator::calculate(
		const header_buffer_type& header_buffer,
		size_type file_size) const
	{
		auto geometry(default_geometry());

		geometry.head_count = header_buffer[header_elements::head_count];
		// TODO-CHET: Maybe only accept head counts of 1 and 2.
		if (geometry.head_count == 0)
		{
			return {};
		}

		geometry.sector_count = header_buffer[header_elements::sector_count];

		const auto disk_image_size(file_size - header_size);
		const auto track_size_in_bytes(geometry.sector_count * geometry.sector_size * geometry.head_count);

		if (disk_image_size % track_size_in_bytes != 0)
		{
			return {};
		}

		geometry.track_count = disk_image_size / track_size_in_bytes;

		return calculated_geometry_type{ header_size, geometry };
	}

}
