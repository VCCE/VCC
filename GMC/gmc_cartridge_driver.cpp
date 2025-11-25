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
#include "gmc_cartridge_driver.h"
#include "resource.h"
#include "vcc/ui/menu/menu_builder.h"
#include "vcc/common/DialogOps.h"
#include "vcc/utils/persistent_value_store.h"
#include "vcc/utils/winapi.h"
#include "vcc/utils/filesystem.h"
#include <Windows.h>
#include <stdexcept>


namespace vcc::cartridges::gmc
{

	gmc_cartridge_driver::gmc_cartridge_driver(std::unique_ptr<expansion_port_bus_type> bus)
		: bus_(move(bus))
	{
		if (bus_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Game Master Cartridge driver. Bus is null.");
		}
	}


	void gmc_cartridge_driver::start(const path_type& rom_filename)
	{
		if (!rom_filename.empty())
		{
			load_rom(rom_filename, false);
		}

		psg_.start();
	}

	void gmc_cartridge_driver::reset()
	{
		rom_image_.reset();
		psg_.reset();
	}

	void gmc_cartridge_driver::write_port(unsigned char port, unsigned char data)
	{
		switch (port)
		{
		case mmio_ports::select_rom_bank:
			rom_image_.select_bank(data);
			break;

		case mmio_ports::psg_io:
			psg_.write(data);
			break;
		}
	}

	unsigned char gmc_cartridge_driver::read_port(unsigned char port)
	{
		if (port == mmio_ports::select_rom_bank)
		{
			return rom_image_.selected_bank();
		}

		return 0;
	}


	bool gmc_cartridge_driver::has_rom() const noexcept
	{
		return !rom_image_.empty();
	}

	gmc_cartridge_driver::path_type gmc_cartridge_driver::rom_filename() const
	{
		return rom_image_.filename();
	}


	unsigned char gmc_cartridge_driver::read_memory_byte(size_type address)
	{
		return rom_image_.read_memory_byte(address);
	}

	gmc_cartridge_driver::sample_type gmc_cartridge_driver::sample_audio()
	{
		psg_device_type::sample_type lbuffer = 0;
		psg_device_type::sample_type rbuffer = 0;

		return psg_.sound_stream_update(lbuffer, rbuffer);
	}


	void gmc_cartridge_driver::load_rom(const path_type& filename, bool reset_on_load)
	{
		if (filename.empty())
		{
			throw std::invalid_argument("Game Master Cartridge driver cannot load ROM. Filename is empty.");
		}

		if (rom_image_type new_rom_image; new_rom_image.load(filename))
		{
			rom_image_ = move(new_rom_image);
			bus_->set_cartridge_select_line(true);
			if (reset_on_load)
			{
				bus_->reset();
			}
		}
	}

	void gmc_cartridge_driver::eject_rom()
	{
		rom_image_.clear();
		bus_->reset();
	}

}
