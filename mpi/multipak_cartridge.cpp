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
#include "resource.h"
#include <vcc/utils/winapi.h>
#include <vcc/utils/filesystem.h>

HINSTANCE gModuleInstance = nullptr;

namespace
{

	using size_type = std::size_t;
	using context_type = cartridge_capi_context;
	using mount_status_type = ::vcc::utils::cartridge_loader_status;
	using slot_id_type = ::std::size_t;
	using path_type = ::std::string;
	using label_type = ::std::string;
	using description_type = ::std::string;
	using menu_item_type = CartMenuItem;

	constexpr std::pair<size_t, size_t> disk_controller_io_port_range = { 0x40, 0x5f };

	const size_t slot_select_port_id = 0x7f;
	const size_t default_switch_slot_value = 0x03;
	const size_t default_slot_register_value = 0xff;

	vcc::utils::critical_section mutex_;
	const HINSTANCE& module_instance_(gModuleInstance);	//	DELETEME

	void* gHostKey = nullptr;
	std::string configuration_path_;
	const context_type* context_ = nullptr;
	std::array<cartridge_capi_context, 4> capi_contexts_;
	std::array<vcc::modules::mpi::cartridge_slot, 4> slots_;
	unsigned char slot_register_ = default_slot_register_value;
	slot_id_type switch_slot_ = default_switch_slot_value;
	slot_id_type cached_cts_slot_ = default_switch_slot_value;
	slot_id_type cached_scs_slot_ = default_switch_slot_value;
	configuration_dialog settings_dialog_;


	// Is a port a disk port?
	constexpr bool is_disk_controller_port(slot_id_type port)
	{
		return port >= disk_controller_io_port_range.first && port <= disk_controller_io_port_range.second;
	}

	template<slot_id_type SlotIndex_>
	static void assert_cartridge_line_on_slot(void* host_context, bool line_state)
	{
		mpi_assert_cartridge_line(SlotIndex_, line_state);
	}

	template<slot_id_type SlotIndex_>
	static void append_menu_item_on_slot(void* host_context, const char* text, int id, MenuItemType type)
	{
		mpi_append_menu_item(SlotIndex_, { text, static_cast<unsigned int>(id), type });
	}

}

BOOL WINAPI DllMain(HINSTANCE module_instance, DWORD reason, [[maybe_unused]] LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		gModuleInstance = module_instance;
	}

	return TRUE;
}

extern "C"
{

	__declspec(dllexport) const char* PakGetName()
	{
		static const auto name(::vcc::utils::load_string(gModuleInstance, IDS_MODULE_NAME));
		return name.c_str();
	}

	__declspec(dllexport) const char* PakGetCatalogId()
	{
		static const auto catalog_id(::vcc::utils::load_string(gModuleInstance, IDS_CATNUMBER));
		return catalog_id.c_str();
	}

	__declspec(dllexport) const char* PakGetDescription()
	{
		static const auto description(::vcc::utils::load_string(gModuleInstance, IDS_DESCRIPTION));
		return description.c_str();
	}


	__declspec(dllexport) void PakInitialize(
		void* const host_key,
		const char* const configuration_path,
		const cartridge_capi_context* const context)
	{
		gHostKey = host_key;
		configuration_path_ = configuration_path;
		context_ = context;

		capi_contexts_[0] =
		{
			context->assert_interrupt,
			assert_cartridge_line_on_slot<0>,
			context->write_memory_byte,
			context->read_memory_byte,
			append_menu_item_on_slot<0>
		};

		capi_contexts_[1] =
		{
			context->assert_interrupt,
			assert_cartridge_line_on_slot<1>,
			context->write_memory_byte,
			context->read_memory_byte,
			append_menu_item_on_slot<1>
		};

		capi_contexts_[2] =
		{
			context->assert_interrupt,
			assert_cartridge_line_on_slot<2>,
			context->write_memory_byte,
			context->read_memory_byte,
			append_menu_item_on_slot<2>
		};

		capi_contexts_[3] =
		{
			context->assert_interrupt,
			assert_cartridge_line_on_slot<3>,
			context->write_memory_byte,
			context->read_memory_byte,
			append_menu_item_on_slot<3>
		};

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
				mpi_mount_cartridge(slot, path);
			}
		}

		// Build the dynamic menu
		mpi_build_menu();
	}


	__declspec(dllexport) void PakTerminate()
	{
		settings_dialog_.close();

		for (auto slot(0u); slot < slots_.size(); slot++)
		{
			mpi_eject_cartridge(slot);
		}
	}

	__declspec(dllexport) void PakReset()
	{
		vcc::utils::section_locker lock(mutex_);

		cached_cts_slot_ = switch_slot_;
		cached_scs_slot_ = switch_slot_;
		for (const auto& cartridge_slot : slots_)
		{
			cartridge_slot.reset();
		}

		context_->assert_cartridge_line(gHostKey, slots_[cached_scs_slot_].line_state());
	}

	__declspec(dllexport) void PakProcessHorizontalSync()
	{
		vcc::utils::section_locker lock(mutex_);

		for (const auto& cartridge_slot : slots_)
		{
			cartridge_slot.process_horizontal_sync();
		}
	}

	__declspec(dllexport) void PakWritePort(unsigned char port_id, unsigned char value)
	{
		vcc::utils::section_locker lock(mutex_);

		if (port_id == slot_select_port_id)
		{
			cached_scs_slot_ = value & 0b00000011;			// SCS
			cached_cts_slot_ = (value >> 4) & 0b00000011;	// CTS
			slot_register_ = value;

			context_->assert_cartridge_line(gHostKey, slots_[cached_scs_slot_].line_state());

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

	__declspec(dllexport) unsigned char PakReadPort(unsigned char port_id)
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

	__declspec(dllexport) unsigned char PakReadMemoryByte(size_type memory_address)
	{
		vcc::utils::section_locker lock(mutex_);

		return slots_[cached_cts_slot_].read_memory_byte(memory_address);
	}

	__declspec(dllexport) void PakGetStatus(char* text_buffer, size_t buffer_size)
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

	__declspec(dllexport) unsigned short PakSampleAudio()
	{
		vcc::utils::section_locker lock(mutex_);

		unsigned short sample = 0;
		for (const auto& cartridge_slot : slots_)
		{
			sample += cartridge_slot.sample_audio();
		}

		return sample;
	}

	__declspec(dllexport) void PakMenuItemClicked(unsigned char menu_item_id)
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

}

label_type mpi_slot_label(slot_id_type slot)
{
	vcc::utils::section_locker lock(mutex_);

	return slots_[slot].label();
}

description_type mpi_slot_description(slot_id_type slot)
{
	vcc::utils::section_locker lock(mutex_);

	return slots_[slot].description();
}

bool mpi_empty(slot_id_type slot)
{
	vcc::utils::section_locker lock(mutex_);

	return slots_[slot].empty();
}

void mpi_eject_cartridge(slot_id_type slot)
{
	vcc::utils::section_locker lock(mutex_);

	slots_[slot].stop();

	slots_[slot] = {};
}

mount_status_type mpi_mount_cartridge(
	slot_id_type slot,
	const path_type& filename)
{
	auto loadedCartridge(vcc::utils::load_cartridge(
		filename,
		nullptr,
		capi_contexts_[slot],
		nullptr,
		configuration_path_));
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

void mpi_switch_to_slot(slot_id_type slot)
{
	vcc::utils::section_locker lock(mutex_);

	switch_slot_ = slot;
	cached_scs_slot_ = slot;
	cached_cts_slot_ = slot;

	context_->assert_cartridge_line(gHostKey, slots_[slot].line_state());
}

slot_id_type mpi_selected_switch_slot()
{
	return switch_slot_;
}

slot_id_type mpi_selected_scs_slot()
{
	return cached_scs_slot_;
}

// Save cart Menu items into containers per slot
void mpi_append_menu_item(slot_id_type slot, menu_item_type item)
{
	switch (item.menu_id) {
	case MID_BEGIN:
	{
		vcc::utils::section_locker lock(mutex_);

		slots_[slot].reset_menu();
	}
	break;

	case MID_FINISH:
		mpi_build_menu();
		break;

	default:
		// Add 20 times the slot number to control id's
		if (item.menu_id >= MID_CONTROL)
		{
			item.menu_id += (slot + 1) * 20;
		}
		{
			vcc::utils::section_locker lock(mutex_);

			slots_[slot].append_menu_item(item);
		}
		break;
	}
}

// This gets called any time a cartridge menu is changed. It draws the entire menu.
void mpi_build_menu()
{
	vcc::utils::section_locker lock(mutex_);

	// Init the menu, establish MPI config control, build slot menus, then draw it.
	context_->add_menu_item(gHostKey, "", MID_BEGIN, MIT_Head);
	context_->add_menu_item(gHostKey, "", MID_ENTRY, MIT_Seperator);
	context_->add_menu_item(gHostKey, "MPI Config", ControlId(19), MIT_StandAlone);
	for (int slot = 3; slot >= 0; slot--)
	{
		slots_[slot].enumerate_menu_items(gHostKey, *context_);
	}
	context_->add_menu_item(gHostKey, "", MID_FINISH, MIT_Head);  // Finish draws the entire menu
}


void mpi_assert_cartridge_line(slot_id_type slot, bool line_state)
{
	vcc::utils::section_locker lock(mutex_);

	slots_[slot].line_state(line_state);
	if (mpi_selected_scs_slot() == slot)
	{
		context_->assert_cartridge_line(gHostKey, slots_[slot].line_state());
	}
}
