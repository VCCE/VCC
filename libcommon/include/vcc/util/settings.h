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
#include <string>
#include <Windows.h>

namespace VCC::Util
{
	class settings
	{
	public:
		using path_type = std::string;

	public:
		explicit settings(path_type path);

		// write string value
		bool write
			(const std::string& section,
			 const std::string& key,
			 const std::string& value) const;

		// write int value
		bool write
			(const std::string& section,
			 const std::string& key,
			 int value) const;

		// write bool value
		bool write_bool
			(const std::string& section,
			 const std::string& key,
			 bool value) const;

		// get char * value
		bool read
			(const std::string& section,
			 const std::string& key,
			 const std::string& default_value,
			 char* buffer,
			 size_t buffer_size) const;

		// get string value
		std::string read
			(const std::string& section,
			 const std::string& key,
			 const std::string& default_value = {}) const;

		// get int value
		int read
			(const std::string& section,
			 const std::string& key,
			 int default_value) const;

		// delete key
		bool delete_key
			(const std::string& section,
			 const std::string& key) const
		{
			return ::WritePrivateProfileString
				(section.c_str(), key.c_str(), NULL, path_.c_str());
		}

		// delete section
		bool delete_section
			(const std::string& section) const
		{
			return ::WritePrivateProfileString
				(section.c_str(), NULL, NULL, path_.c_str());
		}

		// flush cache
		bool flush () const
		{
			return ::WritePrivateProfileString
				(NULL, NULL, NULL, path_.c_str());
		}

	private:
		const path_type path_;
	};
}
