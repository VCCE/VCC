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
#include <vcc/core/detail/exports.h>
#include <string>


namespace vcc::utils
{

	class configuration_serializer
	{
	public:

		using path_type = ::std::string;
		using string_type = ::std::string;

	public:

		LIBCOMMON_EXPORT explicit configuration_serializer(path_type path);

		LIBCOMMON_EXPORT void write(const string_type& section, const string_type& key, int value) const;
		LIBCOMMON_EXPORT void write(const string_type& section, const string_type& key, const string_type& value) const;

		LIBCOMMON_EXPORT int read(const string_type& section, const string_type& key, int default_value) const;
		LIBCOMMON_EXPORT string_type read(const string_type& section, const string_type& key, const string_type& default_value = {}) const;


	private:

		const path_type path_;
	};
}
