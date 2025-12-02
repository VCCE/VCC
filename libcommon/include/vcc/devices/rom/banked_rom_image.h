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
#include "vcc/devices/rom/rom_image.h"
#include <filesystem>


namespace vcc::devices::rom
{

	/// @brief A Banked ROM image.
	///
	/// This ROM image type provides support for banked regions within the ROM
	/// allowing access to larger ROMs than the limited address space allows.
	class banked_rom_image
	{
	public:

		/// @brief Type alias for the underlying ROM image used.
		using rom_image_type = ::vcc::devices::rom::rom_image;
		/// @brief Type alias for each datum stored in the ROM image.
		using value_type = rom_image_type::value_type;
		/// @brief Type alias to lengths, 1 dimension sizes, and indexes.
		using size_type = rom_image_type::size_type;
		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;


		/// @copydoc rom_image_type::empty
		[[nodiscard]] bool empty() const
		{
			return rom_image_.empty();
		}

		/// @copydoc rom_image_type::filename
		[[nodiscard]] path_type filename() const
		{
			return rom_image_.filename();
		}

		/// @brief Loads a ROM image into memory.
		/// 
		/// @param filename The filename of the ROM image to load.
		/// 
		/// @return `true` if the load was successful; `false` otherwise.
		LIBCOMMON_EXPORT [[nodiscard]] bool load(path_type filename);

		/// @brief Removes the ROM image and resets the state of the bank selector
		/// to bank 0.
		void clear()
		{
			rom_image_.clear();
			bank_offset_ = 0;
			bank_register_ = 0;
		}

		/// @brief Resets the ROM device and sets the bank to 0.
		void reset()
		{
			select_bank(0);
		}

		/// @brief Returns the selected bank.
		/// 
		/// @return The selected bank.
		[[nodiscard]] unsigned char selected_bank() const
		{
			return bank_register_;
		}

		/// @brief Selects a new bank.
		/// 
		/// @param bank_id The bank to select.
		void select_bank(unsigned char bank_id)
		{
			if (!rom_image_.empty())
			{
				bank_register_ = bank_id;
				bank_offset_ = bank_id * bank_size_;
			}
		}

		/// @copydoc rom_image_type::read_memory_byte
		[[nodiscard]] value_type read_memory_byte(size_type memory_address) const
		{
			return rom_image_.read_memory_byte(bank_offset_ + memory_address);
		}


	private:

		/// @brief The size of each bank.
		static const unsigned bank_size_ = 16384;	//	8k banks - WUT? FIXME-CHET: This should be configurable

		/// @brief The ROM image.
		rom_image_type rom_image_;
		/// @brief The currently selected bank.
		unsigned char bank_register_ = 0;
		/// @brief The offset into the ROM image where the current bank starts.
		size_type bank_offset_ = 0;
	};


	/// @brief Convert an lvalue reference to a Banked ROM image to an rvalue.
	/// 
	/// @param image The Banked ROM image to convert.
	/// 
	/// @return The reference passed in `image` as an `rvalue`.
	inline banked_rom_image&& move(banked_rom_image& image)
	{
		return static_cast<banked_rom_image&&>(image);
	}

}
