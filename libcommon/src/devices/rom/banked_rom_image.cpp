#include <vcc/devices/rom/banked_rom_image.h>


namespace vcc::devices::rom
{

	bool banked_rom_image::load(std::string filename)
	{
		if (!rom_image_.load(move(filename)))
		{
			return false;
		}

		bank_offset_ = 0;
		bank_register_ = 0;

		return true;
	}

}
