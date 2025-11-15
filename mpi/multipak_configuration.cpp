#include "multipak_configuration.h"
#include "vcc/utils/persistent_value_store.h"


using value_store = ::vcc::utils::persistent_value_store;


multipak_configuration::multipak_configuration(string_type section)
	: section_(move(section))
{}


void multipak_configuration::configuration_path(path_type path)
{
	configuration_path_ = move(path);
}

multipak_configuration::path_type multipak_configuration::configuration_path() const
{
	return configuration_path_;
}


multipak_configuration::path_type multipak_configuration::last_accessed_module_path() const
{
	return value_store(configuration_path()).read("DefaultPaths", "MPIPath");
}

void multipak_configuration::last_accessed_module_path(const path_type& path) const
{
	value_store(configuration_path()).write("DefaultPaths", "MPIPath", path);
}


void multipak_configuration::selected_slot(slot_id_type slot) const
{
	value_store(configuration_path()).write(section_, "SWPOSITION", slot);
}

multipak_configuration::slot_id_type multipak_configuration::selected_slot() const
{
	return value_store(configuration_path()).read(section_, "SWPOSITION", 3);
}


void multipak_configuration::slot_cartridge_path(slot_id_type slot, const path_type& path) const
{
	value_store(configuration_path()).write(section_, get_slot_path_key(slot), path);
}

multipak_configuration::path_type multipak_configuration::slot_cartridge_path(slot_id_type slot) const
{
	return value_store(configuration_path()).read(section_, get_slot_path_key(slot));
}


multipak_configuration::string_type multipak_configuration::get_slot_path_key(slot_id_type slot) const
{
	return "SLOT" + std::to_string(slot + 1);
}
