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
#include "vcc/ui/menu/menu_builder.h"
#include "vcc/utils/winapi.h"
#include "vcc/utils/persistent_value_store.h"


becker_cartridge::becker_cartridge(
	std::shared_ptr<expansion_port_host_type> host,
	std::unique_ptr<expansion_port_ui_type> ui,
	std::unique_ptr<expansion_port_bus_type> bus,
	HINSTANCE module_instance)
	:
	host_(move(host)),
	ui_(move(ui)),
	bus_(move(bus)),
	module_instance_(module_instance),
	configuration_dialog_(
		module_instance,
		std::bind(&becker_cartridge::set_server_address, this, std::placeholders::_1, std::placeholders::_2))
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

becker_cartridge::device_type& becker_cartridge::device()
{
	return device_;
}


void becker_cartridge::start()
{
	device_.start(server_address(), server_port());
}


void becker_cartridge::stop()
{
	device_.stop();
}


void becker_cartridge::status(char* text_buffer, size_t buffer_size)
{
	// TODO-CHET: The becker port device should not be generating the status like it is
	// now. Update the device to provide properties that can be queried and used to
	// generate the status.
}


void becker_cartridge::menu_item_clicked(unsigned char menu_item_id)
{
	if (menu_item_id == menu_item_ids::open_configuration)
	{
		configuration_dialog_.open(server_address(), server_port());
	}
}


becker_cartridge::menu_item_collection_type becker_cartridge::get_menu_items() const
{
	return ::vcc::ui::menu::menu_builder()
		.add_root_item(menu_item_ids::open_configuration, "DriveWire Server..")
		.release_items();
}


becker_cartridge::string_type becker_cartridge::server_address() const
{
	::vcc::utils::persistent_value_store settings(host_->configuration_path());

	return settings.read(configuration_section_id_, "DWServerAddr", "127.0.0.1");
}

becker_cartridge::string_type becker_cartridge::server_port() const
{
	::vcc::utils::persistent_value_store settings(host_->configuration_path());

	return settings.read(configuration_section_id_, "DWServerPort", "65504");
}

void becker_cartridge::set_server_address(
	const string_type& server_address,
	const string_type& server_port)
{
	::vcc::utils::persistent_value_store settings(host_->configuration_path());

	settings.write(configuration_section_id_, "DWServerAddr", server_address);
	settings.write(configuration_section_id_, "DWServerPort", server_port);

	device_.set_server_address(server_address, server_port);
}
