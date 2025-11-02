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
#include "disk_basic_rom_image_id.h"
#include "vcc/utils/persistent_value_section_store.h"
#include <filesystem>
#include <string>
#include <map>


namespace vcc::cartridges::fd502
{

	/// @brief The FD502 Configuration manages various settings for the cartridge plugin.
	class fd502_configuration
	{
	public:

		/// @brief Type alias to lengths, 1 dimension sizes, and indexes.
		using size_type = std::size_t;
		/// @brief Defines the type used to hold a variable length string.
		using string_type = std::string;
		/// @brief Type alias of the value store used to serialize the configuration.
		using value_store_type = ::vcc::utils::persistent_value_section_store;
		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;
		/// @brief Type alias for drive identifiers.
		using drive_id_type = std::size_t;
		/// @brief Type alias for identifiers of built in Disk BASIC ROM image.
		using rom_image_id_type = ::vcc::cartridges::fd502::detail::disk_basic_rom_image_id;


	public:

		/// @brief Construct a FD502 Configuration instance.
		/// 
		/// @param path The path fo the configuration file
		/// @param section The section in the configuration file to store the values.
		fd502_configuration(path_type path, string_type section);

		/// @brief Enable or disable Turbo Mode.
		/// 
		/// Enable or disable turbo mode to reduce the timing of disk spin ups and other time
		/// consuming operations that are not necessary but may be desired in emulation.
		/// 
		/// @todo rename to set_timing_mode or something. change bool to an enum.
		/// 
		/// @param is_enabled `true` if Turbo Mode should be enabled; `false` otherwise.
		void set_turbo_mode(bool is_enabled);

		/// @brief Retrieve the Turbo Mode enable/disable settings.
		/// 
		/// @return `true` if Turbo Mode is enabled; `false` otherwise.
		bool is_turbo_mode_enabled() const;

		/// @brief Set the identifier of the built-in ROM image to use.
		/// 
		/// @param rom_id The identifier of the built-in ROM image to use.
		void set_rom_image_id(rom_image_id_type rom_id) const;

		/// @brief Retrieve the identifier of the built-in ROM image to use.
		/// 
		/// @return The identifier of the built-in ROM image to use.
		rom_image_id_type get_rom_image_id() const;

		/// @brief Sets the last directory a cartridge was loaded from.
		/// 
		/// Sets the last path accessed when successfully loading a cartridge.
		/// 
		/// @param path The new path to set.
		void set_rom_image_directory(const path_type& path);

		/// @brief Retrieves the last directory a cartridge was loaded from.
		/// 
		/// Retrieves the last path accessed when successfully loading a cartridge.
		/// 
		/// @return The last path accessed.
		[[nodiscard]] path_type rom_image_directory() const;

		/// @brief Set the path to the custom ROM image.
		/// 
		/// @param path The path to the custom ROM image.
		void set_rom_image_path(const path_type& path);

		/// @brief Retrieves the path to the custom ROM image.
		/// 
		/// @return The custom ROM image path.
		path_type rom_image_path() const;


		/// @brief Sets the directory containing the last disk image mounted.
		/// 
		/// @param path The directory containing the last disk image mounted.
		void set_disk_image_directory(path_type path);

		/// @brief Retrieve the directory containing the last disk image mounted.
		path_type disk_image_directory() const;

		/// @brief Enable or disable the loading and saving of mount settings.
		/// 
		/// @param is_enabled Specifies if loading and saving mount settings is enabled or
		/// disabled.
		void set_serialize_drive_mount_settings(bool is_enabled);

		/// @brief Retrieve the drive mount serialization settings.
		///
		/// @return `true` if the drive mount settings are loaded at start up and saved when
		/// a disk is inserted into or ejected from a drive. `false` otherwise.
		bool serialize_drive_mount_settings() const;

		/// @brief Set the path of the disk image for a specific drive.
		/// 
		/// @param drive_id The drive to set the path for.
		/// @param path The path to the disk image to insert into the drive specified in
		/// `drive` or an empty path if none is specified.
		void set_disk_image_path(drive_id_type drive_id, const path_type& path);

		/// @brief Retrieve the path to the disk image for a specific drive.
		/// 
		/// @param drive_id The drive to retrieve the disk image for.
		/// 
		/// @return The path to the disk image to insert into the drive specified in `drive`
		/// or an empty path if none is specified.
		path_type disk_image_path(drive_id_type drive_id) const;


		/// @brief Enables or disables the Real Time Clock.
		/// 
		/// @param is_enabled `true` if the Real Time Clock is enabled; `false` otherwise.
		void enable_rtc(bool is_enabled);

		/// @brief Retrieve the Real Time Clock enable/disable settings.
		/// 
		/// @return `true` if the Real Time Clock is enabled; `false` otherwise.
		bool is_rtc_enabled() const;


		/// @brief Enable or disable the Becker Port device.
		/// 
		/// @param is_enabled `true` if the Becker port device is enabled; `false` otherwise.
		void enable_becker_port(bool is_enabled) const;

		/// @brief Retrieves the enabled setting for the Becker Port device.
		/// 
		/// @return `true` if the Becker Port is enabled; `false` otherwise.
		bool is_becker_port_enabled() const;

		/// @brief Set the server address the Becker Port device will connect to.
		/// 
		/// @param address The address of the server to connect to.
		void becker_port_server_address(const string_type& address) const;

		/// @brief Get the server address the Becker Port device will connect to.
		/// 
		/// @return The server address the Becker Port device will connect to.
		string_type becker_port_server_address() const;

		/// @brief Set the server port the Becker Port device will connect to.
		/// 
		/// @param port The port number of the server to connect to.
		void becker_port_server_port(const string_type& port) const;
		/// @brief Get the server port the Becker Port device will connect to.
		/// 
		/// @return The server port the Becker Port device will connect to.
		string_type becker_port_server_port() const;


	private:

		/// @brief Retrieve the setting key for a specific drive id.
		/// 
		/// @param drive_id The drive identifier to get the key for.
		/// 
		/// @return A string containing the settings key for the drive specified in `drive_id`
		string_type get_drive_mount_path_key(drive_id_type drive_id) const;


	private:

		/// @brief Defines the section keys where settings are stored.
		struct keys
		{
			/// @brief Defines the section keys where disk basic ROM settings are stored.
			struct disk_basic
			{
				/// @brief The key where the Disk BASIC ROM identifier setting is stored.
				static const inline string_type rom_id = "disk-basic.rom-id";

				/// @brief The key where the custom ROM image directory is stored.
				static const inline string_type rom_directory = "disk-basic.rom-image-directory";

				/// @brief The key where the custom ROM image path is stored.
				static const inline string_type selected_rom_path = "disk-basic.selected-rom-image-path";
			};

			/// @brief Defines the section keys where disk drive settings are stored.
			struct drives
			{
				/// @brief The key where the enable Turbo Mode setting is stored.
				static const inline string_type turbo_mode = "drives.turbo.enable";

				/// @brief The key where the last directory containing mounted disk image is stored.
				static const inline string_type disk_image_directory = "drives.disk-images-directory";

				/// @brief Defines the section keys where settings for disk drive mount
				/// settings are stored.
				struct mount_paths
				{
					/// @brief The key where the save mount setting is stored.
					static const inline string_type serialize_mount_paths = "drives.mount-paths.serialize";
					/// @brief The format string used to generate
					static const inline string_type mount_path_fmt = "drives.mount-paths.drive{}";
				};
			};

			/// @brief Defines the section keys where settings for additional devices are
			/// stored.
			struct devices
			{
				/// @brief Defines the section keys where settings for the Disto real time
				/// clock settings are stored.
				struct rtc
				{
					/// @brief The key where the enable Real Time Clock setting is stored.
					static const inline string_type enable = "devices.real-time-clock.enable";
				};

				/// @brief Defines the keys for Becker Port settings.
				struct becker_port
				{
					/// @brief The key where the enable Becker Port Device setting is stored.
					static const inline string_type enabled = "devices.becker-port.enable";

					/// @brief Defines the keys for Becker Port server settings.
					struct server
					{
						/// @brief The key where the enable Becker Port server address setting is stored.
						static const inline string_type address = "devices.becker-port.server.address";
						/// @brief The key where the enable Becker Port server port setting is stored.
						static const inline string_type port = "devices.becker-port.server.port";
					};
				};
			};
		};

		/// @brief The default ROM identifier.
		static const auto default_rom_id = rom_image_id_type::microsoft;
		/// @brief A collection of value conversions used to convert the ROM identifier to an
		/// integer value that will be stored in the settings.
		static const std::map<rom_image_id_type, size_type> rom_id_integer_values_;
		/// @brief A collection of value conversions used to convert an integer from the
		/// settings to a ROM identifier.
		static const std::map<size_type, rom_image_id_type> integer_values_to_rom_ids_;

		/// @brief The value store that manages serializing the configuration values.
		value_store_type value_store_;
	};

}
