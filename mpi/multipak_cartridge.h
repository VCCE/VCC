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
#include "cartridge_slot.h"
#include "configuration_dialog.h"
#include "multipak_configuration.h"
#include "vcc/bus/cartridge.h"
#include "vcc/utils/critical_section.h"
#include <array>


class multipak_cartridge
	:
	public ::vcc::bus::cartridge,
	private multipak_controller
{
public:

	using expansion_port_host_type = ::vcc::bus::expansion_port_host;
	using expansion_port_ui_type = ::vcc::bus::expansion_port_ui;
	using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
	using mount_status_type = ::vcc::utils::cartridge_loader_status;
	using slot_id_type = std::size_t;
	using path_type = std::string;
	using label_type = std::string;
	using description_type = std::string;


public:

	multipak_cartridge(
		std::shared_ptr<expansion_port_host_type> host,
		std::unique_ptr<expansion_port_ui_type> ui,
		std::unique_ptr<expansion_port_bus_type> bus,
		HINSTANCE module_instance,
		multipak_configuration& configuration);
	multipak_cartridge(const multipak_cartridge&) = delete;
	multipak_cartridge(multipak_cartridge&&) = delete;

	multipak_cartridge& operator=(const multipak_cartridge&) = delete;
	multipak_cartridge& operator=(multipak_cartridge&&) = delete;

	//	Cartridge implementation
	name_type name() const override;
	catalog_id_type catalog_id() const override;
	description_type description() const override;

	void start() override;
	void stop() override;
	void reset() override;

	unsigned char read_memory_byte(size_type memory_address) override;

	void write_port(unsigned char port_id, unsigned char value) override;
	unsigned char read_port(unsigned char port_id) override;

	void update(float delta) override;
	unsigned short sample_audio() override;

	void status(char* text_buffer, size_t buffer_size) override;
	void menu_item_clicked(unsigned char menu_item_id) override;

	menu_item_collection_type get_menu_items() const override;


protected:

	static const auto expansion_port_base_menu_id = 20u;
	static const auto expansion_port_menu_id_range_size = 20u;


	//	multipak controller implementation
	label_type slot_label(slot_id_type slot) const override;
	description_type slot_description(slot_id_type slot) const override;

	bool empty(slot_id_type slot) const override;

	void eject_cartridge(slot_id_type slot) override;
	mount_status_type mount_cartridge(slot_id_type slot, const path_type& filename) override;

	void switch_to_slot(slot_id_type slot) override;
	slot_id_type selected_switch_slot() const override;
	slot_id_type selected_scs_slot() const override;


protected:

	// Make automatic when mounting, ejecting, selecting slot, etc.
	void set_cartridge_select_line(slot_id_type slot, bool line_state);

	friend class multipak_expansion_slot_host;
	friend class multipak_expansion_slot_ui;
	friend class multipak_expansion_slot_bus;


private:
	
	static const size_t slot_select_port_id = 0x7f;
	static const size_t default_switch_slot_value = 0x03;
	static const size_t default_slot_register_value = 0xff;

	vcc::utils::critical_section mutex_;
	const std::shared_ptr<expansion_port_host_type> host_;
	const std::unique_ptr<expansion_port_ui_type> ui_;
	const std::shared_ptr<expansion_port_bus_type> bus_;
	const HINSTANCE module_instance_;
	multipak_configuration& configuration_;
	std::array<::vcc::modules::mpi::cartridge_slot, 4> slots_;
	unsigned char slot_register_ = default_slot_register_value;
	slot_id_type switch_slot_ = default_switch_slot_value;
	slot_id_type cached_cts_slot_ = default_switch_slot_value;
	slot_id_type cached_scs_slot_ = default_switch_slot_value;
	configuration_dialog settings_dialog_;
};
