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
#include "becker_cartridge.h"
#include "resource.h"
#include "vcc/ui/menu/menu_builder.h"
#include "vcc/utils/winapi.h"
#include "vcc/utils/persistent_value_section_store.h"
#include <stdexcept>


namespace vcc::cartridges::becker_port
{

	becker_cartridge::becker_cartridge(
		std::shared_ptr<expansion_port_host_type> host,
		std::shared_ptr<expansion_port_ui_type> ui,
		HINSTANCE module_instance)
		:
		host_(move(host)),
		module_instance_(module_instance),
		configuration_dialog_(
			module_instance,
			ui,
			std::bind(&becker_cartridge::update_connection_settings, this, std::placeholders::_1, std::placeholders::_2))
	{
		if (host_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Becker Port Cartridge. The host pointer is null.");
		}

		if (module_instance_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Becker Port Cartridge. The module handle is null.");
		}
	}


	becker_cartridge::name_type becker_cartridge::name() const
	{
		return ::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME);
	}

	becker_cartridge::driver_type& becker_cartridge::driver()
	{
		return driver_;
	}


	void becker_cartridge::start()
	{
		driver_.start(server_address_setting(), server_port_setting());
	}


	void becker_cartridge::stop()
	{
		driver_.stop();
	}


	void becker_cartridge::status(char* text_buffer, size_t buffer_size)
	{
		// TODO-CHET: The becker port device should not be generating the status like it is
		// now. Update the device to provide properties that can be queried and used to
		// generate the status.
	}


	void becker_cartridge::menu_item_clicked(menu_item_id_type menu_item_id)
	{
		if (menu_item_id == menu_item_ids::open_settings)
		{
			configuration_dialog_.open(server_address_setting(), server_port_setting());
		}
	}


	becker_cartridge::menu_item_collection_type becker_cartridge::get_menu_items() const
	{
		return ::vcc::ui::menu::menu_builder()
			.add_root_item(menu_item_ids::open_settings, "Becker Port Settings")
			.release_items();
	}


	becker_cartridge::string_type becker_cartridge::server_address_setting() const
	{
		::vcc::utils::persistent_value_store settings(host_->configuration_path());

		// FIXME-CHET: This needs to validate the server address is in a valid format
		return settings.read(
			configuration::section,
			configuration::keys::server_address,
			configuration::defaults::server_address);
	}

	becker_cartridge::string_type becker_cartridge::server_port_setting() const
	{
		::vcc::utils::persistent_value_store settings(host_->configuration_path());

		// FIXME-CHET: This needs to validate the server port
		return settings.read(
			configuration::section,
			configuration::keys::server_port,
			configuration::defaults::server_port);
	}

	void becker_cartridge::update_connection_settings(
		const string_type& server_address,
		const string_type& server_port)
	{
		if (server_address.empty())
		{
			throw std::invalid_argument("Cannot update Becker Port connection settings. The server address is empty.");
		}

		if (server_port.empty())
		{
			throw std::invalid_argument("Cannot update Becker Port connection settings. The server address is empty.");
		}

		::vcc::utils::persistent_value_section_store settings(
			host_->configuration_path(),
			configuration::section);

		settings.write(configuration::keys::server_address, server_address);
		settings.write(configuration::keys::server_port, server_port);

		driver_.update_connection_settings(server_address, server_port);
	}

}
