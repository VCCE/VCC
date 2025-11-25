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
#include "vcc/devices/psg/sn76496.h"
#include "vcc/devices/rom/banked_rom_image.h"
#include "vcc/bus/cartridge_driver.h"
#include "vcc/bus/expansion_port_bus.h"
#include <memory>


namespace vcc::cartridges::gmc
{

	/// @brief Becker Port cartridge driver.
	///
	/// This cartridge driver emulates the Game Master Cartridge including paged ROM images
	/// and the SN76489 Programmable Sound Generator (PSG).
	class gmc_cartridge_driver : public ::vcc::bus::cartridge_driver
	{
	public:

		/// @brief Type alias for the component acting as the expansion bus the cartridge plugin
		/// is connected to.
		using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
		/// @brief Type alias for file paths.
		using path_type = std::string;
		/// @brief Type alias for the component that manages the ROM image.
		using rom_image_type = ::vcc::devices::rom::banked_rom_image;
		/// @brief Type alias for the component emulating the SN76489 PSG.
		using psg_device_type = ::vcc::devices::psg::sn76489_device;


	public:

		/// @brief Constructs the cartridge driver.
		/// 
		/// @param bus A pointer to the bus interface.
		/// 
		/// @throws std::invalid_argument if `bus` is null.
		explicit gmc_cartridge_driver(std::unique_ptr<expansion_port_bus_type> bus);

		/// @brief Start the cartridge driver and initialize the emulated hardware.
		/// 
		/// @param rom_filename The filename of the ROM file to load at startup.
		void start(const path_type& rom_filename);

		/// @brief Resets the device simulating a hardware reset.
		void reset() override;

		/// @brief Indicates if the driver currently has a ROM image loaded.
		/// 
		/// @return `true` if the driver has a ROM file loaded; `false` otherwise.
		[[nodiscard]] bool has_rom() const noexcept;

		/// @brief Retrieves the filename of the currently loaded ROM image.
		/// 
		/// @return The filename of the currently loaded ROM image.
		[[nodiscard]] path_type rom_filename() const;

		/// @brief Loads a new rom image.
		/// 
		/// Attempts to load a specified ROM file. If the ROM image is loaded successfully
		/// the current image is replaced with the new one. If the ROM image cannot be
		/// loaded the current ROM image is not ejected.
		/// 
		/// @param filename The filename of the ROM image to load.
		/// @param reset_on_load Specifies if the system should simulate a hardware
		/// reset. If this value is `true` the system will be reset.
		/// 
		/// @throws std::invalid_argument if `filename` is empty.
		void load_rom(const path_type& filename, bool reset_on_load);

		void eject_rom();

		/// @brief Reads a byte from the currently loaded ROM image.
		/// 
		/// @param memory_address The address in the ROM to read from.
		/// 
		/// @return If a ROM image is loaded the function returns the byte at the
		/// address specified in `memory_address`. If a ROM image is not loaded the
		/// function returns 0.
		[[nodiscard]] unsigned char read_memory_byte(size_type memory_address) override;

		/// @inheritdoc
		void write_port(unsigned char port_id, unsigned char value) override;

		/// @inheritdoc
		[[nodiscard]] unsigned char read_port(unsigned char port_id) override;

		/// @brief Reads the current sample from the PSG.
		/// 
		/// @return The waveform sample from the sound generator's current state.
		[[nodiscard]] sample_type sample_audio() override;


	private:

		/// @brief Contains definitions of the memory mapped I/O ports used by the Game
		/// Master Cartridge hardware.
		struct mmio_ports
		{
			/// @brief Defines the port used to read and set the current ROM page.
			static const auto select_rom_bank = 0x40;
			/// @brief Defines the port used to communicate with the PSG.
			static const auto psg_io = 0x41;
		};

		const std::shared_ptr<expansion_port_bus_type> bus_;
		rom_image_type rom_image_;
		psg_device_type psg_;
	};

}
