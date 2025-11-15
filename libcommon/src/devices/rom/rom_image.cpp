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
#include <fstream>
#include <Windows.h>


namespace vcc::devices::rom
{

	bool rom_image::load(path_type filename)
	{
		std::ifstream input(filename, std::ios::binary);
		if (!input.is_open())
		{
			return false;
		}

		std::streamoff begin = input.tellg();
		input.seekg(0, std::ios::end);
		std::streamoff end = input.tellg();
		std::streamoff file_length = end - begin;
		input.seekg(0, std::ios::beg);

		std::vector<unsigned char> rom_data(static_cast<size_t>(file_length));
		input.read(reinterpret_cast<char*>(&rom_data[0]), rom_data.size());

		if (file_length != rom_data.size())
		{
			return false;
		}

		filename_ = move(filename);
		data_ = move(rom_data);

		return true;
	}

}
