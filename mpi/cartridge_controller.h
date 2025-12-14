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
#include <string>


namespace vcc::cartridges::multipak
{

	/// @brief Interface for controlling the Multi-Pak cartridge.
	class multipak_cartridge_controller
	{
	public:

		/// @brief Type alias for slot indexes and identifiers.
		using slot_id_type = std::size_t;
		/// @brief TYpe alias for a variable length name.
		using name_type = std::string;


	public:

		virtual ~multipak_cartridge_controller() = default;

		/// @brief Determines if a slot is empty.
		/// 
		/// @param slot_id The slot to check.
		/// 
		/// @return `true` if the slot is empty; `false` otherwise.
		virtual bool is_cartridge_slot_empty(slot_id_type slot_id) const = 0;

		/// @brief Retrieves the _name_ of the cartridge in a slot.
		/// 
		/// @todo this should return std::optional
		/// 
		/// @param slot_id The slot to retrieve the name from.
		/// 
		/// @return The name of the cartridge in the specified slot or an empty string if no
		/// cartridge is in the slot.
		virtual name_type get_cartridge_slot_name(slot_id_type slot_id) const = 0;

		/// @brief Sets the startup slot.
		/// 
		/// @param slot_id The new startup slot.
		/// @param reset If `true` resets the emulated computer.
		virtual void switch_to_slot(slot_id_type slot_id, bool reset) = 0;

		/// @brief Retrieves the startup slot.
		/// 
		/// Retrieves the startup slot. This is the slot selected by the virtual front
		/// panel switch of the Multi-Pak.
		/// 
		/// @return The index of the startup slot.
		virtual slot_id_type selected_switch_slot() const = 0;

		/// @brief Select and insert a Device Cartridge.
		/// 
		/// Select a Device Cartridge from a file and insert it into a specific slot.
		/// 
		/// @param slot_id The slot to insert the cartridge into.
		virtual void select_and_insert_cartridge(slot_id_type slot_id) = 0;

		/// @brief Select and insert a Rom Pak Cartridge.
		/// 
		/// Select a ROM PAk cartridge from a file and insert it into a specific slot.
		/// 
		/// @param slot_id The slot to insert the cartridge into.
		virtual void select_and_insert_rompak(slot_id_type slot_id) = 0;

		/// @brief Insert a cartridge from a slot.
		/// 
		/// Ejects a cartridge from a slot and optionally performs a hard reset of the
		/// of the system.
		/// 
		/// @param slot_id The slot containing the cartridge to eject.
		/// @param update_settings It `true` the settings for that slot will be updated
		/// to indicate no cartridge should be inserted in that slot the next time the
		/// emulator starts.
		/// @param allow_reset If `true` and the value in `slot` is the same as the
		/// currently CTS or SCS slot selected via the MPI's control register, the
		/// cartridge will request the system perform a hard reset.
		virtual void eject_cartridge(slot_id_type slot_id, bool update_settings, bool allow_reset) = 0;
	};

}