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
#include "becker_cartridge_driver.h"
#include <stdexcept>


namespace vcc::cartridges::becker_port
{

	void becker_cartridge_driver::start(
		const string_type& server_address,
		const string_type& server_port)
	{
		// Note: this relies on update_connection_settings to validate the settings and throw
		// on invalid values.
		update_connection_settings(server_address, server_port);
		becker_device_.enable(true);
	}

	void becker_cartridge_driver::stop()
	{
		becker_device_.enable(false);
	}


	void becker_cartridge_driver::write_port(unsigned char port_id, unsigned char value)
	{
		if (port_id == mmio_ports::data)
		{
			// TODO-CHET: this should call a function called write_data() without the port id.
			becker_device_.write(value, port_id);
		}
	}

	unsigned char becker_cartridge_driver::read_port(unsigned char port_id)
	{
		switch (port_id)
		{
		case mmio_ports::status:
			// TODO-CHET: this should call a function called has_data() without the port id.
			return becker_device_.read(port_id) != 0 ? mmio_ports::status_bits::data_ready : 0;

		case mmio_ports::data:
			// TODO-CHET: this should call a function called read_data() without the port id.
			return becker_device_.read(port_id);
		}

		return 0;
	}

	becker_cartridge_driver::string_type becker_cartridge_driver::server_address() const
	{
		return becker_device_.server_address();
	}

	becker_cartridge_driver::string_type becker_cartridge_driver::server_port() const
	{
		return becker_device_.server_port();
	}

	void becker_cartridge_driver::update_connection_settings(
		const string_type& server_address,
		const string_type& server_port)
	{
		if (server_address.empty())
		{
			throw std::invalid_argument("Cannot update Becker Port Driver connection settings. The server address is empty.");
		}

		if (server_port.empty())
		{
			throw std::invalid_argument("Cannot update Becker Port Driver connection settings. The server port is empty.");
		}

		becker_device_.sethost(server_address.c_str(), server_port.c_str());
	}

}
