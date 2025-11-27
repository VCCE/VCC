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
#include "vcc/bus/cartridge_driver.h"
#include "vcc/bus/expansion_port_bus.h"
#include <array>


namespace vcc::cartridges::rambuffer
{

	/// @brief RAM Disk cartridge driver.
	///
	/// This cartridge driver emulates a 512k RAM buffer that can be used as a RAM disk or
	/// print spooler.
	class ramdisk_cartridge_driver : public ::vcc::bus::cartridge_driver
	{
	public:

		/// @brief Type alias for memory addresses.
		using address_type = std::size_t;
		/// @brief Type alias for 512k memory buffer.
		using buffer_type = std::array<unsigned char, 1024u * 512u>;


	public:

		/// @brief Initializes the driver to a default working state.
		///
		/// Initializes the driver and emulated device to a default state
		/// with all memory filled with the pattern `0xff` and the current
		/// read/write address set to 0.
		void start();

		/// @brief Simulates a hardware reset of the device.
		///
		/// When this function is called the emulated hardware simulates a hardware reset by
		/// setting the current read/write address to 0. The memory buffer is left unchanged.
		void reset() override;

		/// @brief Write a byte value to an I/O port.
		/// 
		/// Write a byte of data to an I/O port.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated the behavior is undefined.
		/// 
		/// @param port_id A value from 0 to 255 specifying the I/O port to write to. See
		/// mmio_ports for information regarding the supported port ids.
		/// @param value The value to write.
		void write_port(unsigned char port_id, unsigned char value) override;

		/// @brief Read a byte value from an I/O port.
		/// 
		/// Read a byte value from an I/O port.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated the behavior is undefined.
		/// 
		/// @param port_id A value from 0 to 255 specifying the I/O port to read from. If
		/// the port id is not supported by the cartridge the state of the cartridge remains
		/// unchanged and a value of 0 (zero) is returned. See mmio_ports for information
		/// regarding the supported port ids.
		/// 
		/// @return If the port specified in `port_id` is supported by the cartridge a
		/// byte value mirroring the contents of the port; otherwise 0 (zero).
		unsigned char read_port(unsigned char port_id) override;


	private:

		/// @brief Initializes the device to a default state.
		/// 
		/// Initializes the emulated device by setting the read/write address set to 0 and
		/// optionally filling all memory with the pattern `0xff`.
		/// 
		/// @param initialize_memory If `true` specifies that memory should be initialized
		/// with a default pattern.
		void initialize_device_state(bool initialize_memory);


	private:

		/// @brief Contains definitions of the memory mapped I/O ports used by the RAM buffer
		/// hardware.
		struct mmio_ports
		{
			/// @brief This register is write only and sets bits 0 thru 7 of the address.
			static const unsigned char address_low = 0x40;
			/// @brief This register is write only and sets bits 8 thru 15 of the address.
			static const unsigned char address_middle = 0x41;
			/// @brief This register is write only and sets bits 16 thru 18 of the address.
			static const unsigned char address_high = 0x42;
			/// @brief This register is used to read and write data to and from the RAM buffer
			/// at the current address.
			static const unsigned char data = 0x43;
		};

		/// @brief The current read address.
		address_type current_address_ = 0;
		/// @brief Bits 0 thru 7 of the address.
		address_type address_byte_low_ = 0;
		/// @brief Bits 8 thru 15 of the address.
		address_type address_byte_middle_ = 0;
		/// @brief Bits 16 thru 18 of the address.
		address_type address_byte_high_ = 0;
		/// @brief The RAM buffer.
		buffer_type ram_ = {};
	};

}
