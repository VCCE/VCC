//#define USE_LOGGING
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
#include <vcc/util/winapi.h>
#include <vcc/util/filesystem.h>
#include <vcc/util/logger.h>
#include <vcc/bus/cartridge_menu.h>
#include <vcc/bus/cartridge_menuitem.h>
namespace
{

// SlotId is an unsigned int 0-4 used to indicate to a cartridge which slot
// it is in.  SlotId 0 is the boot slot, SlotId's 1-4 are multipak slots
// mpi_slot indexes used elsewhere in this source differ, they represent only
// multipak mpi_slots and are numbered 0-3,  (SlotId = mpi_slot+1)

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
	return ::VCC::Util::load_string(gModuleInstance, IDS_MODULE_NAME);
}

multipak_cartridge::catalog_id_type multipak_cartridge::catalog_id() const
{
	return ::VCC::Util::load_string(gModuleInstance, IDS_CATNUMBER);
}

multipak_cartridge::description_type multipak_cartridge:: description() const
{
	return ::VCC::Util::load_string(gModuleInstance, IDS_CATNUMBER);
}


void multipak_cartridge::start()
{
	// Get the startup slot and set Chip select and SCS slots from ini file
	switch_slot_ = configuration_.selected_slot();
	cached_cts_slot_ = switch_slot_;
	cached_scs_slot_ = switch_slot_;

	// Mount them
	for (auto mpi_slot(0u); mpi_slot < slots_.size(); mpi_slot++)
	{
		const auto path(VCC::Util::find_pak_module_path(
					configuration_.slot_cartridge_path(mpi_slot)));
		if (!path.empty())
		{
			if (mount_cartridge(mpi_slot, path) != VCC::Core::cartridge_loader_status::success) {
				DLOG_C("Clearing configured slot path %d\n",mpi_slot);
				configuration_.slot_cartridge_path(mpi_slot,"");
			}
		}
	}

	// Build the dynamic menu  **OLD**
	build_menu();
}


void multipak_cartridge::stop()
{
	gConfigurationDialog.close();

	for (auto mpi_slot(0u); mpi_slot < slots_.size(); mpi_slot++)
	{
		eject_cartridge(mpi_slot);
	}
}

void multipak_cartridge::reset()
{
	VCC::Util::section_locker lock(mutex_);

	unsigned char mpi_slot = switch_slot_ & 3;
	switch_slot_ = cached_cts_slot_ = cached_scs_slot_ = mpi_slot;
	slot_register_ = 0b11001100 | mpi_slot | (mpi_slot << 4);

	for (const auto& cartridge_slot : slots_)
	{
		cartridge_slot.reset();
	}

	context_->assert_cartridge_line(slots_[cached_scs_slot_].line_state());
}

void multipak_cartridge::process_horizontal_sync()
{
	VCC::Util::section_locker lock(mutex_);

	for(const auto& cartridge_slot : slots_)
	{
		cartridge_slot.process_horizontal_sync();
	}
}

void multipak_cartridge::write_port(unsigned char port_id, unsigned char value)
{
	VCC::Util::section_locker lock(mutex_);

	// slot_select_port_id is 0x7f
	if (port_id == slot_select_port_id)
	{
		// Bits 1-0 SCS slot
		// Bits 5-4 CTS slot
		cached_scs_slot_ = value & 3;
		cached_cts_slot_ = (value >> 4) & 3;
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
	VCC::Util::section_locker lock(mutex_);

	// slot_select_port_id will ALLWAYS be 0x7f chet
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
	VCC::Util::section_locker lock(mutex_);
	return slots_[cached_cts_slot_].read_memory_byte(memory_address);
}

void multipak_cartridge::status(char* text_buffer, size_t buffer_size)
{
	VCC::Util::section_locker lock(mutex_);

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
	VCC::Util::section_locker lock(mutex_);

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

	// Each slot is allocated 50 menu items and items were
	// biased by SlotId * 50 so MPI knows which slot the item is for
	if (menu_item_id < 50) return;  // Nothing more for SlotId 0

	VCC::Util::section_locker lock(mutex_);

	if (menu_item_id < 100) {
		slots_[0].menu_item_clicked(menu_item_id - 50);
		return;
	}

	if (menu_item_id < 150) {
		slots_[1].menu_item_clicked(menu_item_id - 100);
		return;
	}

	if (menu_item_id < 200) {
		slots_[2].menu_item_clicked(menu_item_id - 150);
		return;
	}

	if (menu_item_id < 250) {
		slots_[3].menu_item_clicked(menu_item_id - 200);
		return;
	}
}

// returns menu item from DLL item list
bool multipak_cartridge::get_menu_item(menu_item_entry* item, size_t index)
{
	using VCC::Bus::gDllCartMenu;

	if (!item) return false;

	// index 0 is special, it indicates DLL should refresh it's menus
	if (index == 0) {
		// Rebuild MPI menu
		gDllCartMenu.clear();
		gDllCartMenu.add("", 0, MIT_Seperator);
		gDllCartMenu.add("MPI Config", ControlId(19), MIT_StandAlone);
		// Append child menus
		for (int SlotId = 4; SlotId > 0; SlotId--) {
			menu_item_entry pakitm;
			for (int ndx = 0; ndx < MAX_MENU_ITEMS; ndx++) {
				if (slots_[SlotId-1].get_menu_item(&pakitm,ndx)) {
					// bias control_ids per slot
					if (pakitm.menu_id >= MID_CONTROL)
						pakitm.menu_id += (SlotId * 50);
					gDllCartMenu.add(pakitm.name,pakitm.menu_id,pakitm.type);
				} else {
					break;
				}
			}
		}
		if (gDllCartMenu.size() == 0) return 0;
	}
	return gDllCartMenu.copy_item( *item, index);
}


multipak_cartridge::label_type multipak_cartridge::slot_label(slot_id_type mpi_slot) const
{
	VCC::Util::section_locker lock(mutex_);

	return slots_[mpi_slot].label();
}

multipak_cartridge::description_type multipak_cartridge::slot_description(slot_id_type mpi_slot) const
{
	VCC::Util::section_locker lock(mutex_);

	return slots_[mpi_slot].description();
}

bool multipak_cartridge::empty(slot_id_type mpi_slot) const
{
	VCC::Util::section_locker lock(mutex_);

	return slots_[mpi_slot].empty();
}

void multipak_cartridge::eject_cartridge(slot_id_type mpi_slot)
{

	VCC::Util::section_locker lock(mutex_);
	slots_[mpi_slot].stop();
	slots_[mpi_slot] = {};
	if (mpi_slot == cached_cts_slot_ || mpi_slot == switch_slot_)
		SendMessage(gVccWnd,WM_COMMAND,(WPARAM) ID_FILE_RESET,(LPARAM) 0);
}

// create cartridge object and load cart DLL
multipak_cartridge::mount_status_type multipak_cartridge::mount_cartridge(
	slot_id_type mpi_slot, const path_type& filename)
{
	// Capture pointer to multipak_cartridge
	static multipak_cartridge* self = nullptr;
	self = this;

	// Create thunks for slot sensitive callbacks
	static auto assert_cartridge_line_thunk =
		+[](slot_id_type SlotId,
		bool line_state)
	{
		self->assert_cartridge_line(SlotId, line_state);
	};

	static auto append_menu_item_thunk =
		+[](slot_id_type SlotId,
		const char* menu_name,
		int menu_id,
		MenuItemType menu_type)
	{
		menu_item_type item {menu_name, (unsigned int)menu_id, menu_type};
		self->append_menu_item(SlotId, item);
	};

	// Build callback table for carts loaded on MPI
	cpak_callbacks callbacks {
		gHostCallbacks->assert_interrupt_,
		assert_cartridge_line_thunk,
		gHostCallbacks->write_memory_byte_,
		gHostCallbacks->read_memory_byte_,
		append_menu_item_thunk
	};
	
	DLOG_C("%3d %p %p %p %p %p\n",mpi_slot,callbacks);

	std::size_t SlotId = mpi_slot + 1;

	// ctx is passed to the loader but not to cartridge DLL's
	auto* pakContainer = this;
	auto ctx = std::make_unique<multipak_cartridge_context>(
		mpi_slot,
		*context_,
		*pakContainer);  // Why does the loader need this?

	DLOG_C("load cart %d  %s\n",mpi_slot,filename);

	auto loadedCartridge = VCC::Core::load_cartridge(
		filename,
		std::move(ctx),
		SlotId,
		context_->configuration_path(),    // ini file name
		gVccWnd,
		callbacks);

	if (loadedCartridge.load_result != mount_status_type::success) {
		// Tell user why load failed
		auto error_string(
			VCC::Core::cartridge_load_error_string(
				loadedCartridge.load_result)
			);
		error_string += "\n\n";
		error_string += filename;
		MessageBox(GetForegroundWindow(),
			error_string.c_str(),
			"Load Error",
			MB_OK | MB_ICONERROR);
		return loadedCartridge.load_result;
	}

	// Should we call eject(slot) here?

	VCC::Util::section_locker lock(mutex_);

	slots_[mpi_slot] = {
		filename,
		move(loadedCartridge.handle),
		move(loadedCartridge.cartridge)
	};

	slots_[mpi_slot].start();
	slots_[mpi_slot].reset();

	return loadedCartridge.load_result;
}

// The following has no effect until VCC is reset
void multipak_cartridge::switch_to_slot(slot_id_type mpi_slot)
{
	switch_slot_ = mpi_slot;
}

multipak_cartridge::slot_id_type multipak_cartridge::selected_switch_slot() const
{
	return switch_slot_;
}

multipak_cartridge::slot_id_type multipak_cartridge::selected_scs_slot() const
{
	return cached_scs_slot_;
}

// *OLD* Save cart Menu items into containers per slot  **OLD**
void multipak_cartridge::append_menu_item(slot_id_type SlotId, menu_item_type item)
{
	DLOG_C("menu_item %d %d \n",SlotId,item.menu_id);

	auto mpi_slot = SlotId - 1;

	switch (item.menu_id) {
	case MID_BEGIN:
		{
			VCC::Util::section_locker lock(mutex_);
			slots_[mpi_slot].reset_menu();
			break;
		}
	case MID_FINISH:
		build_menu();
		break;
	default:
		{
			// Add 50 times the slot_id to menu_ids so WinMain knows who is calling
			if (item.menu_id >= MID_CONTROL) {
				item.menu_id += SlotId * 50;
			}
			VCC::Util::section_locker lock(mutex_);
			slots_[mpi_slot].append_menu_item(item);
			break;
		}
	}
}


// *OLD* This gets called any time a cartridge menu is changed. It draws the entire menu.
void multipak_cartridge::build_menu()
{
	// do we really need this here? menu draw should be async
	VCC::Util::section_locker lock(mutex_);

	// Init the menu, establish MPI config control, build slot menus, then draw it.
	context_->add_menu_item("", MID_BEGIN, MIT_Head);
	context_->add_menu_item("", MID_ENTRY, MIT_Seperator);
	context_->add_menu_item("MPI Config", ControlId(19), MIT_StandAlone);
	for (int mpi_slot = 3; mpi_slot >= 0; mpi_slot--)
	{
		slots_[mpi_slot].enumerate_menu_items(*context_);
	}
	context_->add_menu_item("", MID_FINISH, MIT_Head);  // Finish draws the entire menu
}

void multipak_cartridge::assert_cartridge_line(slot_id_type SlotId, bool line_state)
{
	DLOG_C("cart line %d %d \n",SlotId,line_state);

	VCC::Util::section_locker lock(mutex_);
	auto mpi_slot = SlotId - 1;
	slots_[mpi_slot].line_state(line_state);
	if (selected_scs_slot() == mpi_slot) {
		context_->assert_cartridge_line(slots_[mpi_slot].line_state());
	}
}
