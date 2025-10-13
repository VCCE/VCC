////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	This is an expansion module for the Vcc Emulator. It simulated the functions
//	of the TRS-80 Multi-Pak Interface.
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
#include "cartridge_slot.h"
#include "resource.h"
#include <vcc/common/FileOps.h>
#include <vcc/common/DialogOps.h>
#include "../CartridgeMenu.h"
#include <vcc/common/logger.h>
#include <vcc/core/interrupts.h>
#include <vcc/core/cartridge.h>
#include <vcc/core/legacy_cartridge_definitions.h>
#include <vcc/core/cartridge_loader.h>
#include <vcc/core/cartridges/null_cartridge.h>
#include <vcc/core/utils/winapi.h>
#include <vcc/core/utils/dll_deleter.h>
#include <vcc/core/utils/filesystem.h>
#include <vcc/core/limits.h>
#include <array>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <Windows.h>
#include <CommCtrl.h>
#include <vcc/core/utils/configuration_serializer.h>
#include <vcc/core/utils/critical_section.h>


using cartridge_loader_status = vcc::core::cartridge_loader_status;

// Number of slots supported. Changing this might require code modification
constexpr size_t NUMSLOTS = 4u;
constexpr size_t multipak_memory_mapped_control_port_id = 0x7f;
constexpr std::pair<size_t, size_t> disk_controller_io_port_range = { 0x40, 0x5f };

struct cartridge_slot_details
{
	UINT edit_box_id;
	UINT radio_button_id;
	UINT insert_button_id;
	AssertCartridgeLineModuleCallback set_cartridge_line;
	AppendCartridgeMenuModuleCallback append_menu_item;
};

// Is a port a disk port?
constexpr bool is_disk_controller_port(size_t port)
{
	return port >= disk_controller_io_port_range.first && port <= disk_controller_io_port_range.second;
}

template<size_t SlotIndex_>
void assert_cartridge_line_on_slot(bool line_state)
{
	vcc::core::utils::section_locker lock(gPakMutex);

	gCartridgeSlots[SlotIndex_].line_state(line_state);
	if (SpareSelectSlot == SlotIndex_)
	{
		PakSetCart(gCartridgeSlots[SlotIndex_].line_state());
	}
}

template<size_t SlotIndex_>
void append_menu_item_on_slot(const char* text, int id, MenuItemType type)
{
	append_menu_item_on_slot(SlotIndex_, { text, static_cast<unsigned int>(id), type });
}


void eject_or_select_new_cartridge(size_t slot);
void menu_item_clicked_for_slot(size_t slot);
void set_selected_slot(size_t slot);
void CenterDialog(HWND);
void build_cartridge_menu();
void append_menu_item_on_slot(size_t slot, CartMenuItem item);
void update_slot_ui_elements(size_t slot);

//Function Prototypes for this module
LRESULT CALLBACK MpiConfigDlg(HWND, UINT, WPARAM, LPARAM);

void select_new_cartridge(size_t slot);
cartridge_loader_status mount_cartridge(size_t slot, const std::string& filename);
void eject_cartridge(size_t slot);
void load_configuration();
void save_configuration();
std::string get_cartridge_description(size_t slot);

static vcc::core::utils::critical_section gPakMutex;

static AssertInteruptModuleCallback AssertInt = [](Interrupt, InterruptSource) {};
static ReadMemoryByteModuleCallback MemRead8 = [](unsigned short) -> unsigned char { return 0; };
static WriteMemoryByteModuleCallback MemWrite8 = [](unsigned char, unsigned short) {};
static AssertCartridgeLineModuleCallback PakSetCart = [](bool) {};
static AppendCartridgeMenuModuleCallback CartMenuCallback = nullptr;// FIXME: This is only null to account for existing error checking

static HINSTANCE gModuleInstance = nullptr;
static std::string gConfigurationFilename;
static std::string gLastAccessedPath;


//**************************************************************
static std::array<vcc::modules::mpi::cartridge_slot, NUMSLOTS> gCartridgeSlots;
static const std::array<cartridge_slot_details, NUMSLOTS> gAvailableSlotsDetails = { {
	{ IDC_EDIT1, IDC_SELECT1, IDC_INSERT1, assert_cartridge_line_on_slot<0>, append_menu_item_on_slot<0> },
	{ IDC_EDIT2, IDC_SELECT2, IDC_INSERT2, assert_cartridge_line_on_slot<1>, append_menu_item_on_slot<1> },
	{ IDC_EDIT3, IDC_SELECT3, IDC_INSERT3, assert_cartridge_line_on_slot<2>, append_menu_item_on_slot<2> },
	{ IDC_EDIT4, IDC_SELECT4, IDC_INSERT4, assert_cartridge_line_on_slot<3>, append_menu_item_on_slot<3> }
} };

//***************************************************************
static size_t ChipSelectSlot = 3;
static size_t SpareSelectSlot = 3;
static size_t SwitchSlot = 3;
static size_t SlotRegister = 255;
static HWND gConfigurationDialogHandle = nullptr;
static HWND gConfigurationParentHandle = nullptr;


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD  Reason, LPVOID Reserved)
{
	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		gModuleInstance = hinstDLL;
		break;

	case DLL_PROCESS_DETACH:
		// Close dialog before unloading modules so config is saved
		CloseCartDialog(gConfigurationDialogHandle);  // defined in DialogOps
		for (auto slot(0u); slot < gCartridgeSlots.size(); slot++)
		{
			eject_cartridge(slot);
		}
		break;
	}

	return TRUE;
}


extern "C"
{
	__declspec(dllexport) void ModuleName(
		char* module_name_buffer,
		char* catalog_id_buffer,
		AppendCartridgeMenuModuleCallback append_menu_item_callback)
	{
		LoadString(gModuleInstance, IDS_MODULE_NAME, module_name_buffer, MAX_LOADSTRING);
		LoadString(gModuleInstance, IDS_CATNUMBER, catalog_id_buffer, MAX_LOADSTRING);
		CartMenuCallback = append_menu_item_callback;
	}
}

extern "C"
{
	__declspec(dllexport) void ModuleConfig(unsigned char menu_item_id)
	{
		if (menu_item_id == 19)	//MPI Config
		{
			if (!gConfigurationDialogHandle)
			{
				gConfigurationDialogHandle = CreateDialog(gModuleInstance, (LPCTSTR)IDD_DIALOG1, GetActiveWindow(), (DLGPROC)MpiConfigDlg);
			}

			ShowWindow(gConfigurationDialogHandle, 1);
		}

		vcc::core::utils::section_locker lock(gPakMutex);

		//Configs for loaded carts
		if (menu_item_id >= 20 && menu_item_id <= 40)
		{
			gCartridgeSlots[0].menu_item_clicked(menu_item_id - 20);
		}

		if (menu_item_id > 40 && menu_item_id <= 60)
		{
			gCartridgeSlots[1].menu_item_clicked(menu_item_id - 40);
		}

		if (menu_item_id > 60 && menu_item_id <= 80)
		{
			gCartridgeSlots[2].menu_item_clicked(menu_item_id - 60);
		}

		if (menu_item_id > 80 && menu_item_id <= 100)
		{
			gCartridgeSlots[3].menu_item_clicked(menu_item_id - 80);
		}
	}
}

// This captures the Function transfer point for the CPU assert interupt
extern "C"
{
	__declspec(dllexport) void AssertInterupt(AssertInteruptModuleCallback assert_interupt_callback)
	{
		AssertInt = assert_interupt_callback;
	}
}

extern "C"
{
	__declspec(dllexport) void PackPortWrite(unsigned char port_id, unsigned char value)
	{
		vcc::core::utils::section_locker lock(gPakMutex);

		if (port_id == multipak_memory_mapped_control_port_id) //Multi-Pak selects
		{
			SpareSelectSlot = value & 3;				// SCS
			ChipSelectSlot = (value & 0x30) >> 4;	// CTS
			SlotRegister = value;

			PakSetCart(gCartridgeSlots[SpareSelectSlot].line_state());

			return;
		}

		// Only write disk ports (0x40-0x5F) if SCS is set
		if (is_disk_controller_port(port_id))
		{
			gCartridgeSlots[SpareSelectSlot].write_port(port_id, value);
			return;
		}

		for (const auto& cartridge_slot : gCartridgeSlots)
		{
			cartridge_slot.write_port(port_id, value);
		}
	}
}

extern "C"
{
	__declspec(dllexport) unsigned char PackPortRead(unsigned char port_id)
	{
		vcc::core::utils::section_locker lock(gPakMutex);

		if (port_id == multipak_memory_mapped_control_port_id)	// Self
		{
			SlotRegister &= 0xCC;
			SlotRegister |= (SpareSelectSlot | (ChipSelectSlot << 4));

			return SlotRegister & 0xff;
		}

		// Only read disk ports (0x40-0x5F) if SCS is set
		if (is_disk_controller_port(port_id))
		{
			return gCartridgeSlots[SpareSelectSlot].read_port(port_id);
		}

		for (const auto& cartridge_slot : gCartridgeSlots)
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
}

extern "C"
{
	__declspec(dllexport) void HeartBeat()
	{
		for(const auto& cartridge_slot : gCartridgeSlots)
		{
			vcc::core::utils::section_locker lock(gPakMutex);

			cartridge_slot.heartbeat();
		}
	}
}

//This captures the pointers to the MemRead8 and MemWrite8 functions. This allows the DLL to do DMA xfers with CPU ram.
extern "C"
{
	__declspec(dllexport) void MemPointers(
		ReadMemoryByteModuleCallback read_memory_byte_callback,
		WriteMemoryByteModuleCallback write_memory_byte_callback)
	{
		MemRead8 = read_memory_byte_callback;
		MemWrite8 = write_memory_byte_callback;
	}
}

extern "C"
{
	__declspec(dllexport) unsigned char PakMemRead8(unsigned short memory_address)
	{
		vcc::core::utils::section_locker lock(gPakMutex);

		return gCartridgeSlots[ChipSelectSlot].read_memory_byte(memory_address);
	}
}

extern "C"
{
	__declspec(dllexport) void ModuleStatus(char* status_buffer)
	{
		char TempStatus[64] = "";

		sprintf(status_buffer, "MPI:%d,%d", ChipSelectSlot + 1, SpareSelectSlot + 1);
		for (const auto& cartridge_slot : gCartridgeSlots)
		{
			strcpy(TempStatus, "");
			vcc::core::utils::section_locker lock(gPakMutex);
			cartridge_slot.status(TempStatus);
			if (TempStatus[0])
			{
				strcat(status_buffer, " | ");
				strcat(status_buffer, TempStatus);
			}
		}
	}
}

// This gets called at the end of every scan line 262 Lines * 60 Frames = 15780 Hz 15720
extern "C"
{
	__declspec(dllexport) unsigned short ModuleAudioSample()
	{
		unsigned short sample = 0;
		for (const auto& cartridge_slot : gCartridgeSlots)
		{
			vcc::core::utils::section_locker lock(gPakMutex);

			sample += cartridge_slot.sample_audio();
		}

		return sample;
	}
}

extern "C"
{
	__declspec(dllexport) unsigned char ModuleReset()
	{
		vcc::core::utils::section_locker lock(gPakMutex);

		ChipSelectSlot = SwitchSlot;
		SpareSelectSlot = SwitchSlot;
		for (const auto& cartridge_slot : gCartridgeSlots)
		{
			cartridge_slot.reset();
		}

		PakSetCart(gCartridgeSlots[SpareSelectSlot].line_state());

		return 0;
	}
}

extern "C"
{
	__declspec(dllexport) void SetIniPath(const char* configuration_path)
	{
		gConfigurationFilename = configuration_path;
		load_configuration();
	}
}

extern "C"
{
	__declspec(dllexport) void SetCart(AssertCartridgeLineModuleCallback callback)
	{
		PakSetCart = callback;
	}
}

void CenterDialog(HWND hDlg)
{
	RECT rPar;
	GetWindowRect(GetParent(hDlg), &rPar);

	RECT rDlg;
	GetWindowRect(hDlg, &rDlg);

	const auto x = rPar.left + (rPar.right - rPar.left - (rDlg.right - rDlg.left)) / 2;
	const auto y = rPar.top + (rPar.bottom - rPar.top - (rDlg.bottom - rDlg.top)) / 2;
	SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

LRESULT CALLBACK MpiConfigDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (message)
	{
	case WM_CLOSE:
		save_configuration();
		DestroyWindow(hDlg);
		gConfigurationDialogHandle = nullptr;
		return TRUE;

	case WM_INITDIALOG:
		gConfigurationParentHandle = GetParent(hDlg);
		CenterDialog(hDlg);
		gConfigurationDialogHandle = hDlg;
		for (int slot = 0; slot < NUMSLOTS; slot++)
		{
			update_slot_ui_elements(slot);
		}

		set_selected_slot(SwitchSlot);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_SELECT1:
			set_selected_slot(0);
			return TRUE;
		case IDC_SELECT2:
			set_selected_slot(1);
			return TRUE;
		case IDC_SELECT3:
			set_selected_slot(2);
			return TRUE;
		case IDC_SELECT4:
			set_selected_slot(3);
			return TRUE;
		case IDC_INSERT1:
			eject_or_select_new_cartridge(0);
			return TRUE;
		case IDC_INSERT2:
			eject_or_select_new_cartridge(1);
			return TRUE;
		case IDC_INSERT3:
			eject_or_select_new_cartridge(2);
			return TRUE;
		case IDC_INSERT4:
			eject_or_select_new_cartridge(3);
			return TRUE;
		case ID_CONFIG1:
			menu_item_clicked_for_slot(0);
			return TRUE;
		case ID_CONFIG2:
			menu_item_clicked_for_slot(1);
			return TRUE;
		case ID_CONFIG3:
			menu_item_clicked_for_slot(2);
			return TRUE;
		case ID_CONFIG4:
			menu_item_clicked_for_slot(3);
			return TRUE;
		} // End switch LOWORD
		break;
	} // End switch message

	return FALSE;
}

void set_selected_slot(size_t slot)
{
	SendDlgItemMessage(
		gConfigurationDialogHandle,
		IDC_MODINFO,
		WM_SETTEXT,
		0,
		(LPARAM)get_cartridge_description(slot).c_str());

	for (auto ndx(0u); ndx < gAvailableSlotsDetails.size(); ndx++)
	{
		SendDlgItemMessage(
			gConfigurationDialogHandle,
			gAvailableSlotsDetails[ndx].radio_button_id,
			BM_SETCHECK,
			ndx == slot,
			0);
	}

	vcc::core::utils::section_locker lock(gPakMutex);

	SwitchSlot = slot;
	SpareSelectSlot = slot;
	ChipSelectSlot = slot;

	PakSetCart(gCartridgeSlots[slot].line_state());
}

void eject_or_select_new_cartridge(size_t slot)
{
	// Disable Slot changes if parent is disabled.  This prevents user using the
	// config dialog to eject a cartridge while VCC main is using a modal dialog
	// Otherwise user can crash VCC by unloading a disk cart while inserting a disk
	if (!IsWindowEnabled(gConfigurationParentHandle))
	{
		MessageBox(gConfigurationDialogHandle, "Cannot change slot content with dialog open", "ERROR", MB_ICONERROR);
		return;
	}

	if (!gCartridgeSlots[slot].empty())
	{
		eject_cartridge(slot);
	}
	else
	{
		select_new_cartridge(slot);
	}

	update_slot_ui_elements(slot);
	build_cartridge_menu();
}

void menu_item_clicked_for_slot(size_t slot)
{
	vcc::core::utils::section_locker lock(gPakMutex);

	gCartridgeSlots[slot].menu_item_clicked(0);
}

cartridge_loader_status mount_cartridge(size_t slot, const std::string& filename)
{
	auto loadedCartridge(vcc::core::load_cartridge(
		{
			gAvailableSlotsDetails[slot].append_menu_item,
			MemRead8,
			MemWrite8,
			AssertInt,
			gAvailableSlotsDetails[slot].set_cartridge_line
		},
		filename,
		gConfigurationFilename));
	if (loadedCartridge.load_result != cartridge_loader_status::success)
	{
		return loadedCartridge.load_result;
	}

	vcc::core::utils::section_locker lock(gPakMutex);

	gCartridgeSlots[slot] = {
		filename,
		move(loadedCartridge.handle),
		move(loadedCartridge.cartridge)
	};

	gCartridgeSlots[slot].start();
	gCartridgeSlots[slot].reset();

	build_cartridge_menu();

	return loadedCartridge.load_result;
}

void eject_cartridge(size_t slot)
{
	vcc::core::utils::section_locker lock(gPakMutex);

	gCartridgeSlots[slot] = {};
}

void select_new_cartridge(size_t slot)
{
	FileDialog dlg;
	dlg.setTitle("Load Program Pak");
	dlg.setInitialDir(gLastAccessedPath.c_str());
	dlg.setFilter(
		"All Pak Types (*.dll; *.rom; *.ccc; *.pak)\0*.dll;*.ccc;*.rom;*.pak\0"
		"Hardware Pak (*.dll)\0*.dll\0"
		"Rom Pak (*.rom; *.ccc; *.pak)\0*.rom;*.ccc;*.pak\0"
		"\0");
	dlg.setFlags(OFN_FILEMUSTEXIST);
	if (dlg.show(0, gConfigurationDialogHandle))
	{
		eject_cartridge(slot);

		if (mount_cartridge(slot, dlg.path()) == cartridge_loader_status::success)
		{
			gLastAccessedPath = ::vcc::core::utils::get_directory_from_path(dlg.path());
		}
	}
}

void load_configuration()
{
	const auto section(::vcc::core::utils::load_string(gModuleInstance, IDS_MODULE_NAME));
	::vcc::core::configuration_serializer serializer(gConfigurationFilename);

	// Get default paths for modules
	gLastAccessedPath = serializer.read("DefaultPaths", "MPIPath");

	// Get the startup slot and set Chip select and SCS slots from ini file
	SwitchSlot = serializer.read(::vcc::core::utils::load_string(gModuleInstance, IDS_MODULE_NAME), "SWPOSITION", 3);
	ChipSelectSlot = SwitchSlot;
	SpareSelectSlot = SwitchSlot;

	// Mount them
	for (auto slot(0u); slot < gCartridgeSlots.size(); slot++)
	{
		const auto path(vcc::core::utils::find_pak_module_path(serializer.read(section, "SLOT" + std::to_string(slot + 1))));
		if (!path.empty())
		{
			mount_cartridge(slot, path);
		}
	}

	// Build the dynamic menu
	build_cartridge_menu();
}

void save_configuration()
{
	const auto section(vcc::core::utils::load_string(gModuleInstance, IDS_MODULE_NAME));
	::vcc::core::configuration_serializer serializer(gConfigurationFilename);

	serializer.write("DefaultPaths", "MPIPath", gLastAccessedPath);
	serializer.write(section, "SWPOSITION", SwitchSlot);
	for (auto slot(0u); slot < gCartridgeSlots.size(); slot++)
	{
		serializer.write(
			section,
			"SLOT" + ::std::to_string(slot + 1),
			vcc::core::utils::strip_application_path(gCartridgeSlots[slot].path()));
	}
}

std::string get_cartridge_description(size_t slot)
{
	vcc::core::utils::section_locker lock(gPakMutex);

	return "Module Name: " + gCartridgeSlots[slot].name();
}


// This gets called any time a cartridge menu is changed. It draws the entire menu.
void build_cartridge_menu()
{
	// Make sure we have access
	if (CartMenuCallback == nullptr)
	{
		MessageBox(nullptr, "MPI internal menu error", "Ok", 0);
		return;
	}

	vcc::core::utils::section_locker lock(gPakMutex);

	// Init the menu, establish MPI config control, build slot menus, then draw it.
	CartMenuCallback("", MID_BEGIN, MIT_Head);
	CartMenuCallback("", MID_ENTRY, MIT_Seperator);
	CartMenuCallback("MPI Config", ControlId(19), MIT_StandAlone);
	for (int slot = 3; slot >= 0; slot--)
	{
		gCartridgeSlots[slot].enumerate_menu_items(CartMenuCallback);
	}
	CartMenuCallback("", MID_FINISH, MIT_Head);  // Finish draws the entire menu
}

// Save cart Menu items into containers per slot
void append_menu_item_on_slot(size_t slot, CartMenuItem item)
{
	switch (item.menu_id) {
	case MID_BEGIN:
		{
			vcc::core::utils::section_locker lock(gPakMutex);

			gCartridgeSlots[slot].reset_menu();
		}
		break;

	case MID_FINISH:
		build_cartridge_menu();
		break;

	default:
		// Add 20 times the slot number to control id's
		if (item.menu_id >= MID_CONTROL)
		{
			item.menu_id += (slot + 1) * 20;
		}
		{
			vcc::core::utils::section_locker lock(gPakMutex);

			gCartridgeSlots[slot].append_menu_item(item);
		}
		break;
	}
}


void update_slot_ui_elements(size_t slot)
{
	vcc::core::utils::section_locker lock(gPakMutex);

	SendDlgItemMessage(
		gConfigurationDialogHandle,
		gAvailableSlotsDetails[slot].edit_box_id,
		WM_SETTEXT,
		0,
		reinterpret_cast<LPARAM>(gCartridgeSlots[slot].label().c_str()));

	SendDlgItemMessage(
		gConfigurationDialogHandle,
		gAvailableSlotsDetails[slot].insert_button_id,
		WM_SETTEXT,
		0,
		reinterpret_cast<LPARAM>(gCartridgeSlots[slot].empty() ? "..." : "X"));
}
