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
#include <vcc/core/detail/exports.h>
#include <string>
#include <vector>


namespace vcc::devices::rom
{

	/// @brief The ROM Image class represents the contents of a simulated ROM chip.
	class rom_image
	{
	public:

		/// @brief The type used to represent paths.
		using path_type = std::string;
		/// @brief The type used to store a single datum in ROM.
		using value_type = unsigned char;
		/// @brief The type used to store the ROM in memory.
		using container_type = std::vector<value_type>;
		/// @brief The type used to represent a size or length.
		using size_type = container_type::size_type;


	public:

		/// @brief Constructs an empty ROM image.
		LIBCOMMON_EXPORT rom_image() = default;
		LIBCOMMON_EXPORT ~rom_image() = default;

		/// @brief Determines if the ROM image is empty.
		/// 
		/// @return `true` if the ROM image is empty; `false` otherwise.
		[[nodiscard]] bool empty() const
		{
			return data_.empty();
		}

		/// @brief Retrieve filename of the loaded ROM.
		/// 
		/// @return The filename of the loaded ROM or an empty string if no ROM is loaded.
		[[nodiscard]] path_type filename() const
		{
			return filename_;
		}

		/// @brief Loads a ROM file into memory.
		/// 
		/// @param filename The filename to load.
		/// 
		/// @return `true` if the ROM file was loaded successfully; `false` otherwise.
		LIBCOMMON_EXPORT [[nodiscard]] bool load(path_type filename);

		/// @brief Releases the ROM image.
		void clear()
		{
			filename_.clear();
			data_.clear();
		}

		/// @brief Read a byte of memory from the ROM
		/// 
		/// Read a byte of memory from the ROM. If the address where the value will be read
		/// from exceeds the amount of memory in the loaded ROM the read operation will wrap
		/// around to the beginning of memory.
		/// 
		/// @param memory_address The address in memory to read the byte from.
		/// 
		/// @return The byte read from memory if a ROM is loaded otherwise a value of 0 (zero)
		/// is returned.
		[[nodiscard]] value_type read_memory_byte(size_type memory_address) const
		{
			if (data_.empty())
			{
				return 0;
			}

			return data_[memory_address % data_.size()];
		}


	private:

		path_type filename_;
		container_type data_;
	};

}
