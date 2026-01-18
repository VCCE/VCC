////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	This is an expansion module for the Vcc Emulator. It simulated the functions
//	of the TRS-80 Multi-Pak Interface.
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

class multipak_configuration
{
public:

	using slot_id_type = std::size_t;
	using path_type = std::string;
	using string_type = std::string;


public:

	explicit multipak_configuration(string_type section);

	void configuration_path(path_type path);
	path_type configuration_path() const;

	//void last_accessed_module_type(const path_type& path) const;
	path_type last_accessed_module_type() const;

	void last_accessed_rom_path(const path_type& path) const;
	path_type last_accessed_rom_path() const;

	void last_accessed_dll_path(const path_type& path) const;
	path_type last_accessed_dll_path() const;

	void selected_slot(slot_id_type slot) const;
	slot_id_type selected_slot() const;

	void slot_cartridge_path(slot_id_type slot, const path_type& path) const;
	path_type slot_cartridge_path(slot_id_type slot) const;


private:

	string_type get_slot_path_key(slot_id_type slot) const;


private:

	const string_type section_;
	path_type configuration_path_;
};
