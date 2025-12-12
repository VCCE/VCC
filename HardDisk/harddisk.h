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
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_port_host.h"
#include "vcc/bus/expansion_port_ui.h"
#include "vcc/bus/expansion_port_bus.h"
#include "vcc/devices/rom/rom_image.h"
#include <memory>
#include <Windows.h>


class vcc_hard_disk_cartridge : 
	public ::vcc::bus::cartridge,
	public ::vcc::bus::cartridge_driver
{
public:

	using size_type = ::vcc::bus::cartridge::size_type;	//	TODO-CHET: Delete this when device is removes as base class!
	using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
	using expansion_port_ui_type = ::vcc::bus::expansion_port_ui;
	using expansion_port_host_type = ::vcc::bus::expansion_port_host;
	using rom_image_type = ::vcc::devices::rom::rom_image;

	vcc_hard_disk_cartridge(
		std::shared_ptr<expansion_port_host_type> host,
		std::unique_ptr<expansion_port_ui_type> ui,
		std::unique_ptr<expansion_port_bus_type> bus,
		HINSTANCE module_instance);

	/// @inheritdoc
	name_type name() const override;

	[[nodiscard]] driver_type& driver() override;

	void start() override;
	void stop() override;

	void write_port(unsigned char port_id, unsigned char value) override;
	[[nodiscard]] unsigned char read_port(unsigned char port_id) override;

	status_type status() const override;
	void menu_item_clicked(menu_item_id_type menu_item_id) override;

	menu_item_collection_type get_menu_items() const override;


private:

	void LoadConfig();


private:

	const std::shared_ptr<expansion_port_host_type> host_;
	const std::unique_ptr<expansion_port_ui_type> ui_;
	const std::unique_ptr<expansion_port_bus_type> bus_;
	const HINSTANCE module_instance_;
};

