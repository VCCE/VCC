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
#include "gmc_device.h"
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_port_host.h"
#include "vcc/bus/expansion_port_ui.h"
#include "vcc/bus/expansion_port_bus.h"
#include <Windows.h>
#include <memory>


class gmc_cartridge : public ::vcc::bus::cartridge
{
public:

	using size_type = ::vcc::bus::cartridge::size_type;	//	FIXME-CHET: Delete this when device is removes as base class!
	using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
	using expansion_port_ui_type = ::vcc::bus::expansion_port_ui;
	using expansion_port_host_type = ::vcc::bus::expansion_port_host;
	using path_type = std::string;
	using rom_image_type = ::vcc::devices::rom::banked_rom_image;
	using device_type = gmc_device;

public:

	gmc_cartridge(
		std::shared_ptr<expansion_port_host_type> host,
		std::unique_ptr<expansion_port_ui_type> ui,
		std::shared_ptr<expansion_port_bus_type> bus,
		HINSTANCE module_instance);

	name_type name() const override;
	[[nodiscard]] device_type& device() override;

	void start() override;

	void status(char* status_buffer, size_t buffer_size) override;
	void menu_item_clicked(unsigned char menu_item_id) override;

	menu_item_collection_type get_menu_items() const override;


private:

	struct menu_item_ids
	{
		static const unsigned int select_rom = 3;
	};

	static const path_type configuration_section_id_;
	static const path_type configuration_rom_key_id_;


	const std::shared_ptr<expansion_port_host_type> host_;
	const std::unique_ptr<expansion_port_ui_type> ui_;
	const std::shared_ptr<expansion_port_bus_type> bus_;
	const HINSTANCE module_instance_;

	device_type device_;
};


