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
#include <vcc/bus/cartridge.h>
#include <vcc/utils/critical_section.h>
#include "../CartridgeMenu.h"
#include <array>


class multipak_cartridge
	:
	public ::vcc::bus::cartridge,
	private multipak_controller
{
public:

	using context_type = ::vcc::bus::cartridge_context;
	using mount_status_type = ::vcc::utils::cartridge_loader_status;
	using slot_id_type = ::std::size_t;
	using path_type = ::std::string;
	using label_type = ::std::string;
	using description_type = ::std::string;
	using menu_item_type = CartMenuItem;


public:

	multipak_cartridge(
		HINSTANCE module_instance,
		multipak_configuration& configuration,
		std::shared_ptr<context_type> context,
		const cartridge_capi_context& capi_context);
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

	void process_horizontal_sync() override;
	unsigned short sample_audio() override;

	void status(char* text_buffer, size_t buffer_size) override;
	void menu_item_clicked(unsigned char menu_item_id) override;


protected:

	//	multipak controller implementation
	label_type slot_label(slot_id_type slot) const override;
	description_type slot_description(slot_id_type slot) const override;

	bool empty(slot_id_type slot) const override;

	void eject_cartridge(slot_id_type slot) override;
	mount_status_type mount_cartridge(slot_id_type slot, const path_type& filename) override;

	void switch_to_slot(slot_id_type slot) override;
	slot_id_type selected_switch_slot() const override;
	slot_id_type selected_scs_slot() const override;

	void build_menu() override;


protected:

	template<multipak_cartridge::slot_id_type SlotIndex_>
	static void assert_cartridge_line_on_slot(void* host_context, bool line_state)
	{
		static_cast<multipak_cartridge*>(host_context)->assert_cartridge_line(
			SlotIndex_,
			line_state);
	}

	template<multipak_cartridge::slot_id_type SlotIndex_>
	static void append_menu_item_on_slot(void* host_context, const char* text, int id, MenuItemType type)
	{
		static_cast<multipak_cartridge*>(host_context)->append_menu_item(
			SlotIndex_,
			{ text, static_cast<unsigned int>(id), type });
	}

	// Make automatic when mounting, ejecting, selecting slot, etc.
	void assert_cartridge_line(slot_id_type slot, bool line_state);
	void append_menu_item(slot_id_type slot, menu_item_type item);

	friend class multipak_cartridge_context;


private:
	
	static const size_t slot_select_port_id = 0x7f;
	static const size_t default_switch_slot_value = 0x03;
	static const size_t default_slot_register_value = 0xff;

	vcc::utils::critical_section mutex_;
	const HINSTANCE module_instance_;
	multipak_configuration& configuration_;
	std::shared_ptr<context_type> context_;
	const std::array<cartridge_capi_context, 4> capi_contexts_;
	std::array<vcc::modules::mpi::cartridge_slot, 4> slots_;
	unsigned char slot_register_ = default_slot_register_value;
	slot_id_type switch_slot_ = default_switch_slot_value;
	slot_id_type cached_cts_slot_ = default_switch_slot_value;
	slot_id_type cached_scs_slot_ = default_switch_slot_value;
	configuration_dialog settings_dialog_;
};
