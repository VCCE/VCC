////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute it and/or
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
#include "vcc/utils/persistent_value_section_store.h"
#include <Windows.h>


namespace vcc::utils
{

	persistent_value_section_store::persistent_value_section_store(
		path_type path,
		section_type section)
		:
		store_(move(path)),
		section_(move(section))
	{}


	void persistent_value_section_store::remove(const string_type& key) const
	{
		store_.remove(section_, key);
	}

	void persistent_value_section_store::write(
		const string_type& key,
		int value) const
	{
		store_.write(section_, key, value);
	}

	void persistent_value_section_store::write(
		const string_type& key,
		const string_type& value) const
	{
		store_.write(section_, key, value);
	}

	int persistent_value_section_store::read(const string_type& key, const int& default_value) const
	{
		return store_.read(section_, key, default_value);
	}

	persistent_value_section_store::size_type persistent_value_section_store::read(
		const string_type& key,
		const size_type& default_value) const
	{
		return store_.read(section_, key, default_value);
	}

	bool persistent_value_section_store::read(const string_type& key, bool default_value) const
	{
		return store_.read(section_, key, default_value);
	}

	persistent_value_section_store::string_type persistent_value_section_store::read(
		const string_type& key,
		const string_type& default_value) const
	{
		return store_.read(section_, key, default_value);
	}

	persistent_value_section_store::string_type persistent_value_section_store::read(
		const string_type& key,
		const char* default_value) const
	{
		return store_.read(section_, key, default_value);
	}

}
