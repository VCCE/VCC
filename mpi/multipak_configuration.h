#pragma once
#include "vcc/utils/persistent_value_section_store.h"
#include "vcc/utils/resource_location.h"
#include "vcc/bus/expansion_port_ui.h"
#include <string>
#include <optional>


namespace vcc::cartridges::multipak
{

	/// @brief The Multi-Pak configuration store.
	///
	/// This class provides access to the configuration values used by the
	/// Multi-Pak Cartridge.
	class multipak_configuration
	{
	public:

		/// @brief Defines the type used to hold a variable length string.
		using string_type = std::string;
		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;
		/// @brief Type alias for slot indexes and identifiers.
		using slot_id_type = std::size_t;
		/// @brief Type alias for the component providing user interface services to the
		/// cartridge plugin.
		using expansion_port_ui_type = ::vcc::bus::expansion_port_ui;
		/// @brief Type alias of the value store used to serialize the configuration.
		using value_store_type = ::vcc::utils::persistent_value_section_store;
		/// @brief Type alias for a resource location
		using resource_location_type = ::vcc::utils::resource_location;


	public:

		/// @brief Construct a Multi-Pak Configuration.
		/// 
		/// @param ui A pointer to the UI services interface.
		/// @param path The path fo the configuration file
		/// @param section The section in the configuration file to store the values.
		/// 
		/// @throws std::invalid_argument if `ui` is null.
		multipak_configuration(
			std::shared_ptr<expansion_port_ui_type> ui,
			path_type path,
			string_type section);

		/// @brief Sets the last path accessed.
		/// 
		/// Sets the last path accessed when successfully loading a cartridge.
		/// 
		/// @param path The new path to set.
		void last_accessed_path(const path_type& path);

		/// @brief Retrieves the last path accessed.
		/// 
		/// Retrieves the last path accessed when successfully loading a cartridge.
		/// 
		/// @return The last path accessed.
		[[nodiscard]] path_type last_accessed_path() const;

		/// @brief Sets the startup slot.
		/// 
		/// @param slot The new startup slot.
		void selected_slot(slot_id_type slot);

		/// @brief Retrieves the startup slot.
		/// 
		/// @return The startup slot.
		[[nodiscard]] slot_id_type selected_slot() const;

		/// @brief Sets the cartridge path for a slot.
		/// 
		/// Sets the path of the cartridge to load for a specific slot.
		/// 
		/// @param slot The slot the cartridge should be loaded in.
		/// @param path The path to the cartridge to load.
		void slot_path(slot_id_type slot, const resource_location_type& path);

		/// @brief Sets the cartridge path to an empty value for a slot.
		/// 
		/// @param slot The slot to set the cartridge path for.
		void slot_path(slot_id_type slot, std::nullptr_t);

		/// @brief Retrieves the cartridge path for a slot.
		/// 
		/// @param slot The slot to the retrieve the path for.
		/// 
		/// @return The path.
		[[nodiscard]] std::optional<resource_location_type> slot_path(slot_id_type slot) const;


	private:

		/// @brief Generate a value key for a specific slot.
		/// 
		/// @param slot The slot to generate the key for.
		/// 
		/// @return A string containing the generate key.
		[[nodiscard]] string_type get_slot_path_key(slot_id_type slot) const;


	private:

		/// @brief The expansion port UI service provider.
		const std::shared_ptr<expansion_port_ui_type> ui_;
		/// @brief The value store that manages serializing the configuration values.
		value_store_type value_store_;
	};

}
