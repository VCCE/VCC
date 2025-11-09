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
#include "orchestra90cc_cartridge.h"
#include "resource.h"
#include <vcc/utils/FileOps.h>
#include <vcc/utils/winapi.h>
#include <vcc/utils/filesystem.h>
#include <Windows.h>


namespace
{
	unsigned char Rom[8192];

	bool LoadExtRom(const std::string& filename)	//Returns 1 on if loaded
	{
		FILE *rom_handle = nullptr;
		unsigned short index = 0;
		bool RetVal = false;

		rom_handle = fopen(filename.c_str(), "rb");
		if (rom_handle == nullptr)
		{
			memset(Rom, 0xFF, 8192);
		}
		else
		{
			while ((feof(rom_handle) == 0) & (index < 8192))
			{
				Rom[index++] = fgetc(rom_handle);
			}

			RetVal = true;
			fclose(rom_handle);
		}
		return RetVal;
	}

}


orchestra90cc_cartridge::orchestra90cc_cartridge(
	HINSTANCE module_instance,
	std::unique_ptr<context_type> context)
	:
	module_instance_(module_instance),
	context_(move(context))
{ }

orchestra90cc_cartridge::name_type orchestra90cc_cartridge::name() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME);
}

orchestra90cc_cartridge::catalog_id_type orchestra90cc_cartridge::catalog_id() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_CATNUMBER);
}

orchestra90cc_cartridge::description_type orchestra90cc_cartridge::description() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_DESCRIPTION);
}


void orchestra90cc_cartridge::reset()
{
	using ::vcc::utils::get_module_path;
	using ::vcc::utils::get_directory_from_path;

	if (LoadExtRom(get_directory_from_path(get_module_path(module_instance_)) + "ORCH90.ROM"))
	{
		context_->assert_cartridge_line(true);
	}

}

void orchestra90cc_cartridge::write_port(unsigned char Port,unsigned char Data)
{
	switch (Port)
	{
	case 0x7A:
		right_channel_buffer_=Data;			
		break;

	case 0x7B:
		left_channel_buffer_=Data;
		break;
	}
}

unsigned char orchestra90cc_cartridge::read_memory_byte(size_type Address)
{
	return Rom[Address & 8191];
}

unsigned short orchestra90cc_cartridge::sample_audio()
{
	return (left_channel_buffer_ << 8) | right_channel_buffer_;
}

