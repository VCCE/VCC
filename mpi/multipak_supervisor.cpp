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

#include "multipak_supervisor.h"
#include "mpi.h"
#include "resource.h"
#include <vcc/util/coreutil.h>
#include <vcc/util/textutil.h>
#include <vcc/util/filesystem.h>
#include <vcc/util/logger.h>
#include <vcc/bus/cartridge_menu.h>
#include <vcc/bus/cartridge_menuitem.h>
#include <vcc/bus/cartridge_messages.h>

// SlotId is an unsigned int 0-4 used to indicate to a cartridge which slot
// it is in.  SlotId 0 is the boot slot, SlotId's 1-4 are multipak slots
// mpi_slot indexes used elsewhere in this source differ, they represent only
// multipak mpi_slots and are numbered 0-3,  (SlotId = mpi_slot+1)

multipak_supervisor::multipak_supervisor(
	multipak_configuration& configuration)
	:
	configuration_(configuration) {}

void multipak_supervisor::start()
{
	// Mount mpi slots per settings mpi_slit is 0-3, Slot is 1-4
	// Slot 0 is the the boot slot (where MPI cart is)
	for (int mpi_slot = 0; mpi_slot < 4; mpi_slot++)
	{
		const auto path(VCC::Util::find_pak_module_path(
					configuration_.slot_cartridge_path(mpi_slot)));
		if (!path.empty())
		{
			DLOG_C("\nmultipak_supervisor.start slot:%d %s\n",mpi_slot+1,path.c_str()); 
			CartLoadRequest slotData{};
			strcpy_s(slotData.pluginPath, slotData.size, path.c_str());
			SendLoadSlot(gVccWnd, mpi_slot+1, slotData);
		}
	}
	switch_slot_ = configuration_.selected_slot();
	SendActiveSlot(gVccWnd, switch_slot_);
	SendMessage(gVccWnd,WM_VCC_UPD_MENU,(WPARAM) 0,(LPARAM) 0);
}

void multipak_supervisor::stop()
{
	DLOG_C("multipak_supervisor stop\n");
	gConfigurationDialog.close();
}

void multipak_supervisor::reset()
{
	DLOG_C("multipak_supervisor reset\n");

	VCC::Util::section_locker lock(mutex_);

	unsigned char mpi_slot = switch_slot_ & 3;
	switch_slot_ = mpi_slot;

	// Tell pakinterface what the active slot is
	SendActiveSlot(gVccWnd, switch_slot_); //0-3
}

void multipak_supervisor::menu_item_clicked(unsigned char menu_item_id)
{
	DLOG_C("multipak_supervisor menu_item_clicked %d\n", menu_item_id); 

	if (menu_item_id == 19)	//MPI Config
	{
		gConfigurationDialog.open();
	}
}

// Return MPI menu
bool multipak_supervisor::get_menu_item(menu_item_entry* item, size_t index)
{
	using VCC::Bus::gDllCartMenu;
	if (!item) return false;
	if (index == 0) {
		gDllCartMenu.clear();
		gDllCartMenu.add("", 0, MIT_Seperator);
		gDllCartMenu.add("MPI Config", ControlId(19), MIT_StandAlone);
	}
	return gDllCartMenu.copy_item( *item, index);
}

// Send load cartridge request to pakinterface
multipak_supervisor::mount_status_type multipak_supervisor::mount_cartridge(
	slot_id_type mpi_slot, const path_type& filename)
{
	CartLoadRequest slotData{};
	strcpy_s(slotData.pluginPath, slotData.size, filename.c_str());
	SendLoadSlot(gVccWnd, mpi_slot+1, slotData); // Slot is 1-4

	// Dynamic menu update
	SendMessage(gVccWnd,WM_VCC_UPD_MENU,(WPARAM) 0,(LPARAM) 0);
	
	return mount_status_type::success;
}

void multipak_supervisor::switch_to_slot(slot_id_type mpi_slot)
{
	DLOG_C("multipak_supervisor set selected switch slot (0-3) %d\n", mpi_slot); 
	switch_slot_ = mpi_slot;
}

multipak_supervisor::slot_id_type multipak_supervisor::selected_switch_slot() const
{
	DLOG_C("multipak_supervisor get selected switch slot %d\n", switch_slot_); 
	return switch_slot_;
}

