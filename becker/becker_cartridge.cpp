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
#include "becker_cartridge.h"
#include "resource.h"
#include "vcc/utils/winapi.h"
#include "vcc/utils/persistent_value_store.h"
#include "../CartridgeMenu.h"


becker_cartridge::becker_cartridge(
	std::unique_ptr<expansion_port_host_type> host,
	std::unique_ptr<expansion_port_ui_type> ui,
	std::unique_ptr<expansion_port_bus_type> bus,
	HINSTANCE module_instance)
	:
	host_(move(host)),
	ui_(move(ui)),
	bus_(move(bus)),
	module_instance_(module_instance),
	configuration_dialog_(module_instance, *this)
{
}


becker_cartridge::name_type becker_cartridge::name() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME);
}

becker_cartridge::catalog_id_type becker_cartridge::catalog_id() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_CATNUMBER);
}

becker_cartridge::description_type becker_cartridge::description() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_DESCRIPTION);
}


void becker_cartridge::start()
{
	::vcc::utils::persistent_value_store settings(host_->configuration_path());

	gBecker.sethost(
		settings.read(configuration_section_id_, "DWServerAddr", "127.0.0.1").c_str(),
		settings.read(configuration_section_id_, "DWServerPort", "65504").c_str());
	gBecker.enable(true);

	build_menu();
}


void becker_cartridge::stop()
{
	gBecker.enable(false);
	configuration_dialog_.close();
}


void becker_cartridge::write_port(unsigned char port_id, unsigned char value)
{
	if (port_id == mmio_ports::data)
	{
		gBecker.write(value, port_id);
	}
}

unsigned char becker_cartridge::read_port(unsigned char port_id)
{
	switch (port_id)
	{
	case mmio_ports::status:
		return gBecker.read(port_id) != 0 ? 2 : 0;

	case mmio_ports::data:
		return gBecker.read(port_id);
	}

	return 0;
}

void becker_cartridge::status(char* text_buffer, size_t buffer_size)
{
	// TODO-CHET: The becker port device should not be generating the status, that should
	// be done here. The FD502 implementation will need to be updated along with this.
	gBecker.status(text_buffer);
}


void becker_cartridge::menu_item_clicked(unsigned char menu_item_id)
{
	if (menu_item_id == menu_item_ids::open_configuration)
	{
		configuration_dialog_.open();
	}
}


void becker_cartridge::build_menu() const
{
	ui_->add_menu_item("", MID_BEGIN, MIT_Head);
	ui_->add_menu_item("", MID_ENTRY, MIT_Seperator);
	ui_->add_menu_item("DriveWire Server..", ControlId(menu_item_ids::open_configuration), MIT_StandAlone);
	ui_->add_menu_item("", MID_FINISH, MIT_Head);
}


becker_cartridge::string_type becker_cartridge::server_address() const
{
	return gBecker.server_address();
}

becker_cartridge::string_type becker_cartridge::server_port() const
{
	return gBecker.server_port();
}

void becker_cartridge::set_server_address(
	const string_type& server_address,
	const string_type& server_port)
{
	gBecker.sethost(server_address.c_str(), server_port.c_str());

	::vcc::utils::persistent_value_store settings(host_->configuration_path());

	settings.write(configuration_section_id_, "DWServerAddr", server_address);
	settings.write(configuration_section_id_, "DWServerPort", server_port);
}
