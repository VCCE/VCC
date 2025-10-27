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
#include <vcc/devices/rom/rom_image.h>


namespace vcc::devices::rom
{

	class banked_rom_image
	{
	public:

		using rom_image_type = ::vcc::devices::rom::rom_image;
		using value_type = rom_image_type::value_type;
		using size_type = rom_image_type::size_type;
		using path_type = std::string;

		static const unsigned BankSize = 16384;	//	8k banks

		[[nodiscard]] bool empty() const
		{
			return rom_image_.empty();
		}

		[[nodiscard]] path_type filename() const
		{
			return rom_image_.filename();
		}

		LIBCOMMON_EXPORT [[nodiscard]] bool load(path_type filename);

		void clear()
		{
			rom_image_.clear();
			bank_offset_ = 0;
			bank_register_ = 0;
		}

		void reset()
		{
			select_bank(0);
		}

		[[nodiscard]] unsigned char selected_bank() const
		{
			return bank_register_;
		}

		void select_bank(unsigned char bank_id)
		{
			if (!rom_image_.empty())
			{
				bank_register_ = bank_id;
				bank_offset_ = bank_id * BankSize;
			}
		}

		[[nodiscard]] value_type read_memory_byte(size_type memory_address) const
		{
			return rom_image_.read_memory_byte(bank_offset_ + memory_address);
		}


	private:

		rom_image_type rom_image_;
		unsigned char bank_register_ = 0;
		size_type bank_offset_ = 0;
	};

}
