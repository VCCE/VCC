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
#include "vcc/bus/cartridge_driver.h"
#include "vcc/bus/expansion_port_bus.h"
#include "vcc/bus/expansion_port_host.h"
#include "vcc/devices/rom/rom_image.h"
#include <memory>
#include <Windows.h>


namespace vcc::cartridges::orchestra90cc
{

	/// @brief Orchestra-90cc Stereo Pak hardware driver.
	///
	/// This driver emulates the Orchestra-90cc cartridge including the includes ROM image
	/// and two 8 bit digital-to-analog converters.
	class orchestra90cc_cartridge_driver : public ::vcc::bus::cartridge_driver
	{
	public:

		/// @brief Type alias for the component acting as the expansion bus the driver
		/// is connected to.
		using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
		/// @brief Type alias for the component that manages the ROM image.
		using rom_image_type = ::vcc::devices::rom::rom_image;
		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;


	public:

		/// @brief Construct the cartridge driver.
		/// 
		/// @param bus A pointer to the bus interface.
		/// 
		/// @throws std::invalid_argument if `bus` is null.
		explicit orchestra90cc_cartridge_driver(std::unique_ptr<expansion_port_bus_type> bus);

		/// @brief Start the cartridge driver and initialize the emulated hardware.
		/// 
		/// Initializes the cartridge driver and attempts to load a specified ROM file.
		/// 
		/// @param rom_filename The filename of the ROM file to load at startup. If the filename
		/// is empty no ROM is loaded.
		void start(const path_type& rom_filename);

		/// @brief Reads a byte from the ROM image if one is loaded.
		/// 
		/// @param memory_address The address in the ROM to read from.
		/// 
		/// @return If a ROM image is loaded the function returns the byte at the
		/// address specified in `memory_address`. If a ROM image is not loaded the
		/// function returns 0.
		unsigned char read_memory_byte(size_type memory_address) override;

		/// @inheritdoc
		void write_port(unsigned char port_id, unsigned char value) override;

		/// @brief Retrieves the output waveform sample.
		/// 
		/// @return The current waveform samples stored in the left and right channel buffers.
		sample_type sample_audio() override;


	protected:

		/// @brief Contains definitions of the memory mapped I/O ports used by the Orchestra-90cc
		/// driver hardware.
		struct mmio_ports
		{
			/// @brief This write only port is used to send a byte to the right channel DAC
			/// buffer.
			static const auto right_channel = 0x7a;
			/// @brief This write only port is used to send a byte to the left channel DAC
			/// buffer.
			static const auto left_channel = 0x7b;
		};


	private:

		/// @brief The expansion port bus.
		const std::unique_ptr<expansion_port_bus_type> bus_;
		/// @brief The ROM image.
		rom_image_type rom_image_;
		/// @brief The buffer containing the waveform sample for the left channel.
		unsigned char left_channel_buffer_ = 0;
		/// @brief The buffer containing the waveform sample for the right channel.
		unsigned char right_channel_buffer_ = 0;
	};

}
