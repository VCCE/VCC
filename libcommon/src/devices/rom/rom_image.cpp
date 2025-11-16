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
#include "vcc/devices/rom/rom_image.h"
#include "vcc/utils/filesystem.h"
#include <fstream>
#include <Windows.h>


namespace vcc::devices::rom
{

	bool rom_image::load(path_type filename)
	{
		auto rom_data(::vcc::utils::load_file_to_vector(filename));
		if (!rom_data.has_value() || rom_data->empty())
		{
			return false;
		}

		filename_ = move(filename);
		data_ = move(*rom_data);

		return true;
	}

}
