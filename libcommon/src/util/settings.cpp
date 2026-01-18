//#define USE_LOGGING
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

#include <vcc/util/settings.h>
#include <vcc/util/logger.h>
#include <Windows.h>

namespace VCC::Util
{
	settings::settings(path_type path)
		: path_(move(path))
	{}

	// save string value
	void settings::write(
		const std::string& section,
		const std::string& key,
		const std::string& value) const
	{
		::WritePrivateProfileString(
				section.c_str(),
				key.c_str(),
				value.c_str(),
				path_.c_str());
	}

	// save int value
	void settings::write
		(const std::string& section,
		 const std::string& key,
		 int value) const
	{
		::WritePrivateProfileString(
			section.c_str(),
			key.c_str(),
			std::to_string(value).c_str(),
			path_.c_str());
	}

	// save bool value
	void settings::write_bool
		(const std::string& section,
		 const std::string& key,
		 bool value) const
	{ 
		int ival = value ? 1 : 0;
		write(section, key, ival);
	}

	// get char * value
	void settings::read
		(const std::string& section,
		 const std::string& key,
		 const std::string& default_value,
		 char* buffer,
		 size_t buffer_size) const
	{
		::GetPrivateProfileStringA(
		section.c_str(),
		key.c_str(),
		default_value.c_str(),
		buffer,
		static_cast<DWORD>(buffer_size),
		path_.c_str());
	}

	// get string value
	std::string settings::read
		(const std::string& section,
		 const std::string& key,
		 const std::string& default_value) const
	{
		char buffer[MAX_PATH] = {};
		read(section, key, default_value, buffer, sizeof(buffer));
		return std::string(buffer);
	}

	// get int value
	int settings::read
		(const std::string& section,
		 const std::string& key,
		 int default_value) const
	{
		return ::GetPrivateProfileInt(
				section.c_str(),
				key.c_str(),
				default_value,
				path_.c_str());
	}
}
