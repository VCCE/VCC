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
#include "multipak_cartridge.h"
#include "multipak_expansion_slot_ui.h"
#include "multipak_expansion_slot_bus.h"
#include "resource.h"
#include <vcc/ui/menu/menu_builder.h>
#include "vcc/utils/winapi.h"
#include "vcc/utils/filesystem.h"


namespace
{

	constexpr std::pair<size_t, size_t> disk_controller_io_port_range = { 0x40, 0x5f };

	// Is a port a disk port?
	constexpr bool is_disk_controller_port(multipak_cartridge::slot_id_type port)
	{
		return port >= disk_controller_io_port_range.first && port <= disk_controller_io_port_range.second;
	}

}


multipak_cartridge::multipak_cartridge(
	std::shared_ptr<expansion_port_host_type> host,
	std::unique_ptr<expansion_port_ui_type> ui,
	std::unique_ptr<expansion_port_bus_type> bus,
	HINSTANCE module_instance,
	multipak_configuration& configuration)
	:
	host_(move(host)),
	ui_(move(ui)),
	bus_(move(bus)),
	module_instance_(module_instance),
	configuration_(configuration),
	settings_dialog_(module_instance, configuration, *this)
{ }


multipak_cartridge::name_type multipak_cartridge::name() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME);
}

multipak_cartridge::catalog_id_type multipak_cartridge::catalog_id() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_CATNUMBER);
}

multipak_cartridge::description_type multipak_cartridge:: description() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_CATNUMBER);
}

multipak_cartridge::device_type& multipak_cartridge::device()
{
	return *this;
}


void multipak_cartridge::start()
{
	// Get the startup slot and set Chip select and SCS slots from ini file
	switch_slot_ = configuration_.selected_slot();
	cached_cts_slot_ = switch_slot_;
	cached_scs_slot_ = switch_slot_;

	// Mount them
	for (auto slot(0u); slot < slots_.size(); slot++)
	{
		const auto path(vcc::utils::find_pak_module_path(configuration_.slot_cartridge_path(slot)));
		if (!path.empty())
		{
			mount_cartridge(slot, path);
		}
	}
}


void multipak_cartridge::stop()
{
	settings_dialog_.close();

	for (auto slot(0u); slot < slots_.size(); slot++)
	{
		eject_cartridge(slot);
	}
}

void multipak_cartridge::reset()
{
	vcc::utils::section_locker lock(mutex_);

	cached_cts_slot_ = switch_slot_;
	cached_scs_slot_ = switch_slot_;
	for (const auto& cartridge_slot : slots_)
	{
		cartridge_slot.reset();
	}

	bus_->set_cartridge_select_line(slots_[cached_scs_slot_].line_state());
}

void multipak_cartridge::update(float delta)
{
	vcc::utils::section_locker lock(mutex_);

	for(const auto& cartridge_slot : slots_)
	{
		cartridge_slot.update(delta);
	}
}

void multipak_cartridge::write_port(unsigned char port_id, unsigned char value)
{
	vcc::utils::section_locker lock(mutex_);

	if (port_id == slot_select_port_id)
	{
		cached_scs_slot_ = value & 0b00000011;			// SCS
		cached_cts_slot_ = (value >> 4) & 0b00000011;	// CTS
		slot_register_ = value;

		bus_->set_cartridge_select_line(slots_[cached_scs_slot_].line_state());

		return;
	}

	// Only write disk ports (0x40-0x5F) if SCS is set
	if (is_disk_controller_port(port_id))
	{
		slots_[cached_scs_slot_].write_port(port_id, value);
		return;
	}

	for (const auto& cartridge_slot : slots_)
	{
		cartridge_slot.write_port(port_id, value);
	}
}

unsigned char multipak_cartridge::read_port(unsigned char port_id)
{
	vcc::utils::section_locker lock(mutex_);

	if (port_id == slot_select_port_id)	// Self
	{
		slot_register_ &= 0b11001100;
		slot_register_ |= cached_scs_slot_ | (cached_cts_slot_ << 4);

		return slot_register_;
	}

	// Only read disk ports (0x40-0x5F) if SCS is set
	if (is_disk_controller_port(port_id))
	{
		return slots_[cached_scs_slot_].read_port(port_id);
	}

	for (const auto& cartridge_slot : slots_)
	{
		// Return value from first module that returns non zero
		// FIXME-CHET: Shouldn't this OR all the values together?
		const auto data(cartridge_slot.read_port(port_id));
		if (data != 0)
		{
			return data;
		}
	}

	return 0;
}

unsigned char multipak_cartridge::read_memory_byte(size_type memory_address)
{
	vcc::utils::section_locker lock(mutex_);

	return slots_[cached_cts_slot_].read_memory_byte(memory_address);
}

void multipak_cartridge::status(char* text_buffer, size_t buffer_size)
{
	vcc::utils::section_locker lock(mutex_);

	char TempStatus[64] = "";

	sprintf(text_buffer, "MPI:%d,%d", cached_cts_slot_ + 1, cached_scs_slot_ + 1);
	for (const auto& cartridge_slot : slots_)
	{
		strcpy(TempStatus, "");
		cartridge_slot.status(TempStatus, sizeof(TempStatus));
		if (TempStatus[0])
		{
			strcat(text_buffer, " | ");
			strcat(text_buffer, TempStatus);
		}
	}
}

unsigned short multipak_cartridge::sample_audio()
{
	vcc::utils::section_locker lock(mutex_);

	unsigned short sample = 0;
	for (const auto& cartridge_slot : slots_)
	{
		sample += cartridge_slot.sample_audio();
	}

	return sample;
}

void multipak_cartridge::menu_item_clicked(unsigned char menu_item_id)
{
	if (menu_item_id == 19)	//MPI Config
	{
		settings_dialog_.open();
		return;
	}

	vcc::utils::section_locker lock(mutex_);

	//Configs for loaded carts
	if (menu_item_id >= 20 && menu_item_id <= 40)
	{
		slots_[0].menu_item_clicked(menu_item_id - 20);
	}

	if (menu_item_id > 40 && menu_item_id <= 60)
	{
		slots_[1].menu_item_clicked(menu_item_id - 40);
	}

	if (menu_item_id > 60 && menu_item_id <= 80)
	{
		slots_[2].menu_item_clicked(menu_item_id - 60);
	}

	if (menu_item_id > 80 && menu_item_id <= 100)
	{
		slots_[3].menu_item_clicked(menu_item_id - 80);
	}
}


multipak_cartridge::label_type multipak_cartridge::slot_label(slot_id_type slot) const
{
	vcc::utils::section_locker lock(mutex_);

	return slots_[slot].label();
}

multipak_cartridge::description_type multipak_cartridge::slot_description(slot_id_type slot) const
{
	vcc::utils::section_locker lock(mutex_);

	return slots_[slot].description();
}

bool multipak_cartridge::empty(slot_id_type slot) const
{
	vcc::utils::section_locker lock(mutex_);

	return slots_[slot].empty();
}

void multipak_cartridge::eject_cartridge(slot_id_type slot)
{
	vcc::utils::section_locker lock(mutex_);

	slots_[slot].stop();

	slots_[slot] = {};
}

multipak_cartridge::mount_status_type multipak_cartridge::mount_cartridge(
	slot_id_type slot,
	const path_type& filename)
{
	auto loadedCartridge(vcc::utils::load_cartridge(
		filename,
		host_,
		std::make_unique<multipak_expansion_slot_ui>(),
		std::make_unique<multipak_expansion_slot_bus>(slot, *bus_, *this)));
	if (loadedCartridge.load_result != mount_status_type::success)
	{
		return loadedCartridge.load_result;
	}

	// FIXME-CHET: We should probably call eject(slot) here in order to ensure that the
	// cartridge is shut down correctly.

	vcc::utils::section_locker lock(mutex_);

	slots_[slot] = {
		filename,
		move(loadedCartridge.handle),
		move(loadedCartridge.cartridge)
	};

	slots_[slot].start();
	slots_[slot].reset();

	return loadedCartridge.load_result;
}

void multipak_cartridge::switch_to_slot(slot_id_type slot)
{
	vcc::utils::section_locker lock(mutex_);

	switch_slot_ = slot;
	cached_scs_slot_ = slot;
	cached_cts_slot_ = slot;

	bus_->set_cartridge_select_line(slots_[slot].line_state());
}

multipak_cartridge::slot_id_type multipak_cartridge::selected_switch_slot() const
{
	return switch_slot_;
}

multipak_cartridge::slot_id_type multipak_cartridge::selected_scs_slot() const
{
	return cached_scs_slot_;
}

// This gets called any time a cartridge menu is changed. It draws the entire menu.
multipak_cartridge::menu_item_collection_type multipak_cartridge::get_menu_items() const
{
	vcc::utils::section_locker lock(mutex_);

	::vcc::ui::menu::menu_builder builder;

	for (int slot(slots_.size() - 1); slot >= 0; slot--)
	{
		const auto items(slots_[slot].get_menu_items());
		if (!items.empty())
		{
			if (!builder.empty())
			{
				builder.add_root_separator();
			}

			builder.add_items(
				items,
				expansion_port_base_menu_id + slot * expansion_port_menu_id_range_size);
		}
	}

	if (!builder.empty())
	{
		builder.add_root_separator();
	}

	builder.add_root_item(19, "MPI Config");
	return builder.release_items();
}


void multipak_cartridge::set_cartridge_select_line(slot_id_type slot, bool line_state)
{
	vcc::utils::section_locker lock(mutex_);

	slots_[slot].line_state(line_state);
	if (selected_scs_slot() == slot)
	{
		bus_->set_cartridge_select_line(slots_[slot].line_state());
	}
}
