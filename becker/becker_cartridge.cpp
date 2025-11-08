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
#include <vcc/utils/winapi.h>
#include <vcc/utils/configuration_serializer.h>
#include "../CartridgeMenu.h"

// Contains Becker cart exports

becker_cartridge::becker_cartridge(std::unique_ptr<context_type> context, HINSTANCE module_instance)
	:
	context_(move(context)),
	module_instance_(module_instance),
	configuration_dialog_(module_instance, *this)
{}


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

//  Load becker config
	::vcc::utils::configuration_serializer serializer(context_->configuration_path());

	gBecker.sethost(
		serializer.read(configuration_section_id_, "DWServerAddr", "127.0.0.1").c_str(),
		serializer.read(configuration_section_id_, "DWServerPort", "65504").c_str());
	gBecker.enable(true);

//  Create dynamic menu
	build_menu();
}


void becker_cartridge::stop()
{
	gBecker.enable(false);
	configuration_dialog_.close();
}


void becker_cartridge::write_port(unsigned char port_id, unsigned char value)
{
	if (port_id == 0x42)
	{
		gBecker.write(value,port_id);
	}
}

unsigned char becker_cartridge::read_port(unsigned char port_id)
{
	switch (port_id)
	{
	case 0x41:	// read status
		return gBecker.read(port_id) != 0 ? 2 : 0;

	case 0x42:	// read data
		return gBecker.read(port_id);
	}

	return 0;
}

void becker_cartridge::status(char* text_buffer, size_t buffer_size)
{
	gBecker.status(text_buffer);  // text buffer size??
}


void becker_cartridge::menu_item_clicked(unsigned char menu_item_id)
{
	if (menu_item_id == menu_identifiers::open_configuration)
	{
		configuration_dialog_.open();
	}
}


void becker_cartridge::build_menu() const
{
	context_->add_menu_item("", MID_BEGIN, MIT_Head);
	context_->add_menu_item("", MID_ENTRY, MIT_Seperator);
	context_->add_menu_item("DriveWire Server..", ControlId(menu_identifiers::open_configuration), MIT_StandAlone);
	context_->add_menu_item("", MID_FINISH, MIT_Head);
}


becker_cartridge::string_type becker_cartridge::server_address() const
{
	return gBecker.server_address();
}

becker_cartridge::string_type becker_cartridge::server_port() const
{
	return gBecker.server_port();
}

// Save becker config
void becker_cartridge::configure_server(string_type server_address, string_type server_port)
{
	gBecker.sethost(server_address.c_str(), server_port.c_str());

	::vcc::utils::configuration_serializer serializer(context_->configuration_path());

	serializer.write(configuration_section_id_, "DWServerAddr", server_address);
	serializer.write(configuration_section_id_, "DWServerPort", server_port);
}
