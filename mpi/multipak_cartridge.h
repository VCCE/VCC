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
#include "multipak_cartridge_driver.h"
#include "configuration_dialog.h"
#include "multipak_configuration.h"
#include "vcc/bus/cartridge.h"
#include <array>


namespace vcc::cartridges::multipak
{

	/// @brief Multi-Pak Interface plugin.
	///
	/// This plugin provides an implementation of the Multi-Pak Interface, access to the
	/// Multi-Pak device driver, and user interface functionality for access to the Slot
	/// Manager and other plugin control features.
	class multipak_cartridge : public ::vcc::bus::cartridge
	{
	public:

		/// @brief Type alias for the component providing global system services to the
		/// cartridge plugin.
		using expansion_port_host_type = ::vcc::bus::expansion_port_host;
		/// @brief Type alias for the component providing user interface services to the
		/// cartridge plugin.
		using expansion_port_ui_type = ::vcc::bus::expansion_port_ui;
		/// @brief Type alias for the component acting as the expansion bus the cartridge plugin
		/// is connected to.
		using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
		/// @brief Type alias for cartridges plugins managed by Multi-Pak.
		using cartridge_type = ::vcc::bus::cartridge;
		/// @brief Type alias for managed cartridges plugins managed by Multi-Pak.
		using cartridge_ptr_type = std::shared_ptr<cartridge_type>;
		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;
		/// @brief Type alias for the component emulating the Multi-Pak hardware.
		using driver_type = multipak_cartridge_driver;
		/// @copydoc driver_type::mount_status_type
		using mount_status_type = driver_type::mount_status_type;
		/// @copydoc driver_type::slot_id_type
		using slot_id_type = driver_type::slot_id_type;


	public:

		/// @brief Construct a Multi-Pak Cartridge Plugin.
		/// 
		/// @param host A pointer to the host services interface.
		/// @param ui A pointer to the UI services interface.
		/// @param bus A pointer to the bus interface.
		/// @param driver A pointer to the driver implementing the Multi-Pak hardware.
		/// @param module_instance A handle to the instance of the module containing the
		/// resources for the Multi-Pak plugin.
		/// @param configuration A pointer to the configuration for this instance.
		/// 
		/// @throws std::invalid_argument if `host` is null.
		/// @throws std::invalid_argument if `ui` is null.
		/// @throws std::invalid_argument if `bus` is null.
		/// @throws std::invalid_argument if `driver` is null.
		/// @throws std::invalid_argument if `module_instance` is null.
		/// @throws std::invalid_argument if `configuration` is null.
		multipak_cartridge(
			std::shared_ptr<expansion_port_host_type> host,
			std::shared_ptr<expansion_port_ui_type> ui,
			std::shared_ptr<expansion_port_bus_type> bus,
			std::shared_ptr<multipak_cartridge_driver> driver,
			HINSTANCE module_instance,
			std::shared_ptr<multipak_configuration> configuration);

		multipak_cartridge(const multipak_cartridge&) = delete;
		multipak_cartridge(multipak_cartridge&&) = delete;

		multipak_cartridge& operator=(const multipak_cartridge&) = delete;
		multipak_cartridge& operator=(multipak_cartridge&&) = delete;

		/// @inheritdoc
		[[nodiscard]] name_type name() const override;

		/// @inheritdoc
		[[nodiscard]] driver_type& driver() override;

		/// @brief Initialize the device.
		///
		/// Initialize the cartridge device to a default state and automatically inserts and
		/// initializes all cartridges into the slots specified in the configuration. If this
		/// called more than once without stopping the device the behavior is undefined.
		void start() override;

		/// @brief Terminate the plugin.
		///
		/// Stops all device operations, stops and ejects all cartridges, and releases all
		/// resources. This function must be called prior to the cartridge being destroyed to
		/// ensure that all resources are gracefully released. If this function is called
		/// before the cartridge is initialized or after it has been terminated the behavior
		/// is undefined.
		void stop() override;

		/// @brief Retrieve the status of the Multi-Pak and all inserted cartridges.
		/// 
		/// Retrieve the status of the Multi-Pak and all inserted cartridges as a human
		/// readable string. The status of the Multi-Pak status is placed at the front of
		/// the status text with each inserted cartridge that generates a status following.
		/// Each status is separated from those on either side by a pipe character (`|`).
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated the behavior is undefined.
		status_type status() const override;

		/// @brief Execute a command based on a menu item identifier.
		/// 
		/// This override processes commands for both the Multi-Pak interface as well as the
		/// cartridges that are inserted in the various slots. If the menu item id belongs to
		/// the Multi-Pak it will be processed and executed immediately. If the menu item id
		/// does not belong to the Multi-Pak it will determine which cartridge slot it belongs
		/// to, adjust the item id relative to the base of its menu ids, and forward it to
		/// the cartridge in that slot.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated the behavior is undefined.
		/// 
		/// @param menu_item_id The identifier of the menu item.
		void menu_item_clicked(menu_item_id_type menu_item_id) override;

		/// @brief Get the list of menu items for the Multi-Pak and all inserted cartridges.
		/// 
		/// This override retrieves the menu items from each of the inserted cartridges and
		/// concatenates them into a single menu with the Multi-Pak menu items added last.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated the behavior is undefined.
		/// 
		/// @return A collection of menu items for this cartridge.
		[[nodiscard]] menu_item_collection_type get_menu_items() const override;


	private:

		/// @brief Execute a Multi-Pak menu command.
		/// 
		/// Execute a menu command owned by the Multi-Pak cartridge.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated the behavior is undefined.
		/// 
		/// @param menu_item_id The identifier of the Multi-Pak menu item.
		void multipak_menu_item_clicked(menu_item_id_type menu_item_id);

		/// @brief Switch to a specific slot.
		/// 
		/// Switches the active/startup slot of the Multi-Pak to a specific slot index.
		/// 
		/// @param slot The slot index to switch to.
		/// @param reset If `true` the cartridge will request the system perform a
		/// hard reset one the switch is complete.
		void switch_to_slot(slot_id_type slot, bool reset);

		/// @brief Select and insert a cartridge.
		/// 
		/// Select a cartridge from a file and insert it into a specific slot.
		/// 
		/// @param slot The slot to insert the cartridge into.
		void select_and_insert_cartridge(slot_id_type slot);

		/// @brief Load and insert a cartridge.
		/// 
		/// Loads a cartridge by filename and insert it into a specific slot.
		/// 
		/// @param slot The slot to insert the cartridge into.
		/// @param filename The name of the cartridge file to load.
		/// @param update_settings It `true` the settings for that slot will be updated with
		/// the filename of the new cartridge.
		/// @param allow_reset If `true` and the value in `slot` is the same as the
		/// currently CTS or SCS slot selected via the MPI's control register, the
		/// cartridge will request the system perform a hard reset.
		/// 
		/// @return The status of the insert operation.
		[[nodiscard]] mount_status_type insert_cartridge(
			slot_id_type slot,
			const path_type& filename,
			bool update_settings,
			bool allow_reset);

		/// @brief Insert a cartridge from a slot.
		/// 
		/// Ejects a cartridge from a slot and optionally performs a hard reset of the
		/// of the system.
		/// 
		/// @param slot The slot containing the cartridge to eject.
		/// @param update_settings It `true` the settings for that slot will be updated
		/// to indicate no cartridge should be inserted in that slot the next time the
		/// emulator starts.
		/// @param allow_reset If `true` and the value in `slot` is the same as the
		/// currently CTS or SCS slot selected via the MPI's control register, the
		/// cartridge will request the system perform a hard reset.
		void eject_cartridge(slot_id_type slot, bool update_settings, bool allow_reset);


	private:

		/// @brief Defines the menu item ids for all operations that can be performed
		/// on a single slot.
		struct slot_action_command_descriptor
		{
			/// @brief Select the slot as the active/startup slot.
			menu_item_id_type select;
			/// @brief Select the slot as the active/startup slot.
			menu_item_id_type select_and_reset;
			/// @brief Insert a cartridge into the slot.
			menu_item_id_type insert;
			/// @brief Eject a cartridge from a slot.
			menu_item_id_type eject;
		};

		/// @brief Defines the identifiers for menu options.
		/// 
		/// Defines the identifiers for the menu options available to the user in the
		/// emulator's cartridge menu. Identifiers with the following prefixes must
		/// be ordered sequentially relative to the number at the end of the identifier.
		/// 
		/// - select_slot_
		/// - insert_into_slot_
		/// - eject_slot_
		struct menu_item_ids
		{
			/// @brief Requests the Becker Port Cartridge open its settings menu.
			static const menu_item_id_type open_settings = 1;

			/// @brief Select slot 1 as the active/startup slot.
			static const menu_item_id_type select_slot_1 = 2;
			/// @brief Select slot 2 as the active/startup slot.
			static const menu_item_id_type select_slot_2 = 3;
			/// @brief Select slot 3 as the active/startup slot.
			static const menu_item_id_type select_slot_3 = 4;
			/// @brief Select slot 4 as the active/startup slot.
			static const menu_item_id_type select_slot_4 = 5;

			/// @brief Select slot 1 as the active/startup slot.
			static const menu_item_id_type select_slot_1_and_reset = 6;
			/// @brief Select slot 2 as the active/startup slot.
			static const menu_item_id_type select_slot_2_and_reset = 7;
			/// @brief Select slot 3 as the active/startup slot.
			static const menu_item_id_type select_slot_3_and_reset = 8;
			/// @brief Select slot 4 as the active/startup slot.
			static const menu_item_id_type select_slot_4_and_reset = 9;

			/// @brief Insert cartridge into slot 1
			static const menu_item_id_type insert_into_slot_1 = 10;
			/// @brief Insert cartridge into slot 2
			static const menu_item_id_type insert_into_slot_2 = 11;
			/// @brief Insert cartridge into slot 3
			static const menu_item_id_type insert_into_slot_3 = 12;
			/// @brief Insert cartridge into slot 4
			static const menu_item_id_type insert_into_slot_4 = 13;
			/// @brief Eject cartridge from slot 1
			static const menu_item_id_type eject_slot_1 = 14;
			/// @brief Eject cartridge from slot 2
			static const menu_item_id_type eject_slot_2 = 15;
			/// @brief Eject cartridge from slot 3
			static const menu_item_id_type eject_slot_3 = 16;
			/// @brief Eject cartridge from slot 4
			static const menu_item_id_type eject_slot_4 = 17;
		};

		/// @brief Defines details for managing the menu items of inserted cartridges.
		struct cartridges_menu
		{
			/// @brief The first menu id representing a menu item for an inserted cartridge.
			static constexpr auto first_item_id = 200u;
			/// @brief The number of menu ids allocated for each inserted cartridge.
			static constexpr auto item_count = 100u;
		};

		/// @brief The total number of slots in the Multi-Pak.
		static constexpr auto total_slot_count = multipak_cartridge_driver::total_slot_count;

		/// @brief Collection of menu item identifiers of commands for each of the supported
		/// slots.
		static const std::array<slot_action_command_descriptor, total_slot_count> slot_action_command_ids_;

		/// @brief The expansion port host.
		const std::shared_ptr<expansion_port_host_type> host_;
		/// @brief The expansion port UI service provider.
		const std::shared_ptr<expansion_port_ui_type> ui_;
		/// @brief The expansion port bus.
		const std::shared_ptr<expansion_port_bus_type> bus_;
		/// @brief The driver emulating the Multi_pak hardware.
		const std::shared_ptr<multipak_cartridge_driver> driver_;
		/// @brief The Multi-Pak configuration.
		const std::shared_ptr<multipak_configuration> configuration_;
		/// @brief The handle to the module instance containing the cartridges resources.
		const HINSTANCE module_instance_;
		/// @brief The settings dialog.
		configuration_dialog settings_dialog_;
		/// @brief Collection of all cartridges inserted into the Multi-Pak.
		std::array<cartridge_ptr_type, multipak_cartridge_driver::total_slot_count> cartridges_;
	};

}
