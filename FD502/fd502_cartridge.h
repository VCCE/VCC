////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute it and/or
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
#include "fd502_cartridge_driver.h"
#include "fd502_configuration.h"
#include "configuration_dialog.h"
#include "vcc/devices/psg/sn76496.h"
#include "vcc/devices/rom/banked_rom_image.h"
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_port_bus.h"
#include "vcc/bus/expansion_port_ui.h"
#include "vcc/bus/expansion_port_host.h"


namespace vcc::cartridges::fd502
{

	class fd502_cartridge : public ::vcc::bus::cartridge
	{
	public:

		/// @brief Type alias for variable length strings.
		using string_type = std::string;
		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;
		/// @brief Type alias for the component providing global system services to the
		/// cartridge plugin.
		using expansion_port_host_type = ::vcc::bus::expansion_port_host;
		/// @brief Type alias for the component providing user interface services to the
		/// cartridge plugin.
		using expansion_port_ui_type = ::vcc::bus::expansion_port_ui;
		/// @brief Type alias for the component acting as the expansion bus the cartridge plugin
		/// is connected to.
		using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
		/// @brief Type alias for the component emulating the FD502 hardware.
		using driver_type = ::vcc::cartridges::fd502::fd502_cartridge_driver;
		/// @brief Type alias for the configuration provider.
		using configuration_type = ::vcc::cartridges::fd502::fd502_configuration;
		/// @brief Type alias for the configuration dialog.
		using configuration_dialog_type = ::vcc::cartridges::fd502::configuration_dialog;
		/// @brief Type alias for drive identifiers.
		using drive_id_type = std::size_t;
		/// @brief Type alias for the image type for Disk BASIC ROMs.
		using rom_image_type = ::vcc::devices::rom::rom_image;
		/// @brief Type alias for Disk BASIC ROM identifiers.
		using rom_image_id_type = ::vcc::cartridges::fd502::detail::disk_basic_rom_image_id;


	public:

		/// @brief Construct the FD502 Cartridge.
		/// 
		/// @param host A pointer to the host services interface.
		/// @param ui A pointer to the UI services interface.
		/// @param bus A pointer to the bus interface.
		/// @param driver A pointer to the driver implementing the FD502 hardware.
		/// @param configuration A pointer to the configuration provider.
		/// @param module_instance A handle to the instance of the module containing the
		/// resources for the FD502 plugin.
		/// 
		/// @throws std::invalid_argument if `host` is null.
		/// @throws std::invalid_argument if `ui` is null.
		/// @throws std::invalid_argument if `bus` is null.
		/// @throws std::invalid_argument if `driver` is null.
		/// @throws std::invalid_argument if `configuration` is null.
		/// @throws std::invalid_argument if `module_instance` is null.
		fd502_cartridge(
			std::shared_ptr<expansion_port_host_type> host,
			std::unique_ptr<expansion_port_ui_type> ui,
			std::shared_ptr<expansion_port_bus_type> bus,
			std::unique_ptr<driver_type> driver,
			std::unique_ptr<configuration_type> configuration,
			HINSTANCE module_instance);

		/// @inheritdoc
		name_type name() const override;

		/// @inheritdoc
		[[nodiscard]] driver_type& driver() override;

		/// @brief Starts the cartridge.
		/// 
		/// Starts the cartridge, initializes the cartridge driver, and mounts all configured
		/// disk images.
		void start() override;

		/// @inheritdoc
		void stop() override;

		/// @brief Retrieves a human readable status text of the cartridge plugin.
		/// 
		/// @todo This is currently stripped of most device status details and will need to
		/// be expanded when the cart is finished.
		/// 
		/// @param status_buffer The buffer to put the status text in.
		/// @param buffer_size The size of the status buffer.
		void status(char* status_buffer, size_type buffer_size) override;

		/// @brief Builds the menu for the cartridge features.
		/// 
		/// @todo add a few more details once the function is split
		/// 
		/// @return A collection of menu items.
		menu_item_collection_type get_menu_items() const override;

		/// @inheritdoc
		void menu_item_clicked(menu_item_id_type menu_item_id) override;


	private:

		/// @brief Prompts the user to select a disk image and inserts it into a virtual
		/// drive.
		/// 
		/// Opens a modal file selection window and prompts the user to select a disk image.
		/// If the user selects a disk image (i.e. presses OK) and the disk image exists, the
		/// function will attempt to load it and insert it into a virtual disk. If the file
		/// specified by the user does not exist the function will open a modal dialog
		/// allowing the user the option of creating a new disk image.
		/// 
		/// @param drive_id The identifier of the drive to insert the disk image into.
		void insert_disk(drive_id_type drive_id);

		/// @brief Loads and inserts a disk into a specific drive.
		/// 
		/// Attempts to load a disk image and inserts it into the specified drive.
		/// 
		/// @todo this needs to return an error code or report an actual error message.
		/// 
		/// @param drive_id The identifier of the drive to insert the disk image into.
		/// @param disk_image_path The path to the disk image to load and insert.
		void insert_disk(drive_id_type drive_id, const path_type& disk_image_path);

		/// @brief Attempts to insert the next disk of a series into a specific drive.
		/// 
		/// Attempts to insert the next disk of a series into a specific drive based on
		/// the filename of the disk that is currently inserted.
		/// 
		/// @param drive_id The drive to insert the next disk into.
		void insert_next_disk(drive_id_type drive_id);

		/// @brief Ejects a disk from a virtual drive.
		/// 
		/// @param drive_id The identifier of the drive containing the disk image to eject.
		void eject_disk(drive_id_type drive_id);

		/// @brief Loads a Disk BASIC ROM image.
		/// 
		/// @param rom_id The identifier of the ROM image to load.
		/// 
		/// @return If the ROM was loaded successfully, a pointer to the loaded ROM image;
		/// `nullptr` otherwise.
		std::unique_ptr<rom_image_type> load_rom(rom_image_id_type rom_id) const;

		/// @brief Retrieve the path to the next disk image of a series.
		/// 
		/// Attempts to determine the path to the next disk image of a series that disk
		/// currently inserted in the drive is a part of. This function uses the last
		/// character of the filename to determine which in the series the current disk
		/// is and checks if a disk with the same filename but ends a number one digit
		/// higher.
		/// 
		/// @param drive_index The index of the drive to check.
		/// 
		/// @return If the current disk is part of a series and the next disk in the
		/// series can be found this function returns the path to the next disk image;
		/// otherwise an empty path is returned.
		path_type get_next_disk_image_for_drive(drive_id_type drive_index) const;

		/// @brief Generates a menu item string for the path of the next disk in a series.
		/// 
		/// @param drive_index The identifier of the drive the disk will be inserted into.
		/// @param path The path to the disk image.
		/// 
		/// @return A string containing the menu text.
		string_type format_next_disk_image_menu_text(
			drive_id_type drive_index,
			const std::filesystem::path& path) const;

		/// @brief Called when the configuration dialog changes the settings.
		///
		/// Called when the configuration dialog changes the cartridge settings. When called,
		/// this function will enable, disable, and reconfigure the FD502, WD179x, and other
		/// supported devices based on the updated settings.
		void settings_changed();


	private:

		/// @brief The expansion port host.
		const std::shared_ptr<expansion_port_host_type> host_;
		/// @brief The expansion port UI service provider.
		const std::unique_ptr<expansion_port_ui_type> ui_;
		/// @brief The expansion port bus.
		const std::shared_ptr<expansion_port_bus_type> bus_;
		/// @brief The driver emulating the Multi_pak hardware.
		const std::shared_ptr<driver_type> driver_;
		/// @brief The FD502 configuration provider.
		const std::shared_ptr<configuration_type> configuration_;
		/// @brief The handle to the module instance containing the cartridges resources.
		const HINSTANCE module_instance_;
		/// @brief The settings dialog.
		configuration_dialog_type settings_dialog_;
	};

}
