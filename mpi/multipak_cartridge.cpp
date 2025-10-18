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
#include "multipak_cartridge_context.h"
#include "mpi.h"
#include "resource.h"
#include <vcc/core/utils/configuration_serializer.h>
#include <vcc/core/utils/winapi.h>
#include <vcc/core/utils/filesystem.h>


namespace
{

	struct cartridge_slot_callbacks
	{
		AssertCartridgeLineModuleCallback set_cartridge_line;
		AppendCartridgeMenuModuleCallback append_menu_item;
	};

	//**************************************************************
	const std::array<cartridge_slot_callbacks, NUMSLOTS> gSlotCallbacks = { {
		{ assert_cartridge_line_on_slot<0, gMultiPakInterface>, append_menu_item_on_slot<0, gMultiPakInterface> },
		{ assert_cartridge_line_on_slot<1, gMultiPakInterface>, append_menu_item_on_slot<1, gMultiPakInterface> },
		{ assert_cartridge_line_on_slot<2, gMultiPakInterface>, append_menu_item_on_slot<2, gMultiPakInterface> },
		{ assert_cartridge_line_on_slot<3, gMultiPakInterface>, append_menu_item_on_slot<3, gMultiPakInterface> }
	} };

	constexpr std::pair<size_t, size_t> disk_controller_io_port_range = { 0x40, 0x5f };

	// Is a port a disk port?
	constexpr bool is_disk_controller_port(multipak_cartridge::slot_id_type port)
	{
		return port >= disk_controller_io_port_range.first && port <= disk_controller_io_port_range.second;
	}

}

template<multipak_cartridge::slot_id_type SlotIndex_, multipak_cartridge& cartridge>
void assert_cartridge_line_on_slot(bool line_state)
{
	cartridge.assert_cartridge_line(SlotIndex_, line_state);
}

template<multipak_cartridge::slot_id_type SlotIndex_, multipak_cartridge& cartridge>
void append_menu_item_on_slot(const char* text, int id, MenuItemType type)
{
	cartridge.append_menu_item(SlotIndex_, { text, static_cast<unsigned int>(id), type });
}

multipak_cartridge::multipak_cartridge(std::shared_ptr<context_type> context)
	: context_(move(context))
{ }

multipak_cartridge::name_type multipak_cartridge::name() const
{
	return ::vcc::core::utils::load_string(gModuleInstance, IDS_MODULE_NAME);
}

multipak_cartridge::catalog_id_type multipak_cartridge::catalog_id() const
{
	return ::vcc::core::utils::load_string(gModuleInstance, IDS_CATNUMBER);
}

void multipak_cartridge::start()
{
	load_configuration();
}

void multipak_cartridge::reset()
{
	vcc::core::utils::section_locker lock(mutex_);

	cached_cts_slot_ = switch_slot_;
	cached_scs_slot_ = switch_slot_;
	for (const auto& cartridge_slot : slots_)
	{
		cartridge_slot.reset();
	}

	context_->assert_cartridge_line(slots_[cached_scs_slot_].line_state());
}

void multipak_cartridge::heartbeat()
{
	vcc::core::utils::section_locker lock(mutex_);

	for(const auto& cartridge_slot : slots_)
	{
		cartridge_slot.heartbeat();
	}
}

void multipak_cartridge::write_port(unsigned char port_id, unsigned char value)
{
	vcc::core::utils::section_locker lock(mutex_);

	if (port_id == slot_select_port_id)
	{
		cached_scs_slot_ = value & 0b00000011;			// SCS
		cached_cts_slot_ = (value >> 4) & 0b00000011;	// CTS
		slot_register_ = value;

		context_->assert_cartridge_line(slots_[cached_scs_slot_].line_state());

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
	vcc::core::utils::section_locker lock(mutex_);

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
		// FIXME: Shouldn't this OR all the values together?
		const auto data(cartridge_slot.read_port(port_id));
		if (data != 0)
		{
			return data;
		}
	}

	return 0;
}

unsigned char multipak_cartridge::read_memory_byte(unsigned short memory_address)
{
	vcc::core::utils::section_locker lock(mutex_);

	return slots_[cached_cts_slot_].read_memory_byte(memory_address);
}

void multipak_cartridge::status(char* status_buffer)
{
	vcc::core::utils::section_locker lock(mutex_);

	char TempStatus[64] = "";

	sprintf(status_buffer, "MPI:%d,%d", cached_cts_slot_ + 1, cached_scs_slot_ + 1);
	for (const auto& cartridge_slot : slots_)
	{
		strcpy(TempStatus, "");
		cartridge_slot.status(TempStatus);
		if (TempStatus[0])
		{
			strcat(status_buffer, " | ");
			strcat(status_buffer, TempStatus);
		}
	}
}

unsigned short multipak_cartridge::sample_audio()
{
	vcc::core::utils::section_locker lock(mutex_);

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
		gConfigurationDialog.open();
	}

	vcc::core::utils::section_locker lock(mutex_);

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
	vcc::core::utils::section_locker lock(mutex_);

	return slots_[slot].label();
}

multipak_cartridge::description_type multipak_cartridge::slot_description(slot_id_type slot) const
{
	vcc::core::utils::section_locker lock(mutex_);

	return "Module Name: " + slots_[slot].name();
}

bool multipak_cartridge::empty(slot_id_type slot) const
{
	vcc::core::utils::section_locker lock(mutex_);

	return slots_[slot].empty();
}

// FIXME: refactor this into a stop that also shuts down the configuration dialog
void multipak_cartridge::eject_all()
{
	for (auto slot(0u); slot < slots_.size(); slot++)
	{
		eject_cartridge(slot);
	}
}

void multipak_cartridge::eject_cartridge(slot_id_type slot)
{
	vcc::core::utils::section_locker lock(mutex_);

	slots_[slot] = {};
}

multipak_cartridge::mount_status_type multipak_cartridge::mount_cartridge(
	slot_id_type slot,
	const path_type& filename)
{
	auto loadedCartridge(vcc::core::load_cartridge(
		std::make_unique<multipak_cartridge_context>(slot, *context_, *this),
		{
			gSlotCallbacks[slot].append_menu_item,
			gHostContext->read_memory_byte_,
			gHostContext->write_memory_byte_,
			gHostContext->assert_interrupt_,
			gSlotCallbacks[slot].set_cartridge_line
		},
		filename,
		context_->configuration_path()));
	if (loadedCartridge.load_result != mount_status_type::success)
	{
		return loadedCartridge.load_result;
	}

	// FIXME: We should probably call eject(slot) here in order to ensure that the
	// cartridge is shut down correctly.

	vcc::core::utils::section_locker lock(mutex_);

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
	vcc::core::utils::section_locker lock(mutex_);

	switch_slot_ = slot;
	cached_scs_slot_ = slot;
	cached_cts_slot_ = slot;

	context_->assert_cartridge_line(slots_[slot].line_state());
}

multipak_cartridge::slot_id_type multipak_cartridge::selected_switch_slot() const
{
	return switch_slot_;
}

multipak_cartridge::slot_id_type multipak_cartridge::selected_scs_slot() const
{
	return cached_scs_slot_;
}

// Save cart Menu items into containers per slot
void multipak_cartridge::append_menu_item(slot_id_type slot, menu_item_type item)
{
	switch (item.menu_id) {
	case MID_BEGIN:
	{
		vcc::core::utils::section_locker lock(mutex_);

		slots_[slot].reset_menu();
	}
	break;

	case MID_FINISH:
		build_menu();
		break;

	default:
		// Add 20 times the slot number to control id's
		if (item.menu_id >= MID_CONTROL)
		{
			item.menu_id += (slot + 1) * 20;
		}
		{
			vcc::core::utils::section_locker lock(mutex_);

			slots_[slot].append_menu_item(item);
		}
		break;
	}
}

// This gets called any time a cartridge menu is changed. It draws the entire menu.
void multipak_cartridge::build_menu()
{
	vcc::core::utils::section_locker lock(mutex_);

	// Init the menu, establish MPI config control, build slot menus, then draw it.
	context_->add_menu_item("", MID_BEGIN, MIT_Head);
	context_->add_menu_item("", MID_ENTRY, MIT_Seperator);
	context_->add_menu_item("MPI Config", ControlId(19), MIT_StandAlone);
	for (int slot = 3; slot >= 0; slot--)
	{
		slots_[slot].enumerate_menu_items(*context_);
	}
	context_->add_menu_item("", MID_FINISH, MIT_Head);  // Finish draws the entire menu
}


void multipak_cartridge::load_configuration()
{
	const auto& section(name());
	::vcc::core::configuration_serializer serializer(context_->configuration_path());

	// Get the startup slot and set Chip select and SCS slots from ini file
	switch_slot_ = serializer.read(section, "SWPOSITION", 3);
	cached_cts_slot_ = switch_slot_;
	cached_scs_slot_ = switch_slot_;

	// Mount them
	for (auto slot(0u); slot < slots_.size(); slot++)
	{
		const auto path(vcc::core::utils::find_pak_module_path(serializer.read(section, "SLOT" + std::to_string(slot + 1))));
		if (!path.empty())
		{
			mount_cartridge(slot, path);
		}
	}

	// Build the dynamic menu
	build_menu();
}

void multipak_cartridge::save_configuration() const
{
	vcc::core::utils::section_locker lock(mutex_);

	const auto& section(name());
	::vcc::core::configuration_serializer serializer(context_->configuration_path());

	serializer.write(section, "SWPOSITION", switch_slot_);
	for (auto slot(0u); slot < slots_.size(); slot++)
	{
		serializer.write(
			section,
			"SLOT" + ::std::to_string(slot + 1),
			vcc::core::utils::strip_application_path(slots_[slot].path()));
	}
}

void multipak_cartridge::assert_cartridge_line(slot_id_type slot, bool line_state)
{
	vcc::core::utils::section_locker lock(mutex_);

	slots_[slot].line_state(line_state);
	if (selected_scs_slot() == slot)
	{
		context_->assert_cartridge_line(slots_[slot].line_state());
	}
}
