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
#include <vcc/util/exports.h>  // defines LIBCOMMON_EXPORT if libcommon is a DLL
#include <string>
#include <vector>

namespace VCC::Core
{

	class rom_image
	{
	public:

		using path_type = std::string;
		using container_type = std::vector<unsigned char>;
		using value_type = container_type::value_type;
		using size_type = container_type::size_type;


	public:

		LIBCOMMON_EXPORT rom_image() = default;
		LIBCOMMON_EXPORT ~rom_image() = default;

		bool empty() const
		{
			return data_.empty();
		}

		path_type filename() const
		{
			return filename_;
		}

		LIBCOMMON_EXPORT bool load(path_type filename);

		void clear()
		{
			filename_.clear();
			data_.clear();
		}

		value_type read_memory_byte(size_type memory_address) const
		{
			if (data_.empty())
			{
				return 0;
			}

			return data_[memory_address % data_.size()];
		}


	private:

		path_type filename_;
		container_type data_;
	};

}
