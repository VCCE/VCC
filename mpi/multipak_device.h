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
#include "multipak_expansion_port.h"
#include "multipak_configuration.h"
#include "vcc/bus/cartridge_device.h"
#include "vcc/utils/critical_section.h"
#include <array>


class multipak_device : public ::vcc::bus::cartridge_device
{
public:

	using expansion_port_host_type = ::vcc::bus::expansion_port_host;
	using expansion_port_ui_type = ::vcc::bus::expansion_port_ui;
	using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
	using mount_status_type = ::vcc::utils::cartridge_loader_status;
	using slot_id_type = std::size_t;
	using name_type = std::string;
	using size_type = std::size_t;
	using cartridge_type = ::vcc::bus::cartridge;
	using cartridge_ptr_type = std::shared_ptr<cartridge_type>;
	using handle_type = std::unique_ptr<std::remove_pointer_t<HMODULE>, vcc::utils::dll_deleter>;

	static constexpr size_type total_slot_count = 4;

public:

	multipak_device(
		std::shared_ptr<expansion_port_bus_type> bus,
		const multipak_configuration& configuration);
	multipak_device(const multipak_device&) = delete;
	multipak_device(multipak_device&&) = delete;

	multipak_device& operator=(const multipak_device&) = delete;
	multipak_device& operator=(multipak_device&&) = delete;

	/// @inheritdoc
	void start() /*override*/;

	/// @inheritdoc
	void stop() /*override*/;

	/// @inheritdoc
	void reset() override;

	bool empty(slot_id_type slot) const;

	size_type slot_count() const
	{
		return slots_.size();
	}

	name_type slot_name(slot_id_type slot) const;

	void switch_to_slot(slot_id_type slot);
	slot_id_type selected_switch_slot() const;
	slot_id_type selected_scs_slot() const;
	slot_id_type selected_cts_slot() const;

	void insert_cartridge(
		slot_id_type slot,
		handle_type handle,
		cartridge_ptr_type cartridge);

	void eject_cartridge(slot_id_type slot);

	/// @inheritdoc
	unsigned char read_memory_byte(size_type memory_address) override;

	/// @inheritdoc
	void write_port(unsigned char port_id, unsigned char value) override;

	/// @inheritdoc
	unsigned char read_port(unsigned char port_id) override;

	/// @inheritdoc
	void update(float delta) override;

	/// @inheritdoc
	unsigned short sample_audio() override;


protected:

	// Make automatic when mounting, ejecting, selecting slot, etc.
	void set_cartridge_select_line(slot_id_type slot, bool line_state);

	friend class multipak_expansion_port_host;
	friend class multipak_expansion_port_ui;
	friend class multipak_expansion_port_bus;


private:

	static const size_t slot_select_port_id = 0x7f;
	static const size_t default_switch_slot_value = 0x03;
	static const size_t default_slot_register_value = 0xff;

	vcc::utils::critical_section mutex_;
	const std::shared_ptr<expansion_port_bus_type> bus_;
	const multipak_configuration& configuration_;
	std::array<multipak_expansion_port, total_slot_count> slots_;
	unsigned char slot_register_ = default_slot_register_value;
	slot_id_type switch_slot_ = default_switch_slot_value;
	slot_id_type cached_cts_slot_ = default_switch_slot_value;
	slot_id_type cached_scs_slot_ = default_switch_slot_value;
};
