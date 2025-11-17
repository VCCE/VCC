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
#include "configuration_dialog.h"
#include "vcc/devices/serial/beckerport.h"
#include "vcc/bus/cartridge.h"
#include "vcc/bus/cartridge_context.h"
#include <memory>
#include <Windows.h>


class becker_cartridge
	:
	public ::vcc::bus::cartridge,
	private ::cartridge_controller
{
public:

	using context_type = ::vcc::bus::cartridge_context;
	using becker_device_type = ::vcc::devices::serial::Becker;


public:

	becker_cartridge(
		std::unique_ptr<context_type> context,
		HINSTANCE module_instance);

	name_type name() const override;
	catalog_id_type catalog_id() const override;
	description_type description() const override;

	void start() override;
	void stop() override;

	void write_port(unsigned char port_id, unsigned char value) override;
	unsigned char read_port(unsigned char port_id) override;

	void status(char* text_buffer, size_t buffer_size) override;
	void menu_item_clicked(unsigned char menu_item_id) override;


protected:

	string_type server_address() const override;
	string_type server_port() const override;
	void set_server_address(
		const string_type& server_address,
		const string_type& server_port) override;

	void build_menu() const;


private:

	struct menu_item_ids
	{
		static const UINT open_configuration = 16;
	};

	struct mmio_ports
	{
		static const auto status = 0x41;
		static const auto data = 0x42;
	};

	static const inline std::string configuration_section_id_ = "DW Becker";

	const std::unique_ptr<context_type> context_;
	const HINSTANCE module_instance_;
	configuration_dialog configuration_dialog_;
	becker_device_type gBecker;
};
