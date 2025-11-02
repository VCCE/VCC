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
#include "wd1793defs.h"
#include "fd502_control_register.h"
#include "fdc_register_file.h"
#include "fdc_command.h"
#include "vcc/bus/expansion_port_bus.h"
#include <vcc/peripherals/disk_drives/generic_disk_drive.h>
#include <Windows.h>
#include <array>
#include <string>
#include <optional>


namespace vcc::cartridges::fd502
{

	class wd1793_device
	{
	public:

		/// @brief Type alias to lengths, 1 dimension sizes, and indexes.
		using size_type = std::size_t;
		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;
		/// @brief Type alias for drive identifiers.
		using drive_id_type = std::size_t;
		/// @brief Type alias for drive head identifiers.
		using head_id_type = std::size_t;
		/// @brief Type alias for the disk drive device used by the controller.
		/// @todo this should really be vcc::peripherals::disk_drive
		using disk_drive_device_type = ::vcc::peripherals::disk_drives::generic_disk_drive;
		/// @brief Type alias for the diak images used by the controller.
		using disk_image_type = ::vcc::media::disk_image;
		/// @brief The type alias for the sector size identifier.
		using sector_size_id_type = unsigned char;
		/// @brief Type alias for the command identifier.
		using command_id_type = ::vcc::cartridges::fd502::detail::fdc_command_id;
		/// @brief Type alias for the direction of a track step command.
		using step_direction_type = ::vcc::peripherals::step_direction;
		/// @brief Type alias for tick counters.
		using tick_type = std::size_t;


	public:

		/// @brief Construct the WD179x device.
		/// 
		/// @param bus A pointer to the bus interface.
		/// 
		/// @throws std::invalid_argument if `bus` is null.
		explicit wd1793_device(std::shared_ptr<::vcc::bus::expansion_port_bus> bus);

		/// @brief Starts the device.
		///
		/// @todo this should do something. Reset the state maybe?
		void start();

		/// @brief Stops the device.
		///
		/// When called all disk images will be ejected and the device will be reset to a
		/// default state.
		void stop();

		/// @brief Write a value to the FD502 Control Register.
		/// 
		/// Writes a value to the FD502 Control Register and decodes its bitflags into local
		/// state variables.
		/// 
		/// @todo Move this into the FD502 driver and expose the controls lines via accessors
		/// and mutators.
		/// 
		/// @param value The value to write.
		void write_control_register(unsigned char value);

		/// @brief Writes a value to the Command Register.
		/// 
		/// Writes a value to the Command Register and begins execution of the command it
		/// specifies. This register should not be loaded when the device is busy unless the
		/// new command is a force interrupt. The command register can be loaded from the
		/// DAL, but not read onto the DAL.
		/// 
		/// @param value The value to write. This value contains the command to execute and
		/// depending on the command additional arguments that affect how the command is
		/// executed.
		void write_command_register(unsigned char value);

		/// @brief Writes a value to the Track Register.
		/// 
		/// Writes a value to the Track Register. The contents of this register are compared
		/// with the recorded track number in the ID field of the sector record during disk
		/// Read, Write and Verify operations. The Track Register can be loaded from or
		/// transferred to the DAL. This Register should not be loaded when the device is
		/// busy.
		/// 
		/// @param value The value to write.
		void write_track_register(unsigned char value);

		/// @brief Writes a value to the Sector Register.
		/// 
		/// Writes a value to the Sector Register. The contents of the register are compared
		/// with the recorded sector number in the ID field of the sector record during disk
		/// Read or Write operations. The Sector Register contents can be loaded from or
		/// transferred to the DAL. This register should not be loaded when the device is
		/// busy.
		/// 
		/// @param value The value to write.
		void write_sector_register(unsigned char value);

		/// @brief Writes a value to the Data Register.
		/// 
		/// Writes a value to the Data Register. This 8-bit register is used as a holding
		/// register during Write operations. In Disk Write operations information is
		/// transferred in parallel from the Data Register to the Data Shift Register.
		/// 
		/// When executing the Seek command the Data Register holds the address of the desired
		/// Track position. This register is loaded from the DAL and gated onto the DAL under
		/// processor control.
		/// 
		/// @param value The value to write.
		void write_data_register(unsigned char value);

		/// @brief Reads the value in the Status Register.
		/// 
		/// Reads the value in the Status Register. This 8-bit register holds device Status
		/// information. The meaning of the Status bits is a function of the type of command
		/// previously executed. This register can be read onto the DAL, but not loaded from the
		/// DAL.
		/// 
		/// @return The value in the Status Register.
		unsigned char read_status_register() const;

		/// @brief Reads the value in the Track Register.
		/// 
		/// Reads the value in the Track Register.
		/// 
		/// @return The value in the Track Register.
		unsigned char read_track_register() const;

		/// @brief Reads the value in the Sector Register.
		/// 
		/// Reads the value in the Sector Register.
		/// 
		/// @return The value in the Sector Register.
		unsigned char read_sector_register() const;

		/// @brief Reads the value in the Data Register.
		/// 
		/// Reads the value in the Data Register. This 8-bit register is used as a holding
		/// register during Disk Read and Write operations. In Disk Read operations the
		/// assembled data byte is transferred in parallel to the Data Register from the Data
		/// Shift Register. When executing the Seek command the Data Register holds the
		/// address of the desired Track position.
		/// 
		/// @return The value in the Data Register.
		unsigned char read_data_register();

		/// @brief Updates the state of the WD179x device.
		/// 
		/// Updates the state of the WD179x device including the index pulse timers and the
		/// delays used to simulate the timing of commands.
		/// 
		/// @param delta The amount of time that has elapsed since the function was last
		/// called.
		void update(float delta);

		/// @brief Insert a disk image into a specific drive.
		/// 
		/// @param drive_id The identifier of the drive to insert the disk image into.
		/// @param disk_image The disk image to insert.
		/// @param file_path The path to the disk image file. @todo The WD179x should not
		/// be managing this.
		void insert_disk(
			drive_id_type drive_id,
			std::unique_ptr<disk_image_type> disk_image,
			path_type file_path);

		/// @brief Eject a disk image from a specific drive.
		/// 
		/// @param drive_id The identifier of the drive to eject the disk image from.
		void eject_disk(drive_id_type drive_id);

		/// @brief Determines if a disk image in a specific drive is write protected.
		/// 
		/// @param drive_id The identifier of the drive containing the disk image to check.
		/// 
		/// @return `true` if the disk image is write protected; `false` otherwise.
		bool is_disk_write_protected(drive_id_type drive_id) const;

		/// @brief Retrieve the filename of a disk inserted into a specific drive.
		/// 
		/// @todo this should return an empty std::optional if there is no disk in the drive.
		/// 
		/// @param drive_id The identifier of the drive to retrieve the filename of the disk
		/// image currently inserted in the drive.
		/// 
		/// @return The path to the disk image inserted in the drive or an empty path if no
		/// disk image is inserted.
		path_type get_mounted_disk_filename(drive_id_type drive_id) const;

		/// @brief Enables or disables the acceleration of disk timing.
		/// 
		/// @param enable_turbo Specifies if the drive timing should be _turbo charged_. If
		/// this value is `true` the drive spin-up and other timing will be accelerated;
		/// otherwise the timing will be emulated as accurately as possible.
		void set_turbo_mode(bool enable_turbo);

		/// @brief Retrieve the number of disk drives supported.
		/// 
		/// @return The number of disk drives supported.
		size_type drive_count() const;

		bool is_motor_running() const noexcept;
		std::optional<drive_id_type> get_selected_drive_id() const noexcept;
		size_type get_head_position() const noexcept;
		size_type get_selected_head() const noexcept;
		size_type get_current_sector() const noexcept;


	private:

		/// @brief Read data from the data register.
		/// 
		/// This function is used as a callback for the current command and returns the
		/// contents of the Data Register.
		/// 
		/// @return The contents of the Data Register.
		unsigned char read_byte_from_data_register();

		/// @brief Read the next byte from a disk track.
		/// 
		/// This function is used as a callback for the Read Track command and reads the next
		/// byte of raw track data from the disk.
		/// 
		/// @note This functionality is not implemented and always issues a Record Not Found
		/// error.
		/// 
		/// @todo Implement functionality.
		/// 
		/// @return The next byte of track data.
		unsigned char read_byte_from_track();

		/// @brief Read the next byte from a disk sector.
		/// 
		/// This function is used as a callback for the Read Sector commands and reads the
		/// next byte of sector data from the disk.
		/// 
		/// @todo add details of behavior once a little more refactoring is done to add more
		/// states to the commands.
		/// @todo copydoc the details specific for the read commands.
		/// 
		/// @return The next byte of sector data.
		unsigned char read_byte_from_sector();

		/// @brief Read the next byte from the next sector record.
		/// 
		/// This function is used as a callback for the Read Address command and reads the
		/// next byte from the sector record of the next sector. When called for the first
		/// time for a command, the function will retrieve the necessary information for
		/// the Sector Record and build a data stream in the expected format.
		/// 
		/// @todo copydoc the details specific for the read address command.
		/// 
		/// @return The next byte of sector header data.
		unsigned char read_byte_from_address();

		/// @brief Write data to the Data Register.
		/// 
		/// This function is used as a callback for the current command and writes a specified
		/// value to the Data Register.
		/// 
		/// @param value The value to write.
		void write_byte_to_data_register(unsigned char value);

		/// @brief Write data to the current _working_ sector.
		/// 
		/// This function is used as a callback for Write Sector commands and writes a
		/// specified byte to the _working_ sector selected for the command.
		/// 
		/// @todo copydoc the details specific for the write sector commands.
		/// 
		/// @param value The value to write to the sector.
		void write_byte_to_sector(unsigned char value);

		/// @brief Write data to the current _working_ track.
		/// 
		/// This function is used as a callback for Write Track command and writes a
		/// specified byte to the _working_ sector selected for the command.
		/// 
		/// @todo copydoc the details specific for the write sector commands.
		/// 
		/// @param value The value to write to the track.
		void write_byte_to_track(unsigned char value);

		/// @brief Writes a block of raw data to a track.
		/// 
		/// This function takes a block of raw track data containing WD179x control bytes and
		/// structured as a formatted track, converts it to an intermediate form, and passes
		/// it to the disk image. If the disk image accepts the formatting the write is
		/// completed successfully. In the event that the disk image does not accept the
		/// formatting due to unsupported features like out of the ordinary identifiers in
		/// the sector header or sector sizes and the write is aborted.
		/// 
		/// @todo `data_buffer` should be a reference to a vector or some type of view.
		/// 
		/// @param disk_drive The drive containing the disk image to write the track data to.
		/// @param drive_head The head (side) of the disk image to write the track data to.
		/// @param data_buffer The data buffer containing the data.
		/// @param data_buffer_size The size of the data buffer.
		/// 
		/// @return `true` if the track data was successfully written; `false` otherwise.
		bool write_raw_track(
			const disk_drive_device_type& disk_drive,
			head_id_type drive_head,
			const unsigned char* data_buffer,
			size_type data_buffer_size) const;

		/// @brief Executes a command written to the Command Register.
		/// 
		/// @param command_packet The command packet bitfield including the command
		/// identifier and arguments/attributes that further define the behavior of the
		/// command.
		void execute_command(unsigned char command_packet);

		/// @brief Completes the current Command.
		/// 
		/// This function completes the current command by setting the status register,
		/// issuing an interrupt if enabled, and resetting the command state of the WD179x
		/// device.
		/// 
		/// @param status_flags Additional status flags to set.
		void complete_command(std::optional<unsigned char> status_flags);

		/// @brief Retrieve the size of a sector.
		/// 
		/// Retrieves the size in bytes of a sector from a sector size identifier specific to
		/// the WD179x.
		/// 
		/// @todo add translation table
		/// 
		/// @param id The identifier to convert to a sector size.
		/// 
		/// @return The size of the sector.
		size_type get_sector_size_from_id(sector_size_id_type id) const;

		/// @brief Retrieve the identifier of a sector size.
		/// 
		/// Retrieves an identifier unique to the WD179x that indicates the size of a sector.
		/// 
		/// @todo add translation table
		/// 
		/// @param size The size in bytes of the sector.
		/// 
		/// @return The unique identifier of the sector size.
		sector_size_id_type get_id_from_sector_size(size_type size) const;

		/// @brief Retrieve the disk drive selected through the Control Register.
		/// 
		/// @return A reference to the currently selected disk drive. 
		disk_drive_device_type& get_selected_drive();

		/// @brief Retrieve the disk drive selected through the Control Register.
		/// 
		/// @return A reference to the currently selected disk drive. 
		const disk_drive_device_type& get_selected_drive() const;


	private:

		/// @brief Type alias for the function called to read data from the data register.
		using fetch_data_byte_function_type = unsigned char (wd1793_device::*)();
		/// @brief Type alias for the function called to write data to the data register.
		using write_data_byte_function_type = void (wd1793_device::*)(unsigned char);
		/// @brief Type alias for the register file.
		using fdc_register_file_type = ::vcc::cartridges::fd502::detail::fdc_register_file;
		/// @brief Type alias for the FDC command container.
		using fdc_command_type = ::vcc::cartridges::fd502::detail::fdc_command;

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

		/// @brief The minimum size in bytes of a sector.
		static constexpr size_type minimum_sector_size_ = 128;
		/// @brief The number of ticks per second.
		///
		/// This value is based on the NTSC horizontal sync signal.
		/// @todo This should go away once everything is converted to be based on time deltas
		/// instead of ticks.
		static constexpr auto system_update_tick_frequency_ = 60u * 262u;
		/// @brief The maximum number of extra ticks to wait until the index pulse begins.
		static constexpr auto max_index_pulse_wobble_tick_count_ = 64u;
		/// @brief The number of ticks the index pulse should be active.
		static constexpr auto index_pulse_duration_tick_count_ = 30u;
		/// @brief The number of times the disk completes a single rotation every minute of
		/// continuous spinning.
		static constexpr auto disk_rotations_per_minute_ = 300u;
		/// @brief The number of times the disk completes a single rotation every second of
		/// continuous spinning.
		static constexpr auto disk_rotations_per_second_ = disk_rotations_per_minute_ / 60u;
		/// @brief The number of ticks that occur during a single rotation of the disk.
		static constexpr auto ticks_per_disk_rotation_ = system_update_tick_frequency_ / disk_rotations_per_second_;

		/// @brief The expansion port bus.
		const std::shared_ptr<::vcc::bus::expansion_port_bus> bus_;
		/// @brief The pointer to a function called when data is read from the data register.
		fetch_data_byte_function_type fetch_data_byte_function_ = &wd1793_device::read_byte_from_data_register;
		/// @brief The pointer to a function called when data is written to the data register.
		write_data_byte_function_type write_data_byte_function_ = &wd1793_device::write_byte_to_data_register;

		/// @brief The placeholder disk drive that is used when no drive is selected via the
		/// FD502's Control Register.
		disk_drive_device_type placeholder_drive_;
		/// @brief The disk drives connected to the WD1793.
		std::array<disk_drive_device_type, 4> disk_drives_;

		/// @brief The FD502 Control Register.
		fd502_control_register control_register_;
		/// @brief The WD179x Registers
		fdc_register_file_type registers_;
		/// @brief The current executing command. If this value is empty no command is
		/// executing.
		std::optional<fdc_command_type> current_command_;

		/// @brief The direction the drive head will be moved during step, seek, and other
		/// similar commands.
		step_direction_type step_direction_ = step_direction_type::in;

		/// @brief The buffer used for read operations.
		/// @todo Combine this with the write buffer.
		std::vector<unsigned char> read_transfer_buffer_;
		/// @brief The current position in the read buffer during a read operation.
		/// @todo Combine this with the write buffer.
		std::vector<unsigned char>::size_type read_transfer_position_ = 0;

		/// @brief The buffer used for write operations.
		/// @todo Combine this with the read buffer.
		std::vector<unsigned char> write_transfer_buffer_;
		/// @brief The current position in the write buffer during a write operation.
		/// @todo Combine this with the read buffer.
		std::vector<unsigned char>::size_type write_transfer_position_ = 0;


		/// @brief Used for index pulsing.
		/// @todo replace ASAP with ns, ms, or us durations.
		tick_type index_pulse_tick_counter_ = 0;
		/// @brief Used for index pulsing.
		/// @todo replace ASAP with ns, ms, or us durations.
		tick_type index_pulse_begin_tick_count_ = 0;
		/// @brief Used for index pulsing.
		/// @todo replace ASAP with ns, ms, or us durations.
		tick_type index_pulse_end_tick_count_ = 0;

		/// @brief The number of ticks for a command to initialize and start.
		/// @todo verify this is what it's for.
		static constexpr tick_type default_update_ticks_for_command_settle_ = 10;
		/// @brief The number of ticks that occur per millisecond.
		/// @todo verify this is what it's for.
		static constexpr tick_type default_update_ticks_per_millisecond_ = 15;
		/// @brief Number of ticks to wait until an IO operation times out.
		/// @todo verify this is what it's for.
		static constexpr tick_type default_io_wait_tick_count_ = 5;


		/// @brief The number of ticks to delay before the current command _settles_ and
		/// begins executing.
		tick_type update_ticks_for_command_settle_ = default_update_ticks_for_command_settle_;
		/// @brief The number of update ticks that occur each millisecond.
		tick_type update_ticks_per_millisecond_ = default_update_ticks_per_millisecond_;
		/// @brief The amount of time to execute the current command before it completes.
		int ExecTimeWaiter = 0;
		/// @brief The accumulated tick count since the beginning of the current command used
		/// to detect timeout and data loss errors.
		tick_type IOWaiter = 0;

		/// @brief Indicates if the Index Pulse signal is active. This pulse is used to
		/// identify the position of the disk relative to its index hole during spin.
		bool index_pulse_active_ = false;

		/// @brief Status flag indicating if data loss occurred during a read or write
		/// operation, usually due to a timeout or inaction by the computer.
		bool data_loss_detected_ = false;

		/// @brief Specifies if disk spin-up and other timing should be accelerated.
		bool turbo_enabled_ = false;
	};

}
