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
#include "fd502_cartridge_driver.h"


namespace vcc::cartridges::fd502
{

	fd502_cartridge_driver::fd502_cartridge_driver(
		std::shared_ptr<expansion_port_host_type> host,
		std::shared_ptr<expansion_port_bus_type> bus)
		:
		host_(host),
		bus_(bus),
		rom_(std::make_unique<rom_image_type>()),
		wd1793_(bus)
	{
		if (host_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct FD502 Cartridge Driver. Host is null.");
		}

		if (bus_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct FD502 Cartridge Driver. Bus is null.");
		}
	}


	fd502_cartridge_driver::size_type fd502_cartridge_driver::drive_count() const
	{
		return wd1793_.drive_count();
	}

	bool fd502_cartridge_driver::is_motor_running() const noexcept
	{
		return wd1793_.is_motor_running();
	}
	
	std::optional<fd502_cartridge_driver::drive_id_type> fd502_cartridge_driver::get_selected_drive_id() const noexcept
	{
		return wd1793_.get_selected_drive_id();
	}
	
	fd502_cartridge_driver::head_id_type fd502_cartridge_driver::get_selected_head() const noexcept
	{
		return wd1793_.get_selected_head();
	}

	fd502_cartridge_driver::size_type fd502_cartridge_driver::get_head_position() const noexcept
	{
		return wd1793_.get_head_position();
	}

	fd502_cartridge_driver::size_type fd502_cartridge_driver::get_current_sector() const noexcept
	{
		return wd1793_.get_current_sector();
	}


	void fd502_cartridge_driver::start(
		rom_image_ptr_type rom,
		bool enable_turbo,
		bool enable_rtc,
		bool enable_becker_port,
		const string_type& becker_server_address,
		const string_type& becker_server_port)
	{
		if (host_ == nullptr)
		{
			throw std::invalid_argument("Cannot start the FD502 Cartridge Driver. ROM is null.");
		}

		rom_ = move(rom);

		wd1793_.start();
		wd1793_.set_turbo_mode(enable_turbo);	// TODO-CHET: Pass as arg to the wd1793 start function

		enable_rtc_device(enable_rtc);

		becker_port_.sethost(becker_server_address.c_str(), becker_server_port.c_str());
		becker_port_.enable(enable_becker_port);
	}

	void fd502_cartridge_driver::stop()
	{
		rom_ = std::make_unique<rom_image_type>();
		disto_rtc_.enable(false);	//TODO-CHET: every device should have a start and stop 
		becker_port_.enable(false);	//TODO-CHET: every device should have a start and stop
		wd1793_.stop();
	}

	void fd502_cartridge_driver::enable_rtc_device(bool enable_rtc)
	{
		disto_rtc_.enable(enable_rtc);
	}

	void fd502_cartridge_driver::update_becker_port_settings(
		bool enable_becker_port,
		const string_type& becker_server_address,
		const string_type& becker_server_port)
	{
		becker_port_.sethost(becker_server_address.c_str(), becker_server_port.c_str());
		becker_port_.enable(enable_becker_port);
	}

	unsigned char fd502_cartridge_driver::read_memory_byte(size_type Address)
	{
		return rom_->read_memory_byte(Address);
	}

	void fd502_cartridge_driver::set_rom(rom_image_ptr_type rom)
	{
		if (rom == nullptr)
		{
			throw std::invalid_argument("Cannot set ROM. ROM image is null.");
		}

		rom_ = move(rom);
	}

	void fd502_cartridge_driver::set_turbo_mode(bool enable_turbo)
	{
		wd1793_.set_turbo_mode(enable_turbo);
	}

	void fd502_cartridge_driver::insert_disk(
		drive_id_type drive_id,
		std::unique_ptr<disk_image_type> disk_image,
		path_type file_path)
	{
		wd1793_.insert_disk(drive_id, move(disk_image), std::move(file_path));
	}

	void fd502_cartridge_driver::eject_disk(drive_id_type drive_id)
	{
		wd1793_.eject_disk(drive_id);
	}

	fd502_cartridge_driver::path_type fd502_cartridge_driver::get_mounted_disk_filename(drive_id_type drive_id) const
	{
		return wd1793_.get_mounted_disk_filename(drive_id);
	}

	bool fd502_cartridge_driver::is_disk_write_protected(drive_id_type drive_id) const
	{
		return wd1793_.is_disk_write_protected(drive_id);
	}

	void fd502_cartridge_driver::write_port(unsigned char port, unsigned char value)
	{
		switch (port)
		{
		case 0x42:
			if (becker_port_.enabled())
			{
				becker_port_.write(value, port);
			}
			break;

		case 0x50:
			if (disto_rtc_.enabled())
			{
				disto_rtc_.write_data(value);
			}
			break;

		case 0x51:
			if (disto_rtc_.enabled())
			{
				disto_rtc_.set_read_write_address(value);
			}
			break;

		case mmio_ports::control_register:
			wd1793_.write_control_register(value);
			break;

		case mmio_ports::command_register:
			wd1793_.write_command_register(value);
			break;

		case mmio_ports::track_register:
			wd1793_.write_track_register(value);
			break;

		case mmio_ports::sector_register:
			wd1793_.write_sector_register(value);
			break;

		case mmio_ports::data_register:
			wd1793_.write_data_register(value);
			break;
		}
	}

	unsigned char fd502_cartridge_driver::read_port(unsigned char port)
	{
		switch (port)
		{
		case 0x41:
		case 0x42:
			if (becker_port_.enabled())
			{
				return becker_port_.read(port);
			}
			break;

		case 0x50:
		case 0x51:
			if (disto_rtc_.enabled())
			{
				return disto_rtc_.read_data();
			}
			break;

		case mmio_ports::status_register:
			return wd1793_.read_status_register();

		case mmio_ports::track_register:
			return wd1793_.read_track_register();

		case mmio_ports::sector_register:
			return wd1793_.read_sector_register();

		case mmio_ports::data_register:
			return wd1793_.read_data_register();
		}

		return 0;
	}

	void fd502_cartridge_driver::update(float delta)
	{
		wd1793_.update(delta);
	}

}

