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
#include "vcc/devices/serial/beckerport.h"
#include "vcc/bus/cartridge_driver.h"


/// @brief Cartridge driver implementation of the Becker Port hardware.
///
/// Device implementation emulating the Becker Port driver.
class becker_cartridge_driver : public ::vcc::bus::cartridge_driver
{
public:

	/// @brief Defines the type used to reference a Becker Port driver implementation.
	using becker_driver_type = ::vcc::devices::serial::Becker;
	/// @brief Defines the type used to hold a variable length string.
	using string_type = std::string;


private:

	/// @brief Contains definitions of the memory mapped I/O ports used by the Becker
	/// hardware.
	struct mmio_ports
	{
		/// @brief Contains the current status of the device.
		/// 
		/// This read only port that contains the current status of the device represented
		/// by the following bits.
		/// 
		/// | Bit |   Pattern  | Description |
		/// |-----|------------|-------------|
		/// |  0  | 0b00000001 | Unused      |
		/// |  1  | 0b00000010 | Data ready. If this bit is set the device contains data ready to read. |
		/// |  2  | 0b00000100 | Unused      |
		/// |  3  | 0b00001000 | Unused      |
		/// |  4  | 0b00010000 | Unused      |
		/// |  5  | 0b00100000 | Unused      |
		/// |  6  | 0b01000000 | Unused      |
		/// |  7  | 0b10000000 | Unused      |
		static const auto status = 0x41u;

		/// @brief Data port for transferring data between the computer and the Becker port.
		/// 
		/// This port is used for transferring data between the computer and the Becker port.
		static const auto data = 0x42u;
	};


public:

	/// @brief Start the cartridge driver and initialize the emulated hardware.
	/// 
	/// @param server_address The address of the DriveWrite server to connect to.
	/// @param server_port The port of the DriveWire server to connect to.
	virtual void start(const string_type& server_address, const string_type& server_port);

	/// @brief Shuts down the emulated hardware and any active DriveWrite connections.
	virtual void stop();

	/// @inheritdoc
	void write_port(unsigned char port_id, unsigned char value) override;

	/// @inheritdoc
	unsigned char read_port(unsigned char port_id) override;

	/// @brief Updates the address and port of the DriveWrite server to connect to.
	/// 
	/// @param server_address The address of the DriveWrite server to connect to.
	/// @param server_port The port of the DriveWire server to connect to.
	virtual void update_connection_settings(
		const string_type& server_address,
		const string_type& server_port);


protected:

	/// @brief Retrieve the address of the DriveWire server the Becker device will connect to.
	/// 
	/// @return A string containing the server address.
	string_type server_address() const;

	/// @brief Retrieve the port of the DriveWire server the Becker device will connect to.
	/// 
	/// @return A string containing the server port.
	string_type server_port() const;


private:

	becker_driver_type becker_device_;
};
