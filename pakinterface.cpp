/*
    Copyright 2015 by Joseph Forgione
    This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

#include "defines.h"
#include "tcc1014mmu.h"
#include "tcc1014registers.h"
#include "CartridgeMenu.h"
#include "pakinterface.h"
#include "config.h"
#include "Vcc.h"
#include "mc6821.h"
#include "resource.h"
#include <vcc/core/limits.h>
#include <vcc/core/legacy_cartridge_definitions.h>
#include <vcc/core/cartridges/null_cartridge.h>
#include <vcc/core/utils/dll_deleter.h>
#include <vcc/core/utils/winapi.h>
#include <vcc/common/logger.h>
#include <vcc/common/FileOps.h>
#include <vcc/common/DialogOps.h>
#include <fstream>
#include <Windows.h>
#include <commdlg.h>


using cartridge_loader_status = vcc::core::cartridge_loader_status;
using cartridge_loader_result = vcc::core::cartridge_loader_result;


// Storage for Pak ROMs
extern SystemState EmuState;

static vcc::core::utils::critical_section gPakMutex;
static char DllPath[MAX_PATH] = "";
static cartridge_loader_result::handle_type gActiveModule;
static cartridge_loader_result::cartridge_ptr_type gActiveCartrige(std::make_unique<vcc::core::cartridges::null_cartridge>());

static cartridge_loader_status load_any_cartridge(const char* filename, const char* iniPath);

void CartMenuCallBack(const char* name, int menu_id, MenuItemType type);
void PakAssertInterupt(Interrupt interrupt, InterruptSource source);


struct vcc_cartridge_context : public ::vcc::core::cartridge_context
{

	path_type configuration_path() const override
	{
		char path_buffer[MAX_PATH];

		GetIniFilePath(path_buffer);

		return path_buffer;
	}

	void reset() override
	{
		EmuState.ResetPending = 2;
	}

	void write_memory_byte(unsigned char value, unsigned short address) override
	{
		MemWrite8(value, address);
	}

	unsigned char read_memory_byte(unsigned short address) override
	{
		return MemRead8(address);
	}

	void assert_cartridge_line(bool line_state) override
	{
		SetCart(line_state);
	}

	void assert_interrupt(Interrupt interrupt, InterruptSource interrupt_source) override
	{
		PakAssertInterupt(interrupt, interrupt_source);
	}

	void add_menu_item(const char* menu_name, int menu_id, MenuItemType menu_type) override
	{
		CartMenuCallBack(menu_name, menu_id, menu_type);
	}
};

void PakTimer()
{
	vcc::core::utils::section_locker lock(gPakMutex);

	gActiveCartrige->heartbeat();
}

void ResetBus()
{
	vcc::core::utils::section_locker lock(gPakMutex);

	gActiveCartrige->reset();
}

void GetModuleStatus(SystemState *SMState)
{
	vcc::core::utils::section_locker lock(gPakMutex);

	gActiveCartrige->status(SMState->StatusLine);
}

unsigned char PackPortRead (unsigned char port)
{
	vcc::core::utils::section_locker lock(gPakMutex);

	return gActiveCartrige->read_port(port);
}

void PackPortWrite(unsigned char Port,unsigned char Data)
{
	vcc::core::utils::section_locker lock(gPakMutex);

	gActiveCartrige->write_port(Port,Data);
}

unsigned char PackMem8Read (unsigned short Address)
{
	vcc::core::utils::section_locker lock(gPakMutex);

	return gActiveCartrige->read_memory_byte(Address&32767);
}

// Convert PAK interrupt assert to CPU assert or Gime assert.
void PakAssertInterupt(Interrupt interrupt, InterruptSource source)
{
	(void) source; // not used

	switch (interrupt) {
	case INT_CART:
		GimeAssertCartInterupt();
		break;
	case INT_NMI:
		CPUAssertInterupt(IS_NMI, INT_NMI);
		break;
	}
}

unsigned short PackAudioSample()
{
	vcc::core::utils::section_locker lock(gPakMutex);

	return gActiveCartrige->sample_audio();
}

// Create first two entries for cartridge menu.
void BeginCartMenu()
{
	vcc::core::utils::section_locker lock(gPakMutex);

	CartMenu.add("", MID_BEGIN, MIT_Head, 0);
	CartMenu.add("Cartridge", MID_ENTRY, MIT_Head);
	if (!gActiveCartrige->name().empty())
	{
		char tmp[64] = {};
		snprintf(tmp, 64, "Eject %s", gActiveCartrige->name().c_str());
		CartMenu.add(tmp, ControlId(2), MIT_Slave);
	}
	CartMenu.add("Load Cart", ControlId(1), MIT_Slave);
	CartMenu.add("", MID_FINISH, MIT_Head);
}

// Callback for loaded cart DLLs. First two entries are reserved
void CartMenuCallBack(const char *name, int menu_id, MenuItemType type)
{
	CartMenu.add(name, menu_id, type, 2);
}


void PakLoadCartridgeUI()
{
	char inifile[MAX_PATH];
	GetIniFilePath(inifile);

	static char pakPath[MAX_PATH] = "";
	GetPrivateProfileString("DefaultPaths", "PakPath", "", pakPath, MAX_PATH, inifile);
	FileDialog dlg;
	dlg.setTitle(TEXT("Load Program Pack"));
	dlg.setInitialDir(pakPath);
	dlg.setFilter("All Supported Formats (*.dll;*.ccc;*.rom)\0*.dll;*.ccc;*.rom\0DLL Packs\0*.dll\0Rom Packs\0*.ROM;*.ccc;*.pak\0\0");
	dlg.setFlags(OFN_FILEMUSTEXIST);
	if (dlg.show()) {
		if (PakLoadCartridge(dlg.path()) == cartridge_loader_status::success) {
			dlg.getdir(pakPath);
			WritePrivateProfileString("DefaultPaths", "PakPath", pakPath, inifile);
		}
	}
}

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

	char TempIni[MAX_PATH]="";
	GetIniFilePath(TempIni);

	const auto result(load_any_cartridge(filename, TempIni));
	if (result == cartridge_loader_status::success)
	{
		return result;
	}

	const auto string_id_ptr(string_id_map.find(result));
	auto error_string(vcc::core::utils::load_string(
		EmuState.WindowInstance,
		string_id_ptr != string_id_map.end() ? string_id_ptr->second : IDS_UNKNOWN_ERROR));

	error_string += "\n\n";
	error_string += filename;

	MessageBox(EmuState.WindowHandle, error_string.c_str(), "Load Error", MB_OK | MB_ICONERROR);

	return result;
}

// Insert Module returns 0 on success
static cartridge_loader_status load_any_cartridge(const char *filename, const char* iniPath)
{
	cartridge_loader_result loadedCartridge(vcc::core::load_cartridge(
		std::make_unique<vcc_cartridge_context>(),
		{
			CartMenuCallBack,
			MemRead8,
			MemWrite8,
			PakAssertInterupt,
			SetCart
		},
		filename,
		iniPath));
	if (loadedCartridge.load_result != cartridge_loader_status::success)
	{
		return loadedCartridge.load_result;
	}

	UnloadDll();

	vcc::core::utils::section_locker lock(gPakMutex);

	strcpy(DllPath, filename);
	gActiveCartrige = move(loadedCartridge.cartridge);
	gActiveModule = move(loadedCartridge.handle);
	BeginCartMenu();
	gActiveCartrige->start();

	// Reset if enabled
	EmuState.ResetPending = 2;

	return loadedCartridge.load_result;
}


void UnloadDll()
{
	vcc::core::utils::section_locker lock(gPakMutex);

	gActiveCartrige = std::make_unique<vcc::core::cartridges::null_cartridge>();
	gActiveModule.reset();

	BeginCartMenu();
	gActiveCartrige->start();
}

void GetCurrentModule(char *DefaultModule)
{
	strcpy(DefaultModule,DllPath);
	return;
}

void UpdateBusPointer()
{
	// Do nothing for now
}

void UnloadPack()
{
	UnloadDll();
	strcpy(DllPath,"");
	SetCart(0);

	EmuState.ResetPending=2;
}


// CartMenuActivated is called from VCC main when a cartridge menu item is clicked.
void CartMenuActivated(unsigned int MenuID)
{
	switch (MenuID)
	{
	case 1:
		LoadPack();
		return;

	case 2:
		UnloadPack();
		return;

	default:
		break;
	}

	vcc::core::utils::section_locker lock(gPakMutex);

	gActiveCartrige->menu_item_clicked(MenuID);
}
