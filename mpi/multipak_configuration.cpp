#include "multipak_configuration.h"


namespace vcc::cartridges::multipak
{

	multipak_configuration::multipak_configuration(
		std::shared_ptr<expansion_port_ui_type> ui,
		path_type path,
		string_type section)
		: 
		ui_(move(ui)),
		value_store_(std::move(path), move(section))
	{
		if (ui_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Multi-Pak Cartridge. UI is null.");
		}
	}


	multipak_configuration::path_type multipak_configuration::last_accessed_path() const
	{
		return ui_->last_accessed_rompak_path();
	}

	void multipak_configuration::last_accessed_path(const path_type& path)
	{
		ui_->last_accessed_rompak_path(path);
	}


	void multipak_configuration::selected_slot(slot_id_type slot)
	{
		value_store_.write("selected_slot_index", slot);
	}

	multipak_configuration::slot_id_type multipak_configuration::selected_slot() const
	{
		return value_store_.read("selected_slot_index", 3);
	}


	void multipak_configuration::slot_path(slot_id_type slot, const resource_location_type& path)
	{
		value_store_.write(get_slot_path_key(slot), path.to_string());
	}

	void multipak_configuration::slot_path(slot_id_type slot, nullptr_t)
	{
		value_store_.write(get_slot_path_key(slot), nullptr);
	}

	std::optional<multipak_configuration::resource_location_type> multipak_configuration::slot_path(slot_id_type slot) const
	{
		return resource_location_type::from_string(value_store_.read(get_slot_path_key(slot)));
	}

	multipak_configuration::string_type multipak_configuration::get_slot_path_key(slot_id_type slot) const
	{
		return std::format("paths.slot{}", slot + 1);
	}

}
