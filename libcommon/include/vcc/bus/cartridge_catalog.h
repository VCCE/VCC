////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
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
#include <vcc/bus/cartridge_catalog_item.h>
#include <vcc/detail/exports.h>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <filesystem>
#include <optional>
#include <functional>


namespace vcc::bus
{

	/// @brief Basic Cartridge Catalog.
	///
	/// The Cartridge Catalog provides information about Cartridges that are installed and
	/// available to the system. This class serves as a stepping stone in achieving a more
	/// robust plugin system and should not be extended much beyond where it is.
	class cartridge_catalog
	{
	public:

		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;
		/// @brief Type alias for a cartridge catalog item.
		using item_type = cartridge_catalog_item;
		/// @brief Type alias for the unique identifier used to identify cartridges.
		using guid_type = item_type::guid_type;
		/// @brief Type alias for the sequential container of catalog items.
		using item_container_type = std::vector<item_type>;
		/// @brief Type alias for the associative container of catalog items.
		using item_map_type = std::map<guid_type, item_type>;
		/// @brief Type alias for the visitor function.
		using visitor_function_type = std::function<void(const guid_type&, const item_type&)>;


	public:

		/// @brief Construct the Cartridge Catalog.
		/// 
		/// @param cartridge_path System path to where cartridges are stored.
		/// 
		/// @throws std::invalid_argument if the cartridge path is empty.
		/// @throws std::invalid_argument if the cartridge path does not exist.
		LIBCOMMON_EXPORT explicit cartridge_catalog(path_type cartridge_path);
		
		LIBCOMMON_EXPORT virtual ~cartridge_catalog() = default;

		/// @brief Determines if the Catalog is empty.
		/// 
		/// @return `true` if the Catalog is empty; `false` otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT virtual bool empty() const;

		/// @brief Determines if an identifier represents an installed cartridge.
		/// 
		/// @param id The identifier to check.
		/// 
		/// @return `true` if the cartridge is installed; `false` otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT virtual bool is_valid_cartridge_id(const guid_type& id) const;

		/// @brief Determines if a cartridge is already loaded.
		/// 
		/// @param id The unique identifier of the cartridge.
		/// 
		/// @return `true` if the cartridge is loaded; `false` otherwise.
		/// 
		/// @throws std::invalid_argument if a cartridge matching the identifier is not found.
		[[nodiscard]] LIBCOMMON_EXPORT virtual bool is_loaded(const guid_type& id) const;

		/// @brief Get the absolute pathname to a cartridge file.
		/// 
		/// @param id The unique identifier of the cartridge.
		/// 
		/// @return An absolute system path to the cartridge file.
		/// 
		/// @throws std::invalid_argument if a cartridge matching the identifier is not found.
		[[nodiscard]] LIBCOMMON_EXPORT virtual path_type get_item_pathname(const guid_type& id) const;

		/// @brief Create a copy of the Cartridge items in an associative container.
		/// 
		/// @return An associative container containing copies of all cartridge items.
		[[nodiscard]] LIBCOMMON_EXPORT virtual item_map_type copy_items() const;

		/// @brief Create a copy of the Cartridge items in a sequence container.
		/// 
		/// @return A sequence container containing copies of all cartridge items.
		[[nodiscard]] LIBCOMMON_EXPORT virtual item_container_type copy_items_ordered() const;

		/// @brief Accept a cartridge item visitor.
		/// 
		/// @param visitor A reference to the visitor.
		LIBCOMMON_EXPORT virtual void accept(const visitor_function_type& visitor) const;


	private:

		/// @brief The system path to where cartridge files are stored.
		const path_type cartridge_path_;
		/// @brief The list of currently available cartridges.
		item_map_type items_;
	};

}
