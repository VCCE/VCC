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
#include "vcc/devices/psg/sn76496.h"
#include "vcc/devices/rom/banked_rom_image.h"
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_port_host.h"
#include "vcc/bus/expansion_port_ui.h"
#include "vcc/bus/expansion_port_bus.h"
#include <memory>
#include <Windows.h>


class gmc_cartridge : public ::vcc::bus::cartridge
{
public:

	using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
	using expansion_port_ui_type = ::vcc::bus::expansion_port_ui;
	using expansion_port_host_type = ::vcc::bus::expansion_port_host;
	using path_type = std::string;
	using rom_image_type = ::vcc::devices::rom::banked_rom_image;


public:

	gmc_cartridge(
		std::shared_ptr<expansion_port_host_type> host,
		std::unique_ptr<expansion_port_ui_type> ui,
		std::unique_ptr<expansion_port_bus_type> bus,
		HINSTANCE module_instance);

	name_type name() const override;
	catalog_id_type catalog_id() const override;
	description_type description() const override;

	void start() override;
	void reset() override;

	unsigned char read_memory_byte(size_type memory_address) override;

	void write_port(unsigned char port_id, unsigned char value) override;
	unsigned char read_port(unsigned char port_id) override;

	unsigned short sample_audio() override;

	void status(char* status_buffer, size_t buffer_size) override;
	void menu_item_clicked(unsigned char menu_item_id) override;


protected:

	void load_rom(const path_type& filename, bool reset_on_load);
	void build_menu();


private:

	struct menu_item_ids
	{
		static const unsigned int select_rom = 3;
	};

	struct mmio_ports
	{
		static const unsigned char select_bank = 0x40;
		static const unsigned char psg_io = 0x41;
	};

	static const path_type configuration_section_id_;
	static const path_type configuration_rom_key_id_;


	const std::shared_ptr<expansion_port_host_type> host_;
	const std::unique_ptr<expansion_port_ui_type> ui_;
	const std::unique_ptr<expansion_port_bus_type> bus_;
	const HINSTANCE module_instance_;
	rom_image_type rom_image_;

	using psg_device_type = ::vcc::devices::psg::sn76489_device;
	using sample_type = psg_device_type::sample_type;

	psg_device_type psg_;
};


