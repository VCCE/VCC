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
#include "vcc/media/geometry_calculators/floppy_disk_geometry_calculator.h"


namespace vcc::media::geometry_calculators
{

	floppy_disk_geometry_calculator::floppy_disk_geometry_calculator(const geometry_type& default_geometry)
		:
		geometry_calculator(default_geometry),
		raw_geometry_calculator_(default_geometry),
		os9_geometry_calculator_(default_geometry),
		jvc_geometry_calculator_(default_geometry),
		vdk_geometry_calculator_(default_geometry)
	{}

	floppy_disk_geometry_calculator::optional_calculated_geometry_type floppy_disk_geometry_calculator::calculate(
		const header_buffer_type& header_buffer,
		size_type file_size) const
	{
		switch (file_size % default_geometry().sector_size)
		{
		case raw_disk_geometry_calculator::header_size:
			// If the image contains an OS-9 formatted disk and the geometry can be
			// determined from it's header then we use the geometry it gives us.
			if (auto geometry(os9_geometry_calculator_.calculate(header_buffer, file_size));
				geometry.has_value())
			{
				return geometry.value();
			}

			return raw_geometry_calculator_.calculate(header_buffer, file_size);

		case jvc_disk_geometry_calculator::header_size:
			// The original implementation supported header sizes of 3 (which included
			// the sector size) and 4 (which included the first valid sector id), neither
			// of which seem to be relevant to the CoCo (even as an oddity) so only the
			// variant including the sector and head counts is supported.
			return jvc_geometry_calculator_.calculate(header_buffer, file_size);

		case vdk_disk_geometry_calculator::header_size:
			return vdk_geometry_calculator_.calculate(header_buffer, file_size);
		}

		return {};
	}

}