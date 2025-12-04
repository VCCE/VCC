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
#include <vector>
#include <string>


namespace vcc::ui
{

	/// @brief Defines the shape of a file filter.
	/// 
	/// Defines the shape of a file filter containing a description and a list of filters.
	struct file_filter
	{
		/// @brief Type alias for a variable length string.
		using string_type = std::string;
		/// @brief Type alias for a vector of strings.
		using string_vector_type = std::vector<std::string>;

		/// @brief A string containing the description of the filters.
		string_type description;
		/// @brief A list of strings each containing a file filter.
		string_vector_type filters;
	};

}
