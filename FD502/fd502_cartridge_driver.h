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
#include "vcc/devices/rom/rom_image.h"
#include <vcc/devices/rtc/oki_m6242b.h>
#include <vcc/devices/serial/beckerport.h>
#include "vcc/bus/cartridge_driver.h"
#include "vcc/bus/expansion_port_bus.h"
#include "vcc/bus/expansion_port_ui.h"
#include "vcc/bus/expansion_port_host.h"
#include <memory>
#include "wd1793.h"


namespace vcc::cartridges::fd502
{

	class fd502_cartridge_driver : public ::vcc::bus::cartridge_driver
	{
	public:

		/// @brief Type alias for variable length strings.
		using string_type = std::string;
		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;
		/// @brief Type alias for the component providing global system services to the
		/// cartridge plugin.
		using expansion_port_host_type = ::vcc::bus::expansion_port_host;
		/// @brief Type alias for the component acting as the expansion bus the cartridge plugin
		/// is connected to.
		using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
		/// @brief Type alias for the image type for Disk BASIC ROMs.
		using rom_image_type = ::vcc::devices::rom::rom_image;
		/// @brief Type alias for owned pointers to rom images.
		using rom_image_ptr_type = std::unique_ptr<rom_image_type>;
		/// @brief Type alias for drive identifiers.
		using drive_id_type = std::size_t;
		/// @brief Type alias for the diak images used by the driver.
		using disk_image_type = ::vcc::media::disk_image;
		/// @brief Type alias for the Becker Port device.
		using becker_port_device_type = ::vcc::devices::serial::Becker;
		/// @brief Type alias for the Real Time Clock (RTC) device.
		using real_time_clock_device_type = ::vcc::devices::rtc::oki_m6242b;
		/// @brief Type alias for drive head identifiers.
		using head_id_type = std::size_t;


	public:

		/// @brief Construct the FD502 Cartridge Driver
		/// 
		/// @param host A pointer to the host services interface.
		/// @param bus A pointer to the bus interface.
		/// 
		/// @throws std::invalid_argument if `host` is null.
		/// @throws std::invalid_argument if `bus` is null.
		fd502_cartridge_driver(
			std::shared_ptr<expansion_port_host_type> host,
			std::shared_ptr<expansion_port_bus_type> bus);

		/// @brief Starts the driver.
		/// 
		/// Configures and starts the FD502 cartridge driver and the WD179x, Real Time Clock,
		/// and Becker Port devices.
		/// 
		/// @param rom A pointer to the Disk BASIC ROM image to use.
		/// @param enable_turbo Specifies if the drive timing should be _turbo charged_. If
		/// this value is `true` the drive spin-up and other timing will be accelerated;
		/// otherwise the timing will be emulated as accurately as possible.
		/// @param enable_rtc Specifies if the Real Time Clock device should be enabled.
		/// @param enable_becker_port Specifies if the Becker Port device should be enabled.
		/// @param becker_server_address Specifies the address of the server the Becker Port
		/// device will connect to.
		/// @param becker_server_port Specifies the connection port of the server the Becker
		/// Port device will connect to.
		/// 
		/// @throws std::invalid_argument if `rom` is null.
		void start(
			rom_image_ptr_type rom,
			bool enable_turbo,
			bool enable_rtc,
			bool enable_becker_port,
			const string_type& becker_server_address,
			const string_type& becker_server_port);

		/// @brief Stops the FD502 Cartridge Driver and all managed devices.
		void stop();

		/// @brief Updates the states of managed devices.
		/// 
		/// @param delta The time delta since the last call to `update`.
		void update(float delta) override;

		/// @@inheitdoc
		unsigned char read_memory_byte(size_type memory_address) override;

		/// @@inheitdoc
		void write_port(unsigned char port_id, unsigned char value) override;

		/// @@inheitdoc
		unsigned char read_port(unsigned char port_id) override;

		/// @brief Insert a disk image into a specific drive.
		/// 
		/// @todo maybe just copydoc from the wd179x type
		/// 
		/// @param drive_id The identifier of the drive to insert the disk image into.
		/// @param disk_image The disk image to insert.
		/// @param file_path The path to the disk image file.
		void insert_disk(
			drive_id_type drive_id,
			std::unique_ptr<disk_image_type> disk_image,
			path_type file_path);

		/// @brief Eject a disk image from a specific drive.
		/// 
		/// @todo maybe just copydoc from the wd179x type
		/// 
		/// @param drive_id The identifier of the drive to eject the disk image from.
		void eject_disk(drive_id_type drive_id);

		/// @brief Determines if a disk image in a specific drive is write protected.
		/// 
		/// @todo maybe just copydoc from the wd179x type
		/// 
		/// @param drive_id The identifier of the drive containing the disk image to check.
		/// 
		/// @return `true` if the disk image is write protected; `false` otherwise.
		bool is_disk_write_protected(drive_id_type drive_id) const;

		/// @brief Retrieve the filename of a disk inserted into a specific drive.
		/// 
		/// @todo this should return an empty std::optional if there is no disk in the drive.
		/// @todo maybe just copydoc from the wd179x type
		/// 
		/// @param drive_id The identifier of the drive to retrieve the filename of the disk
		/// image currently inserted in the drive.
		/// 
		/// @return The path to the disk image inserted in the drive or an empty path if no
		/// disk image is inserted.
		path_type get_mounted_disk_filename(drive_id_type drive_id) const;

		/// @brief Sets the Disk BASIC ROM image used by the driver.
		/// 
		/// @param rom A pointer to the ROM image to use. This argument cannot be null.
		/// 
		/// @throws std::invalid_argument if `rom` is null.
		void set_rom(rom_image_ptr_type rom);

		/// @brief Enables or disables the acceleration of disk timing.
		/// 
		/// @todo maybe just copydoc from the wd179x type
		/// 
		/// @param enable_turbo Specifies if the drive timing should be _turbo charged_. If
		/// this value is `true` the drive spin-up and other timing will be accelerated;
		/// otherwise the timing will be emulated as accurately as possible.
		void set_turbo_mode(bool enable_turbo);

		/// @brief Enables or disables the Real Time Clock device.
		/// 
		/// @param enable_rtc Specifies if the Real Time Clock device should be enabled.
		void enable_rtc_device(bool enable_rtc);

		/// @brief Updates the Becker Port settings.
		/// 
		/// @todo better name!
		/// 
		/// @param enable_becker_port Specifies if the Becker Port device should be enabled.
		/// @param becker_server_address Specifies the address of the server the Becker Port
		/// device will connect to.
		/// @param becker_server_port Specifies the connection port of the server the Becker
		/// Port device will connect to.
		void update_becker_port_settings(
			bool enable_becker_port,
			const string_type& becker_server_address,
			const string_type& becker_server_port);

		/// @brief Retrieve the number of disk drives supported.
		/// 
		/// @return The total number of disk drives supported.
		size_type drive_count() const;

		/// @copydoc wd1793_device::is_motor_running
		bool is_motor_running() const noexcept;
		/// @copydoc wd1793_device::get_selected_drive_id
		std::optional<drive_id_type> get_selected_drive_id() const noexcept;
		/// @copydoc wd1793_device::get_selected_head
		head_id_type get_selected_head() const noexcept;
		/// @copydoc wd1793_device::get_head_position
		size_type get_head_position() const noexcept;
		/// @copydoc wd1793_device::get_current_sector
		size_type get_current_sector() const noexcept;

	private:

		/// @brief Defines the memory mapped I/O ports used by the WD179x controller.
		struct mmio_ports
		{
			/// @brief The control register.
			/// 
			/// The Control Register is used for selecting drives, heads, and configuring the
			/// controller.
			static constexpr auto control_register = 0x40u;
			/// @brief Command register.
			/// 
			/// The Command Register is used to execute commands on the controller. This
			/// register is write only and shares a port with the status register.
			static constexpr auto command_register = 0x48u;
			/// @brief Status register.
			/// 
			/// The Status Register provides operational status and result flags to the host
			/// system. This register is read only and shares a port with the command register.
			static constexpr auto status_register = 0x48u;
			/// @brief Track register.
			/// 
			/// The Track Register is used to instruct the controller which track to operate
			/// on and to communicate track information back to the host system.
			static constexpr auto track_register = 0x49u;
			/// @brief Sector register.
			/// 
			/// The Sector Register is used to instruct the controller which sector to operate
			/// on and to communicate sector information back to the host system.
			static constexpr auto sector_register = 0x4au;
			/// @brief Data Register.
			/// 
			/// The Data Register is used for transferring data between the host system and
			/// the controller when reading and writing sectors, sector headers, and tracks.
			static constexpr auto data_register = 0x4bu;
		};


	private:

		/// @brief The expansion port host.
		const std::shared_ptr<expansion_port_host_type> host_;
		/// @brief The expansion port bus.
		const std::shared_ptr<expansion_port_bus_type> bus_;
		/// @brief The Disk BASIC ROM image.
		rom_image_ptr_type rom_;
		/// @brief The WD179x device.
		wd1793_device wd1793_;
		/// @brief The Disto Real Time Clock device.
		real_time_clock_device_type disto_rtc_;
		/// @brief The Becker Port serial device.
		becker_port_device_type becker_port_;
	};

}
