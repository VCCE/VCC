#include "multipak_configuration.h"


namespace vcc::cartridges::multipak
{

	multipak_configuration::multipak_configuration(path_type path, string_type section)
		: value_store_(move(path), move(section))
	{
	}


	multipak_configuration::path_type multipak_configuration::last_accessed_path() const
	{
		return value_store_.read("paths.cartridges");
	}

	void multipak_configuration::last_accessed_path(const path_type& path)
	{
		value_store_.write("paths.cartridges", path);
	}


	void multipak_configuration::selected_slot(slot_id_type slot)
	{
		value_store_.write("selected_slot_index", slot);
	}

	multipak_configuration::slot_id_type multipak_configuration::selected_slot() const
	{
		return value_store_.read("selected_slot_index", 3);
	}


	void multipak_configuration::slot_path(slot_id_type slot, const path_type& path)
	{
		value_store_.write(get_slot_path_key(slot), path);
	}

	multipak_configuration::path_type multipak_configuration::slot_path(slot_id_type slot) const
	{
		return value_store_.read(get_slot_path_key(slot));
	}


	multipak_configuration::string_type multipak_configuration::get_slot_path_key(slot_id_type slot) const
	{
		return "paths.slot" + std::to_string(slot + 1);
	}

}
