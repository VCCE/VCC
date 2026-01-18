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

namespace VCC::Util
{
	class settings
	{
	public:
		using path_type = std::string;

	public:
		LIBCOMMON_EXPORT explicit settings(path_type path);

		// write string value
		LIBCOMMON_EXPORT void write
			(const std::string& section,
			 const std::string& key,
			 const std::string& value) const;

		// write int value
		LIBCOMMON_EXPORT void write
			(const std::string& section,
			 const std::string& key,
			 int value) const;

		// write bool value
		LIBCOMMON_EXPORT void write_bool
			(const std::string& section,
			 const std::string& key,
			 bool value) const;

		// get char * value
		LIBCOMMON_EXPORT void read
			(const std::string& section,
			 const std::string& key,
			 const std::string& default_value,
			 char* buffer,
			 size_t buffer_size) const;

		// get string value
		LIBCOMMON_EXPORT std::string read
			(const std::string& section,
			 const std::string& key,
			 const std::string& default_value = {}) const;

		// get int value
		LIBCOMMON_EXPORT int read
			(const std::string& section,
			 const std::string& key,
			 int default_value) const;

	private:
		const path_type path_;
	};
}
