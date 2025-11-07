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
#include <vcc/core/cartridge.h>
#include <vcc/core/cartridge_context.h>
#include <vector>
#include <memory>


namespace vcc::cartridges
{

	/// @brief A cartridge that contains a ROM image.
	/// 
	/// @todo provide summary once the switch `banked_rom_image` is complete.
	class rom_cartridge : public ::vcc::core::cartridge
	{
	public:

		/// @brief The type that defines the interface for communicating with the
		/// system managing the cartridge.
		using context_type = ::vcc::core::cartridge_context;
		/// @brief The type used to store the ROM image.
		/// @todo Change to using `banked_rom_image`.
		using buffer_type = std::vector<uint8_t>;
		/// @brief The type used to represent a size or length.
		using size_type = buffer_type::size_type;


	public:

		/// @brief Constructs a ROM Cartridge.
		/// 
		/// @param context The context of the system managing the cartridge. This parameter cannot be null.
		/// @param name The name of the cartridge. This parameter cannot be empty.
		/// @param catalog_id The catalog id or part number of the cartridge.
		/// @param buffer A buffer containing the ROM image. This parameter cannot be empty.
		/// @param enable_bank_switching Specifies if bank switching should be enabled.
		/// 
		/// @throw std::invalid_argument If `context` is null.
		/// @throw std::invalid_argument If `name` is empty.
		/// @throw std::invalid_argument If `buffer` is empty.
		LIBCOMMON_EXPORT rom_cartridge(
			std::unique_ptr<context_type> context,
			name_type name,
			catalog_id_type catalog_id,
			buffer_type buffer,
			bool enable_bank_switching);
		
		/// @inheritdoc
		LIBCOMMON_EXPORT [[nodiscard]] name_type name() const override;

		/// @inheritdoc
		LIBCOMMON_EXPORT [[nodiscard]] catalog_id_type catalog_id() const override;

		/// @brief Retrieves an optional description of the cartridge.
		/// 
		/// This function may be invoked prior to calling `start` to initialize the device
		/// and after `stop` has been called to terminate the device.
		///
		/// @return Am empty string.
		LIBCOMMON_EXPORT [[nodiscard]] description_type description() const override;

		/// @brief Initialize the cartridge.
		///
		/// Initialize the cartridge to a default state and sets the cartridge select line to
		/// auto-start the ROM when the system starts up.
		/// 
		/// @todo This should throw on multiple calls to provide base behavior
		/// @todo Add exception information. Need custom exceptions first.
		LIBCOMMON_EXPORT void start() override;

		/// @brief Reset the cartridge state.
		/// 
		/// Resets the cartridge to a default state. If bank switching is enabled the current
		/// bank is set to 0 (zero).
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated an exception is thrown.
		/// 
		/// @todo This does not throw the appropriate exceptions for init/term.
		/// @todo Add exception information. Need custom exceptions first.
		LIBCOMMON_EXPORT void reset() override;

		/// @brief Read a byte of memory from the ROM
		/// 
		/// Read a byte of memory from the ROM. If the address where the value will be read
		/// from exceeds the first 16K of the ROM image the value will be read from the
		/// region of memory pointed to by the currently selected bank.
		/// 
		/// If this function is called before the cartridge is initialized or after it has
		/// been terminated an exception is thrown.
		/// 
		/// @todo This does not throw the appropriate exceptions for init/term.
		/// @todo Add exception information. Need custom exceptions first.
		/// 
		/// @param memory_address The address in memory to read the byte from.
		/// 
		/// @return The value read from memory.
		LIBCOMMON_EXPORT [[nodiscard]] unsigned char read_memory_byte(size_type memory_address) override;

		/// @inheritdoc
		/// 
		/// @todo This does not throw the appropriate exceptions for init/term.
		/// @todo Add details of the port handler
		LIBCOMMON_EXPORT void write_port(unsigned char port_id, unsigned char value) override;


	private:

		const std::unique_ptr<context_type> context_;
		const name_type name_;
		const catalog_id_type catalog_id_;
		const buffer_type buffer_;
		const bool enable_bank_switching_;
		size_type bank_offset_;
	};

}
