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

	/// @brief Represents an interchangeable cartridge device.
	///
	/// This abstract class defines the fundamental operations of a cartridge device
	/// that can be connected to devices such as a computer or video game console to
	/// provide programs and data on ROM, extra RAM, or additional hardware capabilities
	/// such as a floppy disk controller or programmable sound generator.
	class LIBCOMMON_EXPORT cartridge
	{
	public:

		/// @brief Specifies the type used to store names.
		using name_type = std::string;
		/// @brief Specifies the type used to store catalog identifiers.
		using catalog_id_type = std::string;
		/// @brief Specifies the type used to store cartridge descriptions.
		using description_type = std::string;
		/// @brief Specifies the type used to store a size of length.
		using size_type = std::size_t;


	public:

		/// @brief Construct the cartridge to a default state.
		cartridge() = default;
		cartridge(const cartridge&) = delete;
		cartridge(cartridge&&) = delete;

		/// @brief Release all resources held by the cartridge.
		virtual ~cartridge() = default;

		/// @brief Retrieves the name of the cartridge.
		/// 
		/// Retrieves the name of the cartridge.
		/// 
		/// This function may be invoked prior to calling `start` to initialize the device
		/// and after `stop` has been called to terminate the device.
		///
		/// @return A string containing the cartridge name.
		virtual [[nodiscard]] name_type name() const = 0;

		/// @brief Retrieves an optional catalog identifier of the cartridge.
		/// 
		/// Retrieves an optional catalog identifier of the cartridge. Catalog identifiers
		/// are arbitrary and generally represent the part number of the cartridge included
		/// in catalogs and magazine advertisements. Cartridges are not requires to provide
		/// a catalog identifier and may return an empty string.
		/// 
		/// This function may be invoked prior to calling `start` to initialize the device
		/// and after `stop` has been called to terminate the device.
		///
		/// @return A string containing the catalog identifier. If the cartridge does not
		/// include a catalog id an empty string is returned.
		virtual [[nodiscard]] catalog_id_type catalog_id() const = 0;

		/// @brief Retrieves an optional description of the cartridge.
		/// 
		/// This function may be invoked prior to calling `start` to initialize the device
		/// and after `stop` has been called to terminate the device.
		///
		/// @return A string containing the description of the cartridge. If the cartridge
		/// does not include a description an empty string is returned.
		virtual [[nodiscard]] description_type description() const = 0;

		/// @brief Initialize the device.
		///
		/// Initialize the cartridge device to a default state. If this called more than
		/// once without stopping the device an exception is thrown.
		/// 
		/// @todo Add exception information. Need custom exceptions first.
		virtual void start();

		/// @brief Terminate the device.
		///
		/// Stops all device operations and releases all resources. This function must be
		/// called prior to the cartridge being destroyed to ensure that all resources are
		/// gracefully released. If this function is called before the cartridge is
		/// initialized or after it has been terminated an exception is thrown.
		/// 
		/// @todo Add exception information. Need custom exceptions first.
		virtual void stop();

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
		/// The current frequency is roughly 15.72khz but is subject to change.
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

		/// @brief Retrieve the cartridge status.
		/// 
		/// Retrieves the status of the cartridge as a human readable string.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated an exception is thrown.
		/// 
		/// @todo Add exception information. Need custom exceptions first.
		/// @todo Maybe rename `buffer_size` to `buffer_length`.
		/// 
		/// @param text_buffer The text buffer to generate the status string in.
		/// @param buffer_size The length of the text buffer.
		virtual void status(char* text_buffer, size_type buffer_size);

		/// @brief Inform the cartridge a menu item has been clicked.
		/// 
		/// Inform the cartridge a menu item associated with it has been clicked.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated an exception is thrown.
		/// 
		/// @todo Add exception information. Need custom exceptions first.
		/// 
		/// @param menu_item_id The identifier of the menu item.
		virtual void menu_item_clicked(unsigned char menu_item_id);
	};

}
