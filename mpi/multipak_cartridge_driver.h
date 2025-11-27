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
#include "multipak_expansion_port.h"
#include "multipak_configuration.h"
#include "vcc/bus/cartridge_driver.h"
#include <array>


namespace vcc::cartridges::multipak
{

	/// @brief Multi-Pak Interface driver.
	///
	/// This cartridge driver emulates the Multi-Pak Interface. It provides the standard
	/// features of 4 individual cartridge slots, the ability to insert and eject
	/// cartridges and select the active/startup slot.
	class multipak_cartridge_driver : public ::vcc::bus::cartridge_driver
	{
	public:

		/// @brief Type alias for the component providing global system services to the
		/// cartridge plugin.
		using expansion_port_host_type = ::vcc::bus::expansion_port_host;
		/// @brief Type alias for the component acting as the expansion bus the cartridge plugin
		/// is connected to.
		using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
		/// @brief Type alias for cartridges plugins managed by Multi-Pak.
		using cartridge_type = ::vcc::bus::cartridge;
		/// @brief Type alias for managed cartridges plugins managed by Multi-Pak.
		using cartridge_ptr_type = std::shared_ptr<cartridge_type>;
		/// @brief Type alias for status values returned when loading or inserting a cartridge.
		using mount_status_type = ::vcc::utils::cartridge_loader_status;
		/// @brief Type alias for slot indexes and identifiers.
		using slot_id_type = std::size_t;
		/// @brief TYpe alias for a variable length name.
		using name_type = std::string;
		/// @brief Type alias to lengths, 1 dimension sizes, and indexes.
		using size_type = std::size_t;
		/// @brief Type alias for library handles with a managed lifetime.
		using managed_handle_type = std::unique_ptr<
			std::remove_pointer_t<HMODULE>,
			vcc::utils::dll_deleter>;

		/// @brief Total number of slots supported by the Multi-Pak driver.
		static constexpr size_type total_slot_count = 4;

	public:

		/// @brief Construct the Multi-Pak Cartridge Driver.
		/// 
		/// @param bus A pointer to the bus interface.
		/// @param configuration A pointer to the configuration for this instance.
		/// 
		/// @throws std::invalid_argument if `bus` is null.
		/// @throws std::invalid_argument if `configuration` is null.
		multipak_cartridge_driver(
			std::shared_ptr<expansion_port_bus_type> bus,
			std::shared_ptr<multipak_configuration> configuration);

		/// @brief Starts the driver.
		///
		/// @todo pass the startup slot as a parameter and eliminate the need to pass the
		/// configuration.
		/// 
		/// Starts the driver and sets the active, CTS, and SCS slot selections to the ones
		/// specified in the configuration.
		void start();

		/// @brief Stops the driver.
		///
		/// Stops and ejects all cartridges inserted into the Multi-Pak and stops the driver.
		void stop();

		/// @brief Resets the device.
		///
		/// Resets the device by setting the CTS and SCS selected slots to the active/startup
		/// slot, resets all inserted cartridges, and sets the cartridge line for the active
		/// slot.
		void reset() override;

		/// @brief Determines if a slot is empty.
		/// 
		/// @param slot The slot to check.
		/// 
		/// @return `true` if the slot is empty; `false` otherwise.
		[[nodiscard]] bool empty(slot_id_type slot) const;

		/// @brief Retrieves the total number of slots supported.
		/// 
		/// @return The total number of slots selected.
		[[nodiscard]] size_type slot_count() const
		{
			return slots_.size();
		}

		/// @brief Retrieves the _name_ of the cartridge in a slot.
		/// 
		/// @todo this should return std::optional
		/// 
		/// @param slot The slot to retrieve the name from.
		/// @return The name of the cartridge in the specified slot or an empty string if no
		/// cartridge is in the slot.
		[[nodiscard]] name_type slot_name(slot_id_type slot) const;

		/// @brief Sets the active/startup slot.
		/// 
		/// @param slot The new active/startup slot.
		void switch_to_slot(slot_id_type slot);

		/// @brief Retrieves the active/startup slot.
		/// 
		/// Retrieves the active/startup slot. This is the slot selected by the virtual front
		/// panel switch of the Multi-Pak.
		/// 
		/// @return The index of the active/startup slot.
		[[nodiscard]] slot_id_type selected_switch_slot() const;

		/// @brief Returns the Spare Chip Select slot.
		/// 
		/// Returns the Spare Chip Select slot selected through software control via the
		/// Multi-Paks memory mapped I/O register.
		/// 
		/// @return The slot index of the slot selected as the Spare Chip Select slot.
		[[nodiscard]] slot_id_type selected_scs_slot() const;

		/// @brief Returns the CTS slot.
		/// 
		/// Returns the CTS slot selected through software control via the Multi-Paks
		/// memory mapped I/O register.
		/// 
		/// @return The slot index of the slot selected as the CTS slot.
		[[nodiscard]] slot_id_type selected_cts_slot() const;

		/// @brief Inserts a cartridge into a slot.
		/// 
		/// Inserts a cartridge into a slot. If a cartridge is already inserted into the slot
		/// it will be ejected.
		/// 
		/// @param slot The index of the slot to insert the cartridge into.
		/// @param handle The library handle containing the cartridge plugin.
		/// @param cartridge The cartridge to insert into the slot.
		void insert_cartridge(
			slot_id_type slot,
			managed_handle_type handle,
			cartridge_ptr_type cartridge);

		/// @brief Eject a cartridge from a slot.
		/// 
		/// @param slot The index of the slot to eject the cartridge from.
		void eject_cartridge(slot_id_type slot);

		/// @inheritdoc
		[[nodiscard]] unsigned char read_memory_byte(size_type memory_address) override;

		/// @inheritdoc
		void write_port(unsigned char port_id, unsigned char value) override;

		/// @inheritdoc
		[[nodiscard]] unsigned char read_port(unsigned char port_id) override;

		/// @inheritdoc
		void update(float delta) override;

		/// @inheritdoc
		[[nodiscard]] sample_type sample_audio() override;

		/// @brief Set the cartridge select line.
		/// 
		/// Set the cartridge select line for a specific slot.
		/// 
		/// @todo Make automatic when mounting, ejecting, selecting slot, etc.
		/// 
		/// @param slot The slot to set the select line for.
		/// @param line_state The new state of the cartridge select line.
		void set_cartridge_select_line(slot_id_type slot, bool line_state);


	private:

		/// @brief Defines the memory mapped I/O ports used by the Multi-Pak hardware.
		struct mmio_ports
		{
			/// @brief The cartridge slot CTS/SCS select register.
			static const size_t slot_select = 0x7f;
		};

		/// @brief Defines default values used by the Multi-Pak driver.
		struct defaults
		{
			/// @brief The default active/startup slot if one is not configured.
			static const size_t switch_slot = 0x03;
			/// @brief The initial value of the slot CTS/SCS select register.
			static const size_t slot_register = 0xff;
		};

		/// @brief A pointer to the bus the cartridge is connected to.
		const std::shared_ptr<expansion_port_bus_type> bus_;
		/// @brief A pointer to the Multi-Pak configuration.
		const std::shared_ptr<multipak_configuration> configuration_;
		/// @brief A collection of slots that cartridges can be inserted into.
		std::array<multipak_expansion_port, total_slot_count> slots_;
		/// @brief The current value of the CTS/CTS selection register.
		unsigned char slot_register_ = defaults::slot_register;
		/// @brief The current slot selected by the virtual front panel switch.
		slot_id_type switch_slot_ = defaults::switch_slot;
		/// @brief The currently selected CTS slot
		slot_id_type cached_cts_slot_ = defaults::switch_slot;
		/// @brief The currently selected SCS (Spare Chip Select) slot
		slot_id_type cached_scs_slot_ = defaults::switch_slot;
	};

}
