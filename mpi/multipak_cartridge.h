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
#include "multipak_cartridge_driver.h"
#include "configuration_dialog.h"
#include "multipak_configuration.h"
#include "vcc/bus/cartridge.h"
#include <array>


class multipak_cartridge : public ::vcc::bus::cartridge
{
public:

	using expansion_port_host_type = ::vcc::bus::expansion_port_host;
	using expansion_port_ui_type = ::vcc::bus::expansion_port_ui;
	using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
	using mount_status_type = ::vcc::utils::cartridge_loader_status;
	using slot_id_type = std::size_t;
	using path_type = std::string;
	using cartridge_ptr_type = std::shared_ptr<::vcc::bus::cartridge>;


public:

	multipak_cartridge(
		std::shared_ptr<expansion_port_host_type> host,
		std::shared_ptr<expansion_port_ui_type> ui,
		std::shared_ptr<expansion_port_bus_type> bus,
		std::shared_ptr<multipak_cartridge_driver> driver,
		HINSTANCE module_instance,
		multipak_configuration& configuration);
	multipak_cartridge(const multipak_cartridge&) = delete;
	multipak_cartridge(multipak_cartridge&&) = delete;

	multipak_cartridge& operator=(const multipak_cartridge&) = delete;
	multipak_cartridge& operator=(multipak_cartridge&&) = delete;

	//	Cartridge implementation
	name_type name() const override;
	[[nodiscard]] driver_type& driver() override;

	void start() override;
	void stop() override;

	void status(char* text_buffer, size_t buffer_size) override;
	void menu_item_clicked(unsigned char menu_item_id) override;

	menu_item_collection_type get_menu_items() const override;


private:

	mount_status_type insert_cartridge(slot_id_type slot, const path_type& filename);
	void eject_cartridge(slot_id_type slot);

	static const auto expansion_port_base_menu_id = 20u;
	static const auto expansion_port_menu_id_range_size = 20u;

	const std::shared_ptr<expansion_port_host_type> host_;
	const std::shared_ptr<expansion_port_ui_type> ui_;
	const std::shared_ptr<expansion_port_bus_type> bus_;
	const std::shared_ptr<multipak_cartridge_driver> driver_;
	const HINSTANCE module_instance_;
	multipak_configuration& configuration_;
	configuration_dialog settings_dialog_;
	std::array<cartridge_ptr_type, multipak_cartridge_driver::total_slot_count> cartridges_;
};
