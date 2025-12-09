#pragma once
#include <string>

//TODO:  Eliminate this insanity. This class gains nothing but obscurity

class multipak_configuration
{
public:

	using slot_id_type = std::size_t;
	using path_type = std::string;
	using string_type = std::string;


public:

	explicit multipak_configuration(string_type section);

	// in configuration
	void configuration_path(path_type path);	//	FIXME: maybe make private with a friend setter
	path_type configuration_path() const;

	void last_accessed_module_type(const path_type& path) const;
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
