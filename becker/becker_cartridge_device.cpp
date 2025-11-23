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
#include "becker_cartridge_device.h"


void becker_cartridge_device::start(
	const string_type& server_address,
	const string_type& server_port)
{
	becker_device_.sethost(server_address.c_str(), server_port.c_str());
	becker_device_.enable(true);
}

void becker_cartridge_device::stop()
{
	becker_device_.enable(false);
}


void becker_cartridge_device::write_port(unsigned char port_id, unsigned char value)
{
	if (port_id == mmio_ports::data)
	{
		// FIXME-CHET: this should call a function called write_data() without the port id.
		becker_device_.write(value, port_id);
	}
}

unsigned char becker_cartridge_device::read_port(unsigned char port_id)
{
	switch (port_id)
	{
	case mmio_ports::status:
		// FIXME-CHET: WTF is the 2? make symbolic
		// FIXME-CHET: this should call a function called read_status() without the port id.
		return becker_device_.read(port_id) != 0 ? 2 : 0;

	case mmio_ports::data:
		// FIXME-CHET: this should call a function called read_data() without the port id.
		return becker_device_.read(port_id);
	}

	return 0;
}

becker_cartridge_device::string_type becker_cartridge_device::server_address() const
{
	return becker_device_.server_address();
}

becker_cartridge_device::string_type becker_cartridge_device::server_port() const
{
	return becker_device_.server_port();
}

void becker_cartridge_device::set_server_address(
	const string_type& server_address,
	const string_type& server_port)
{
	becker_device_.sethost(server_address.c_str(), server_port.c_str());
}
