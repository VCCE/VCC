#pragma once
#include <string>
#include "vcc/utils/persistent_value_section_store.h"


namespace vcc::cartridges::multipak
{

	/// @brief The Multi-Pak configuration store.
	///
	/// This class provides access to the configuration values used by the
	/// Multi-Pak Cartridge.
	class multipak_configuration
	{
	public:

		/// @brief Type alias of the value store used to serialize the configuration.
		using value_store_type = ::vcc::utils::persistent_value_section_store;
		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;
		/// @brief Type alias for slot indexes and identifiers.
		using slot_id_type = std::size_t;
		/// @brief Defines the type used to hold a variable length string.
		using string_type = std::string;


	public:

		/// @brief Construct a Multi-Pak Configuration.
		/// 
		/// @param path The path fo the configuration file
		/// @param section The section in the configuration file to store the values.
		multipak_configuration(path_type path, string_type section);

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

		/// @brief Sets the active/startup slot.
		/// 
		/// @param slot The new active/startup slot.
		void selected_slot(slot_id_type slot);

		/// @brief Retrieves the active/startup slot.
		/// 
		/// @return The active/startup slot.
		[[nodiscard]] slot_id_type selected_slot() const;

		/// @brief Sets the cartridge path for a slot.
		/// 
		/// Sets the path of the cartridge to load for a specific slot.
		/// 
		/// @param slot The slot the cartridge should be loaded in.
		/// @param path The path to the cartridge to load.
		void slot_path(slot_id_type slot, const path_type& path);

		/// @brief Retrieves the cartridge path for a slot.
		/// 
		/// @param slot The slot to the retrieve the path for.
		/// 
		/// @return The path.
		[[nodiscard]] path_type slot_path(slot_id_type slot) const;


	private:

		/// @brief Generate a value key for a specific slot.
		/// 
		/// @param slot The slot to generate the key for.
		/// 
		/// @return A string containing the generate key.
		[[nodiscard]] string_type get_slot_path_key(slot_id_type slot) const;


	private:

		/// @brief The value store that manages serializing the configuration values.
		value_store_type value_store_;
	};

}
