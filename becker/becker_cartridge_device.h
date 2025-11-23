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
#include "vcc/devices/serial/beckerport.h"
#include "vcc/bus/cartridge_device.h"


class becker_cartridge_device : public ::vcc::bus::cartridge_device
{
public:

	using becker_device_type = ::vcc::devices::serial::Becker;
	using string_type = std::string;


public:

	void start(const string_type& server_address, const string_type& server_port);
	void stop();

	void write_port(unsigned char port_id, unsigned char value) override;
	unsigned char read_port(unsigned char port_id) override;

	void set_server_address(
		const string_type& server_address,
		const string_type& server_port);


private:

	string_type server_address() const;
	string_type server_port() const;

private:

	struct mmio_ports
	{
		static const auto status = 0x41;
		static const auto data = 0x42;
	};


private:

	becker_device_type becker_device_;
};
