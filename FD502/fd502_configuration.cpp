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
#include "fd502_configuration.h"


namespace vcc::cartridges::fd502
{

	const std::map<
		fd502_configuration::rom_image_id_type,
		fd502_configuration::size_type> fd502_configuration::rom_id_integer_values_ =
	{
		{rom_image_id_type::custom, 0},
		{rom_image_id_type::microsoft, 1},
		{rom_image_id_type::rgbdos, 2}
	};

	const std::map<
		fd502_configuration::size_type,
		fd502_configuration::rom_image_id_type> fd502_configuration::integer_values_to_rom_ids_ =
	{
		{0, rom_image_id_type::custom },
		{1, rom_image_id_type::microsoft },
		{2, rom_image_id_type::rgbdos }
	};

	fd502_configuration::fd502_configuration(path_type path, string_type section)
		: value_store_(std::move(path), move(section))
	{
	}


	void fd502_configuration::set_turbo_mode(bool is_enabled)
	{
		value_store_.write(keys::drives::turbo_mode, is_enabled);
	}

	bool fd502_configuration::is_turbo_mode_enabled() const
	{
		return value_store_.read(keys::drives::turbo_mode, true);
	}

	void fd502_configuration::set_rom_image_id(rom_image_id_type rom_id) const
	{
		value_store_.write(keys::disk_basic::rom_id, rom_id_integer_values_.at(rom_id));
	}

	fd502_configuration::rom_image_id_type fd502_configuration::get_rom_image_id() const
	{
		auto index(value_store_.read(keys::disk_basic::rom_id, rom_id_integer_values_.at(default_rom_id)));
		const auto id_ptr(integer_values_to_rom_ids_.find(index));

		return id_ptr == integer_values_to_rom_ids_.end()
			? default_rom_id
			: id_ptr->second;
	}

	void fd502_configuration::set_rom_image_directory(const path_type& path)
	{
		value_store_.write(keys::disk_basic::rom_directory, path.string());
	}

	fd502_configuration::path_type fd502_configuration::rom_image_directory() const
	{
		return value_store_.read(keys::disk_basic::rom_directory);
	}

	void fd502_configuration::set_rom_image_path(const path_type& path)
	{
		value_store_.write(keys::disk_basic::selected_rom_path, path);
	}

	fd502_configuration::path_type fd502_configuration::rom_image_path() const
	{
		return value_store_.read(keys::disk_basic::selected_rom_path);
	}


	void fd502_configuration::set_disk_image_directory(path_type path)
	{
		value_store_.write(keys::drives::disk_image_directory, path);
	}

	fd502_configuration::path_type fd502_configuration::disk_image_directory() const
	{
		return value_store_.read(keys::drives::disk_image_directory);
	}

	void fd502_configuration::set_serialize_drive_mount_settings(bool is_enabled)
	{
		value_store_.write(keys::drives::mount_paths::serialize_mount_paths, is_enabled);
	}

	bool fd502_configuration::serialize_drive_mount_settings() const
	{
		return value_store_.read(keys::drives::mount_paths::serialize_mount_paths, true);
	}

	void fd502_configuration::set_disk_image_path(drive_id_type drive_id, const path_type& path)
	{
		value_store_.write(get_drive_mount_path_key(drive_id), path);
	}

	fd502_configuration::path_type fd502_configuration::disk_image_path(drive_id_type drive_id) const
	{
		return value_store_.read(get_drive_mount_path_key(drive_id));
	}


	void fd502_configuration::enable_rtc(bool state)
	{
		value_store_.write(keys::devices::rtc::enable, state);
	}

	bool fd502_configuration::is_rtc_enabled() const
	{
		return value_store_.read(keys::devices::rtc::enable, false);
	}


	void fd502_configuration::enable_becker_port(bool is_enabled) const
	{
		value_store_.write(keys::devices::becker_port::enabled, is_enabled);
	}

	bool fd502_configuration::is_becker_port_enabled() const
	{
		return value_store_.read(keys::devices::becker_port::enabled, false);
	}


	void fd502_configuration::becker_port_server_address(const string_type& address) const
	{
		value_store_.write(keys::devices::becker_port::server::address, address);
	}

	fd502_configuration::string_type fd502_configuration::becker_port_server_address() const
	{
		return value_store_.read(keys::devices::becker_port::server::address, "127.0.0.1");
	}


	void fd502_configuration::becker_port_server_port(const string_type& port) const
	{
		value_store_.write(keys::devices::becker_port::server::port, port);
	}

	fd502_configuration::string_type fd502_configuration::becker_port_server_port() const
	{
		return value_store_.read(keys::devices::becker_port::server::port, "65504");
	}


	fd502_configuration::string_type fd502_configuration::get_drive_mount_path_key(drive_id_type drive_id) const
	{
		return std::vformat(keys::drives::mount_paths::mount_path_fmt, std::make_format_args(drive_id));
	}

}
