#include "multipak_configuration.h"
#include <vcc/core/initial_settings.h>

using settings = ::vcc::core::initial_settings;

// Constructor sets the section
multipak_configuration::multipak_configuration(string_type section)
	: section_(move(section))
{}

// Set ini file path
void multipak_configuration::configuration_path(path_type path)
{
	configuration_path_ = move(path);
}

// Return ini file path
multipak_configuration::path_type multipak_configuration::configuration_path() const
{
	return configuration_path_;
}

// last rom path
void multipak_configuration::last_accessed_rom_path(const path_type& path) const
{
	settings(configuration_path()).write("DefaultPaths", "PakPath", path);
}
multipak_configuration::path_type multipak_configuration::last_accessed_rom_path() const
{
	return settings(configuration_path()).read("DefaultPaths", "PakPath");
}

// last dll path
void multipak_configuration::last_accessed_dll_path(const path_type& path) const
{
	settings(configuration_path()).write("DefaultPaths", "MPIPath", path);
}
multipak_configuration::path_type multipak_configuration::last_accessed_dll_path() const
{
	return settings(configuration_path()).read("DefaultPaths", "MPIPath");
}

// selected slot
void multipak_configuration::selected_slot(slot_id_type slot) const
{
	settings(configuration_path()).write(section_, "SWPOSITION", slot);
}
multipak_configuration::slot_id_type multipak_configuration::selected_slot() const
{
	return settings(configuration_path()).read(section_, "SWPOSITION", 3);
}

// slot path
void multipak_configuration::slot_cartridge_path(slot_id_type slot, const path_type& path) const
{
	settings(configuration_path()).write(section_, get_slot_path_key(slot), path);
}
multipak_configuration::path_type multipak_configuration::slot_cartridge_path(slot_id_type slot) const
{
	return settings(configuration_path()).read(section_, get_slot_path_key(slot));
}

// slot path string with SLOT prepended
multipak_configuration::string_type multipak_configuration::get_slot_path_key(slot_id_type slot) const
{
	return "SLOT" + std::to_string(slot + 1);
}
