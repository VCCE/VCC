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
#include <vcc/utils/configuration_serializer.h>
#include <Windows.h>


namespace vcc::utils
{

	configuration_serializer::configuration_serializer(path_type path)
		: path_(move(path))
	{
	}

	void configuration_serializer::write(
		const string_type& section,
		const string_type& key,
		int value) const
	{
		WritePrivateProfileString(
			section.c_str(),
			key.c_str(),
			std::to_string(value).c_str(),
			path_.c_str());
	}

	void configuration_serializer::write(
		const string_type& section,
		const string_type& key,
		const string_type& value) const
	{
		WritePrivateProfileString(section.c_str(), key.c_str(), value.c_str(), path_.c_str());
	}

	int configuration_serializer::read(const string_type& section, const string_type& key, const int& default_value) const
	{
		return GetPrivateProfileInt(section.c_str(), key.c_str(), default_value, path_.c_str());
	}

	configuration_serializer::size_type configuration_serializer::read(
		const string_type& section,
		const string_type& key,
		const size_type& default_value) const
	{
		return GetPrivateProfileInt(section.c_str(), key.c_str(), default_value, path_.c_str());
	}

	bool configuration_serializer::read(const string_type& section, const string_type& key, bool default_value) const
	{
		return GetPrivateProfileInt(section.c_str(), key.c_str(), default_value, path_.c_str()) != 0;
	}

	configuration_serializer::string_type configuration_serializer::read(
		const string_type& section,
		const string_type& key,
		const string_type& default_value) const
	{
		// FIXME-CHET: Need to determine the size of the string
		char loaded_string[MAX_PATH] = {};

		GetPrivateProfileString(
			section.c_str(),
			key.c_str(),
			default_value.c_str(),
			loaded_string,
			MAX_PATH,
			path_.c_str());

		return loaded_string;
	}

}
