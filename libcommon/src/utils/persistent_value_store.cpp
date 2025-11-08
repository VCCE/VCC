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
#include <vcc/utils/persistent_value_store.h>
#include <Windows.h>


namespace vcc::utils
{

	persistent_value_store::persistent_value_store(path_type path)
		: path_(move(path))
	{
	}

	void persistent_value_store::write(
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

	void persistent_value_store::write(
		const string_type& section,
		const string_type& key,
		const string_type& value) const
	{
		WritePrivateProfileString(section.c_str(), key.c_str(), value.c_str(), path_.c_str());
	}

	int persistent_value_store::read(const string_type& section, const string_type& key, const int& default_value) const
	{
		return GetPrivateProfileInt(section.c_str(), key.c_str(), default_value, path_.c_str());
	}

	persistent_value_store::size_type persistent_value_store::read(
		const string_type& section,
		const string_type& key,
		const size_type& default_value) const
	{
		return GetPrivateProfileInt(section.c_str(), key.c_str(), default_value, path_.c_str());
	}

	bool persistent_value_store::read(const string_type& section, const string_type& key, bool default_value) const
	{
		return GetPrivateProfileInt(section.c_str(), key.c_str(), default_value, path_.c_str()) != 0;
	}

	persistent_value_store::string_type persistent_value_store::read(
		const string_type& section,
		const string_type& key,
		const string_type& default_value) const
	{
		return read(section, key, default_value.c_str());
	}

	persistent_value_store::string_type persistent_value_store::read(
		const string_type& section,
		const string_type& key,
		const char* default_value) const
	{
		// FIXME-CHET: There is no way to effectively determining the length of
		// the string that will be loaded. This will need to either use a fixed
		// buffer of 65536 characters (the maximum allowed), iterate growing
		// the buffer the string is loaded into until it is big enough, switch
		// to a library that better supports INI configuration instead of using
		// an API that's intended for 16bit compatibility.
		char loaded_string[MAX_PATH] = {};

		GetPrivateProfileString(
			section.c_str(),
			key.c_str(),
			default_value,
			loaded_string,
			MAX_PATH,
			path_.c_str());

		return loaded_string;
	}

}
