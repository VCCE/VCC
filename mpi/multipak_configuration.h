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

	explicit multipak_configuration(std::string section);

	void configuration_path(std::string path);
	std::string configuration_path() const;

	std::string last_accessed_module_type() const;

	void last_accessed_rom_path(const std::string& path) const;
	std::string last_accessed_rom_path() const;

	void last_accessed_dll_path(const std::string& path) const;
	std::string last_accessed_dll_path() const;

	void selected_slot(std::size_t slot) const;
	std::size_t selected_slot() const;

	void slot_cartridge_path(std::size_t slot, const std::string& path) const;
	std::string slot_cartridge_path(std::size_t slot) const;

private:

	const std::string section_;
	std::string configuration_path_;
};
