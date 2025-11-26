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
#include "vcc/detail/exports.h"
#include <string>


namespace vcc::bus
{

	class LIBCOMMON_EXPORT cartridge_driver
	{
	public:

		/// @brief Specifies the type used to store a size of length.
		using size_type = std::size_t;


	public:

		/// @brief Construct the cartridge to a default state.
		cartridge_driver() = default;
		cartridge_driver(const cartridge_driver&) = delete;
		cartridge_driver(cartridge_driver&&) = delete;

		/// @brief Release all resources held by the cartridge.
		virtual ~cartridge_driver() = default;

		/// @brief Resets the device to a default state.
		///
		/// Resets the cartridge to a default state.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated an exception is thrown.
		/// 
		/// @todo Add exception information. Need custom exceptions first.
		virtual void reset();

		/// @brief Read a byte of memory from the cartridge.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated an exception is thrown.
		/// 
		/// @todo Add exception information. Need custom exceptions first.
		/// @todo consider defining the result of reading from a memory address that does
		/// not exist (i.e. wrapping around when out of range).
		/// 
		/// @param memory_address The memory address to read.
		/// 
		/// @return The value at the location specified in `memory_address`, if memory at
		/// that address exists; otherwise the return value is implementation defined.
		virtual [[nodiscard]] unsigned char read_memory_byte(size_type memory_address);

		/// @brief Write a byte value to an I/O port.
		/// 
		/// Write a byte of data to an I/O port.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated an exception is thrown.
		/// 
		/// @todo Add exception information. Need custom exceptions first.
		/// @todo Consider renaming this to try_write_port
		/// 
		/// @param port_id A value from 0 to 255 specifying the I/O port to write to.
		/// @param value The value to write.
		virtual void write_port(unsigned char port_id, unsigned char value);

		/// @brief Read a byte value from an I/O port.
		/// 
		/// Read a byte value from an I/O port.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated an exception is thrown.
		/// 
		/// @todo Add exception information. Need custom exceptions first.
		/// @todo Consider renaming this to try_read_port
		/// @todo Consider changing return type to std::optional and let the call site handle it.
		/// 
		/// @param port_id A value from 0 to 255 specifying the I/O port to read from. If
		/// the port id is not supported by the cartridge the state of the cartridge remains
		/// unchanged and a value of 0 (zero) is returned.
		/// 
		/// @return If the port specified in `port_id` is supported by the cartridge a
		/// byte value mirroring the contents of the port; otherwise 0 (zero).
		virtual [[nodiscard]] unsigned char read_port(unsigned char port_id);

		/// @brief Process a horizontal sync signal.
		///
		/// Invoked frequently during system emulation. This must be called by the host to allow
		/// cartridge to perform tasks asynchronously and manage their state in line with the overall
		/// emulation.
		/// 
		/// The current frequency is roughly 15.72khz (0.06361323ms/63.61323us per cycle) but
		/// is subject to change.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated an exception is thrown.
		/// 
		/// @todo Add exception information. Need custom exceptions first.
		/// @todo Consider renaming this.
		virtual void update(float delta);

		/// @brief Read the current audio sample.
		/// 
		/// Read the current waveform sample data from the cartridge. The value returned is
		/// 16 bits containing two 8 bit samples. The most significant byte of the value
		/// contains the left channel and least significant byte contains the right channel.
		/// 
		/// If the cartridge does not play audio or its audio functionality is disabled it
		/// returns 0.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated an exception is thrown.
		/// 
		/// @todo Add exception information. Need custom exceptions first.
		/// 
		/// @return The stereo waveform sample data.
		virtual [[nodiscard]] unsigned short sample_audio();
	};

}
