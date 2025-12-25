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
//#define USE_LOGGING
#include "multipak_cartridge.h"
#include "multipak_cartridge_context.h"
#include "mpi.h"
#include "resource.h"
#include <vcc/core/winapi.h>
#include <vcc/core/filesystem.h>
#include <vcc/core/logger.h>

namespace
{
	// Callback trampolines
	// host_context is the opaque callback_context provided for this cartridge instance.
	// In multipak this is always a multipak_cartridge*, allowing the trampoline to
	// bounce the operation to the correct slot without exposing host internals.
	template<multipak_cartridge::slot_id_type SlotIndex_>
	void assert_cartridge_line_on_slot(void* host_context, bool line_state)
	{
		static_cast<multipak_cartridge*>(host_context)->assert_cartridge_line(
			SlotIndex_,
			line_state);
	}
	template<multipak_cartridge::slot_id_type SlotIndex_>
	void append_menu_item_on_slot(void* host_context, const char* text, int id, MenuItemType type)
	{
		static_cast<multipak_cartridge*>(host_context)->append_menu_item(
			SlotIndex_,
			{ text, static_cast<unsigned int>(id), type });
	}

	// Per-slot callback table.
	// Each entry provides the host-side trampoline functions for that slot,
	// allowing cartridges to call back into multipak using a uniform API
	// while the trampolines route the request to the correct slot.
	// Multipak uses two slot specific callbacks, the rest are just passed from vcc.
	struct cartridge_slot_callbacks
	{
		PakAssertCartridgeLineHostCallback set_cartridge_line;
		PakAppendCartridgeMenuHostCallback append_menu_item;
	};
	const std::array<cartridge_slot_callbacks, NUMSLOTS> gSlotCallbacks = { {
		{ assert_cartridge_line_on_slot<0>, append_menu_item_on_slot<0> },
		{ assert_cartridge_line_on_slot<1>, append_menu_item_on_slot<1> },
		{ assert_cartridge_line_on_slot<2>, append_menu_item_on_slot<2> },
		{ assert_cartridge_line_on_slot<3>, append_menu_item_on_slot<3> }
	} };

	// Is a port a disk port?
	constexpr std::pair<size_t, size_t> disk_controller_io_port_range = { 0x40, 0x5f };
	constexpr bool is_disk_controller_port(multipak_cartridge::slot_id_type port)
	{
		return port >= disk_controller_io_port_range.first && port <= disk_controller_io_port_range.second;
	}

}

multipak_cartridge::multipak_cartridge(
	multipak_configuration& configuration,
	std::shared_ptr<context_type> context)
	:
	configuration_(configuration),
	context_(move(context))
{ }

multipak_cartridge::name_type multipak_cartridge::name() const
{
	return ::vcc::core::utils::load_string(gModuleInstance, IDS_MODULE_NAME);
}

multipak_cartridge::catalog_id_type multipak_cartridge::catalog_id() const
{
	return ::vcc::core::utils::load_string(gModuleInstance, IDS_CATNUMBER);
}

multipak_cartridge::description_type multipak_cartridge:: description() const
{
	return ::vcc::core::utils::load_string(gModuleInstance, IDS_CATNUMBER);
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
		const auto path(vcc::core::utils::find_pak_module_path(configuration_.slot_cartridge_path(slot)));
		if (!path.empty())
		{
			mount_cartridge(slot, path);
		}
	}

	// Build the dynamic menu
	build_menu();
}


void multipak_cartridge::stop()
{
	gConfigurationDialog.close();

	for (auto slot(0u); slot < slots_.size(); slot++)
	{
		eject_cartridge(slot);
	}
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

void multipak_cartridge::process_horizontal_sync()
{
	vcc::core::utils::section_locker lock(mutex_);

	for(const auto& cartridge_slot : slots_)
	{
		cartridge_slot.process_horizontal_sync();
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
		// Should this OR all the values together?
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

void multipak_cartridge::status(char* text_buffer, size_t buffer_size)
{
	vcc::core::utils::section_locker lock(mutex_);

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

	// Menu items for loaded carts. Each slot is allocated 50.
	if (menu_item_id >= 50 && menu_item_id <= 100)
	{
		slots_[0].menu_item_clicked(menu_item_id - 50);
	}

	if (menu_item_id > 100 && menu_item_id <= 150)
	{
		slots_[1].menu_item_clicked(menu_item_id - 100);
	}

	if (menu_item_id > 150 && menu_item_id <= 200)
	{
		slots_[2].menu_item_clicked(menu_item_id - 150);
	}

	if (menu_item_id > 200 && menu_item_id <= 250)
	{
		slots_[3].menu_item_clicked(menu_item_id - 200);
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

	return slots_[slot].description();
}

bool multipak_cartridge::empty(slot_id_type slot) const
{
	vcc::core::utils::section_locker lock(mutex_);

	return slots_[slot].empty();
}

void multipak_cartridge::eject_cartridge(slot_id_type slot)
{
	vcc::core::utils::section_locker lock(mutex_);
	slots_[slot].stop();
	slots_[slot] = {};
	SendMessage(gVccWnd,WM_COMMAND,(WPARAM) ID_FILE_RESET,(LPARAM) 0);
}

multipak_cartridge::mount_status_type multipak_cartridge::mount_cartridge(
	slot_id_type slot, const path_type& filename)
{
	cpak_callbacks callbacks {
		gHostCallbacks->assert_interrupt_,
		gSlotCallbacks[slot].set_cartridge_line,
		gHostCallbacks->write_memory_byte_,
		gHostCallbacks->read_memory_byte_,
		gSlotCallbacks[slot].append_menu_item
	};
	DLOG_C("%3d %p %p %p %p %p\n",slot,callbacks);

	auto* multipakHost = this;

	// callback_context is a host-owned opaque per-cartridge state. Passed to cartridges
	// at initialization and returned on every callback so the host can route and manage a
	// specific cartridge instance. MPI currently uses it to know what slot a cart is in.
	auto ctx = std::make_unique<multipak_cartridge_context>(
		slot,
		*context_,
		*multipakHost);

	auto loadedCartridge = vcc::core::load_cartridge(
		filename,
		std::move(ctx),
		multipakHost,
		context_->configuration_path(),
		gVccWnd,
		callbacks);

	if (loadedCartridge.load_result != mount_status_type::success)
	{
		// Tell user why load failed
		auto error_string(vcc::core::cartridge_load_error_string(loadedCartridge.load_result));
		error_string += "\n\n";
		error_string += filename;
		MessageBox(GetForegroundWindow(), error_string.c_str(), "Load Error", MB_OK | MB_ICONERROR);
		return loadedCartridge.load_result;
	}

	// Should we call eject(slot) here?

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

// The following has no effect until VCC is reset
void multipak_cartridge::switch_to_slot(slot_id_type slot)
{
	switch_slot_ = slot;
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
			break;
		}

	case MID_FINISH:
		build_menu();
		break;

	default:
		{
			// Add 50 times the slot number to control id's
			if (item.menu_id >= MID_CONTROL) {
				item.menu_id += (slot + 1) * 50;
			}
			vcc::core::utils::section_locker lock(mutex_);
			slots_[slot].append_menu_item(item);
			break;
		}
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


void multipak_cartridge::assert_cartridge_line(slot_id_type slot, bool line_state)
{
	vcc::core::utils::section_locker lock(mutex_);

	slots_[slot].line_state(line_state);
	if (selected_scs_slot() == slot)
	{
		context_->assert_cartridge_line(slots_[slot].line_state());
	}
}
