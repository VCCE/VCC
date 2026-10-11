//define USE_LOGGING
//======================================================================
// This file is part of VCC (Virtual Color Computer).
// Vcc is Copyright 2015 by Joseph Forgione
//
// VCC (Virtual Color Computer) is free software, you can redistribute
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//======================================================================
//
// As of VCC version 2.1.10 the MPI cartridge no longer loads DLL and
// ROM files that represent cartridges. Instead it sends messages to
// pakinterface with cartridge update requests. An invidible proxy
// window is used to forward the messages so messaging is not affected
// by VCC full screen toggles.
//
// An array of 5 slots holds the cartridge objects used to keep track of
// loaded cartridges. Slot 0 is the COCO boot slot and slots 1-4 are
// mulipak slots.
//
//======================================================================

#include "defines.h"
#include "tcc1014mmu.h"
#include "tcc1014registers.h"
#include "pakinterface.h"
#include "config.h"
#include "Vcc.h"
#include "mc6821.h"
#include "resource.h"
#include "pakrouter.h"
#include <vcc/bus/null_cartridge.h>
#include <vcc/bus/cartridge_menu.h>
#include <vcc/bus/cartridge_messages.h>
#include <vcc/bus/dll_deleter.h>
#include <vcc/util/limits.h>
#include <vcc/util/logger.h>
#include <vcc/util/FileOps.h>
#include <vcc/util/DialogOps.h>
#include <fstream>
#include <Windows.h>
#include <commdlg.h>

using cartridge_loader_status = VCC::Core::cartridge_loader_status;
using cartridge_loader_result = VCC::Core::cartridge_loader_result;

// Storage for Pak ROMs
extern SystemState EmuState;

static VCC::Util::critical_section gPakMutex;
static char DllPath[MAX_PATH] = "";

void CartMenuCallBack(const char* name, int menu_id, MenuItemType type);

// Arrays for holding cartridge slots. Slot 0 is boot slot.
#define NumCartSlots 5
using plugin_ptr = cartridge_loader_result::cartridge_ptr_type;
static std::array<plugin_ptr, NumCartSlots> gCartSlots{
	std::make_unique<VCC::Core::null_cartridge>(),
	std::make_unique<VCC::Core::null_cartridge>(),
	std::make_unique<VCC::Core::null_cartridge>(),
	std::make_unique<VCC::Core::null_cartridge>(),
	std::make_unique<VCC::Core::null_cartridge>()
};

// Router object manages cartridge exports and callbacks
static VCC::Core::PakRouter gPakRouter(gCartSlots);

// A list of smart handles for unloading dll's
using plugin_handle = cartridge_loader_result::handle_type;
static std::array<plugin_handle, NumCartSlots> gCartDlls{};

void UnloadCartridge(int slot);
static cartridge_loader_status load_any_cartridge(int slot, const char *filename);

//--------------------------------------------------------
// Define cartridge callbacks for the router
//--------------------------------------------------------

static void write_memory_byte_impl(size_t slot, unsigned char val, unsigned short adr) {
	gPakRouter.cart_write_memory(slot, val, adr);
}

static unsigned char read_memory_byte_impl(size_t slot,unsigned short adr) {
    return gPakRouter.cart_read_memory(slot, adr);
}

static void assert_cart_line_impl(size_t slot,bool state) {
    gPakRouter.cart_assert_line(slot, state);
}

static void assert_interrupt_impl(size_t slot, Interrupt intr, InterruptSource src) {
    gPakRouter.cart_assert_interrupt(slot, intr, src);
}

struct cpak_callbacks slot_callbacks = {
	assert_interrupt_impl,
	assert_cart_line_impl,
	write_memory_byte_impl,
	read_memory_byte_impl
};

//--------------------------------------------------------
//	Define cartridge exports for pakrouter.
//--------------------------------------------------------

// Send hsync to all loaded carts
void PakTimer()
{
	VCC::Util::section_locker lock(gPakMutex);
	gPakRouter.horizontal_sync();
}

// Send reset to all loaded carts. This gets called when VCC is reset.
void ResetBus()
{
	DLOG_C("\npakinterface ResetBus()\n");
	VCC::Util::section_locker lock(gPakMutex);
	gPakRouter.reset();
}

// Gather status line for all loaded carts
void GetModuleStatus(SystemState *SMState)
{
	gPakRouter.plugin_status(SMState->StatusLine,sizeof(SMState->StatusLine));
}

unsigned char PakReadPort (unsigned char port)
{
	VCC::Util::section_locker lock(gPakMutex);
	return gPakRouter.read_port(port);
}

void PakWritePort(unsigned char Port,unsigned char Data)
{
	VCC::Util::section_locker lock(gPakMutex);
	gPakRouter.write_port(Port,Data);
}

unsigned char PackMem8Read (unsigned short Address)
{
	VCC::Util::section_locker lock(gPakMutex);
	return gPakRouter.read_memory_byte(Address&32767);
}

unsigned short PackAudioSample()
{
	VCC::Util::section_locker lock(gPakMutex);
	return gPakRouter.sample_audio();
}

//--------------------------------------------------------
// Build Plugin dynamic menus
//--------------------------------------------------------
void BuildCartMenu()
{
	//VCC::Util::section_locker lock(gPakMutex);
	using VCC::Bus::gVccCartMenu;
	gVccCartMenu.clear();

	// Item to remove cart in boot slot.
	if (!gCartSlots[0]->name().empty()) {
		std::string tmp = "&Eject " + gCartSlots[0]->name();
		gVccCartMenu.add(tmp, ControlId(2), MIT_StandAlone);
	}

	// Items to load the boot slot
	if (gPakRouter.mpi_not_active()) {
		gVccCartMenu.add("Load &MPI", ControlId(3), MIT_StandAlone);
	}
	gVccCartMenu.add("Load &DLL", ControlId(1), MIT_StandAlone);
	gVccCartMenu.add("Load &ROM", ControlId(4), MIT_StandAlone);

	// Items from loaded plugins
	for (int slot : {0, 4, 3, 2, 1}) {
		auto* cart = gCartSlots[slot].get();
		if (!cart) continue;
		for (int ndx = 0; ndx < MAX_MENU_ITEMS; ndx++) {
			menu_item_entry item;
			if (!cart->get_menu_item(&item, ndx)) break;
			// Bias menu id's for slot location
			if (item.menu_id >= MID_CONTROL)
				item.menu_id += (slot * 50);
			gVccCartMenu.add(item.name,item.menu_id,item.type);
		}
	}
}

//--------------------------------------------------------
// Dialog for loading boot slot plugin
//--------------------------------------------------------
void LoadCartridgeDialog(int type)
{
	char inifile[MAX_PATH];
	GetIniFilePath(inifile);

	static char cartDir[MAX_PATH] = "";
	FileDialog dlg;
	if (type == 0) {
		dlg.setTitle(TEXT("Load Program Pack"));
		dlg.setFilter("Hardware Packs\0*.dll\0"
				"All Supported Formats (*.dll;*.ccc;*.rom)\0*.dll;*.ccc;*.rom\0\0");
		Setting().read("DefaultPaths", "DLLPath", "", cartDir, MAX_PATH);
	} else {
		dlg.setTitle(TEXT("Load ROM"));
		dlg.setFilter("Rom Packs(*.ccc;*.rom)\0*.ccc;*.rom\0"
				"All Supported Formats (*.dll;*.ccc;*.rom)\0*.dll;*.ccc;*.rom\0\0");
		Setting().read("DefaultPaths", "RomPath", "", cartDir, MAX_PATH);
	}
	dlg.setInitialDir(cartDir);
	dlg.setFlags(OFN_FILEMUSTEXIST);
	if (dlg.show()) {
		auto status = PakLoadCartridge(dlg.path());
		if (status == cartridge_loader_status::success) {
			char filetype[4];
			dlg.getdir(cartDir);
			dlg.gettype(filetype);
			if ((strcmp(filetype,"dll") == 0) | (strcmp(filetype,"DLL") == 0 )) {  // DLL?
				Setting().write("DefaultPaths", "DLLPath", cartDir);
			} else {
				Setting().write("DefaultPaths", "RomPath", cartDir);
			}
		}
	}
}

//--------------------------------------------------------
// Load cartridge to boot slot
//--------------------------------------------------------

cartridge_loader_status PakLoadCartridge(const char* filename)
{
	static const std::map<cartridge_loader_status, UINT> string_id_map = {
		{ cartridge_loader_status::already_loaded, IDS_MODULE_ALREADY_LOADED},
		{ cartridge_loader_status::cannot_open, IDS_MODULE_CANNOT_OPEN},
		{ cartridge_loader_status::not_found, IDS_MODULE_NOT_FOUND },
		{ cartridge_loader_status::not_loaded, IDS_MODULE_NOT_LOADED },
		{ cartridge_loader_status::not_rom, IDS_MODULE_NOT_ROM },
		{ cartridge_loader_status::not_expansion, IDS_MODULE_NOT_EXPANSION }
	};

	const auto result(load_any_cartridge(0, filename));
	if (result == cartridge_loader_status::success)
	{
		Setting().write("Module", "OnBoot", filename);
		return result;
	}

	// Tell user why load failed
	auto error_string(VCC::Core::cartridge_load_error_string(result));
	error_string += "\n\n";
	error_string += filename;
	MessageBox(EmuState.WindowHandle, error_string.c_str(), "Load Error", MB_OK | MB_ICONERROR);

	return result;
}

//--------------------------------------------------------
// Unload cartridge in slot
//--------------------------------------------------------
void UnloadCartridge(int slot)
{
	DLOG_C("UnloadCartridge in slot %d\n",slot);

	// If slot 0 is unloaded then all other slots must be
	// unloaded first. MPI no longer does this when stopped.
	// Doing here gets that previous recursion in plain sight
	if (slot == 0) {
		UnloadCartridge(4);
		UnloadCartridge(3);
		UnloadCartridge(2);
		UnloadCartridge(1);
		gPakRouter.set_active_slot(0);
	}

	// Cartridge teardown must be done carefully to avoid VCC crashes.

	// Stop the cartridge
	gCartSlots[slot]->stop();
	
	// Replace with null cartridge (takes over CoCo CPU traffic)
	gCartSlots[slot] = std::make_unique<VCC::Core::null_cartridge>();
	gCartSlots[slot]->start();
	
	// The dll must not be unloaded while there is CPU
	// traffic running from it or VCC will crash. DLL's are
	// responsible for cleanig up threads and traffic when
	// asked to stop. A short pause here gives the exiting
	// DLL a moment to complete those tasks.
	Sleep(5);

	// Smart pointer reset() now unloads the dll
	gCartDlls[slot].reset();

	// update menus
	SendMessage(EmuState.WindowHandle,WM_VCC_UPD_MENU,(WPARAM) 0,(LPARAM) 0);
}

//--------------------------------------------------------
// Unload boot cartridge called by Vcc.cpp
//--------------------------------------------------------
void UnloadDll()
{
	UnloadCartridge(0);
	gPakRouter.set_active_slot(0);
}

//--------------------------------------------------------
// Load cartridge
//--------------------------------------------------------
static cartridge_loader_status load_any_cartridge(int slot, const char *filename)
{
	DLOG_C("\npakinterface.load_any_cartridge slot %d %s called\n", slot, filename);

	slot_id_type SlotId = slot;

	// DLL plugins need ini file path so they can manage settings
	char iniPath[MAX_PATH]="";
	GetIniFilePath(iniPath);

	// Load the cartridge
	auto loadedCartridge = VCC::Core::load_cartridge(
		filename,
		SlotId,
		iniPath,
		EmuState.hMsgProxy,
		slot_callbacks);

	if (loadedCartridge.load_result != cartridge_loader_status::success) {
		DLOG_C("pakinterface.load_any_cartridge slot %d %s failed\n", slot, filename);
		return loadedCartridge.load_result;
	}

	// Unload current cart in slot
	UnloadCartridge(slot);
	if (slot == 0) gPakRouter.set_active_slot(0);

	DLOG_C("pakinterface.load_any_cartridge move cart object to slot %d %s\n",slot,filename);
	gCartSlots[slot] = std::move(loadedCartridge.cartridge);
	gCartDlls[slot]  = std::move(loadedCartridge.handle);

	DLOG_C("pakinterface.load_any_cartridge start and initialize slot %d\n",slot);
	gCartSlots[slot]->start();
	gCartSlots[slot]->reset();

	if (slot == 0) {  // TODO: also check for active slot here?
		strcpy(DllPath, filename);
		EmuState.ResetPending = 2;
		SendMessage(EmuState.WindowHandle,WM_VCC_UPD_MENU,(WPARAM) 0,(LPARAM) 0);
	}

	return loadedCartridge.load_result;
}

void GetCurrentModule(char *DefaultModule)
{
	strcpy(DefaultModule,DllPath);
	return;
}

void UpdateBusPointer()
{
	// Do nothing for now. What the plan was for this is unknown.
}

//--------------------------------------------------------
// unload bootslot
//--------------------------------------------------------
void UnloadPack()
{
	DLOG_C("pakinterface.UnloadPack\n");
	UnloadCartridge(0);
	
	strcpy(DllPath,"");
	SetCart(0);

	gPakRouter.set_active_slot(0);
	EmuState.ResetPending=2;

	char inifile[MAX_PATH];
	GetIniFilePath(inifile);
	Setting().delete_key("Module", "OnBoot");
}

//--------------------------------------------------------
// load bootslot.  type: 0 program cartridge 1 rom cartridge
//--------------------------------------------------------
void LoadPack(int type) {
	LoadCartridgeDialog(type);
	gPakRouter.set_active_slot(0);
	EmuState.ResetPending=2;
	// Assert cart line for ROMs so they can auto run
	if (type) SetCart(1);
}

//--------------------------------------------------------
// CartMenuActivated is called from VCC WndPrc when a
// cartridge menu item is clicked. MenuID is less than 250
//--------------------------------------------------------
void CartMenuActivated(unsigned int MenuID)
{
	if (MenuID >= 250) return;

	switch (MenuID)
	{
	case 1:
		LoadPack(0);  // Load a CPAK
		break;

	case 2:
		UnloadPack();
		break;

	case 3:
	{
		char path[MAX_PATH];
		GetModuleFileName(nullptr,path,MAX_PATH);
		PathRemoveFileSpec(path);
		strncat(path,"mpi.dll",MAX_PATH);
		PakLoadCartridge(path);
		break;
	}
	case 4:
		LoadPack(1);	// Load a ROM
		break;

	default:
		// Let pakrouter handle clicks for multipak slots
		gPakRouter.menu_item_clicked(MenuID);
	}
}

//--------------------------------------------------------------
// Messages from MPI
// -------------------------------------------------------------

// Set startup slot currenly comes from a radio button in MPI config.
bool SetActiveSlot(unsigned int startup_slot)
{
	DLOG_C("Pakinterface SetActiveSlot %d\n",startup_slot);
	// Startup slot is 0-3 One is added to set the active slot.
	gPakRouter.set_active_slot(startup_slot+1);
	return true;
}

// Unload slot contents
bool UnloadSlot(unsigned int slot)
{
	DLOG_C("Pakinterface UnloadSlot: %d\n",slot);
	UnloadCartridge(slot);
	return true;
}

// Load multi slot (1-4)
bool LoadSlot(unsigned int slot, const CartLoadRequest * data)
{
	DLOG_C("pakinterface LoadSlot %d %s\n",slot,data->pluginPath);
	if (slot < 1 || slot > 4) {
		DLOG_C("Pakinterface LoadSlot bad slot num\n");
		return false;
	}

	if (data == nullptr) {
		DLOG_C("Pakinterface LoadSlot null data pointer\n");
		return false;
	}

	load_any_cartridge(slot, data->pluginPath);
	return true;
}

// Gets the name of cart in a slot.
bool GetSlotCartName(unsigned int slot, CartNameReply* rpy)
{
	auto* cart = gCartSlots[slot].get();
    if (cart) {
    	std::string s = cart->name();
    	std::strncpy(rpy->name, s.c_str(), rpy->size);
    	rpy->name[rpy->size-1] = '\0';
        return true;
    }
    rpy->name[0] = '\0';
    return false;
}

// Get description of cart a in slot
bool GetSlotCartDescript(unsigned int slot, CartDescReply* rpy)
{
	auto* cart = gCartSlots[slot].get();
    if (cart) {
    	std::string s = cart->description();
    	std::strncpy(rpy->description, s.c_str(), rpy->size);
    	rpy->description[rpy->size-1] = '\0';
        return true;
    }
    rpy->description[0] = '\0';
    return false;
}

