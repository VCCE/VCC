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
#include <vcc/core/cartridges/basic_cartridge.h>
#include <vcc/core/cartridge_loader.h>
#include <vcc/core/utils/critical_section.h>
#include "../CartridgeMenu.h"
#include <array>


constexpr size_t NUMSLOTS = 4u;

class multipak_cartridge : public ::vcc::core::cartridge
{
public:

	using context_type = ::vcc::core::cartridge_context;
	using mount_status_type = ::vcc::core::cartridge_loader_status;
	using slot_id_type = ::std::size_t;
	using path_type = ::std::string;
	using label_type = ::std::string;
	using description_type = ::std::string;
	using menu_item_type = CartMenuItem;


public:

	explicit multipak_cartridge(std::shared_ptr<context_type> context);
	multipak_cartridge(const multipak_cartridge&) = delete;
	multipak_cartridge(multipak_cartridge&&) = delete;

	multipak_cartridge& operator=(const multipak_cartridge&) = delete;
	multipak_cartridge& operator=(multipak_cartridge&&) = delete;

	//	Cartridge implementation
	name_type name() const override;
	catalog_id_type catalog_id() const override;

	void start() override;
	void reset() override;
	void heartbeat() override;
	void write_port(unsigned char port_id, unsigned char value) override;
	unsigned char read_port(unsigned char port_id) override;
	unsigned char read_memory_byte(unsigned short memory_address) override;
	void status(char* text_buffer, size_t buffer_size) override;
	unsigned short sample_audio() override;
	void menu_item_clicked(unsigned char menu_item_id) override;

	//	Multi-pak implementation
	label_type slot_label(slot_id_type slot) const;
	description_type slot_description(slot_id_type slot) const;

	bool empty(slot_id_type slot) const;

	void eject_all();
	void eject_cartridge(slot_id_type slot);
	mount_status_type mount_cartridge(slot_id_type slot, const path_type& filename);

	void switch_to_slot(slot_id_type slot);
	slot_id_type selected_switch_slot() const;
	slot_id_type selected_scs_slot() const;

	void build_menu();

	// Make automatic when mounting, ejecting, selecting slot, etc.
	void save_configuration() const;
	void assert_cartridge_line(slot_id_type slot, bool line_state);
	void append_menu_item(slot_id_type slot, menu_item_type item);


private:

	void load_configuration();


private:
	
	static const size_t slot_select_port_id = 0x7f;
	static const size_t default_switch_slot_value = 0x03;
	static const size_t default_slot_register_value = 0xff;

	vcc::core::utils::critical_section mutex_;
	std::shared_ptr<context_type> context_;
	std::array<vcc::modules::mpi::cartridge_slot, NUMSLOTS> slots_;
	unsigned char slot_register_ = default_slot_register_value;
	slot_id_type switch_slot_ = default_switch_slot_value;
	slot_id_type cached_cts_slot_ = default_switch_slot_value;
	slot_id_type cached_scs_slot_ = default_switch_slot_value;
};
