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

		static const unsigned BankSize = 16384;	//	8k banks

		bool empty() const
		{
			return rom_image_.empty();
		}

		std::string filename() const
		{
			return rom_image_.filename();
		}

		LIBCOMMON_EXPORT bool load(std::string filename);

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

		unsigned char selected_bank() const
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

		value_type read_memory_byte(size_type memory_address) const
		{
			return rom_image_.read_memory_byte(bank_offset_ + memory_address);
		}


	private:

		rom_image_type rom_image_;
		unsigned char bank_register_ = 0;
		size_type bank_offset_ = 0;
	};

}
