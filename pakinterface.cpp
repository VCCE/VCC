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
#include <vcc/bus/null_cartridge.h>
#include <vcc/bus/dll_deleter.h>
#include <vcc/core/limits.h>
#include <vcc/core/winapi.h>
#include <vcc/core/logger.h>
#include <vcc/core/FileOps.h>
#include <vcc/core/DialogOps.h>
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

static void PakAssertCartrigeLine(void* /*callback_context*/, bool line_state)
{
	SetCart(line_state);
}

static void PakWriteMemoryByte(void* /*callback_context*/, unsigned char data, unsigned short address)
{
	MemWrite8(data, address);
}

static unsigned char PakReadMemoryByte(void* /*callback_context*/, unsigned short address)
{
	return MemRead8(address);
}

static void PakAssertInterupt(void* /*callback_context*/, Interrupt interrupt, InterruptSource source)
{
	PakAssertInterupt(interrupt, source);
}

static void PakAddMenuItem(void* /*callback_context*/, const char* name, int menu_id, MenuItemType type)
{
	CartMenuCallBack(name, menu_id, type);
}

void PakTimer()
{
	vcc::core::utils::section_locker lock(gPakMutex);

	gActiveCartrige->process_horizontal_sync();
}

void ResetBus()
{
	vcc::core::utils::section_locker lock(gPakMutex);

	gActiveCartrige->reset();
}

void GetModuleStatus(SystemState *SMState)
{
	vcc::core::utils::section_locker lock(gPakMutex);

	gActiveCartrige->status(SMState->StatusLine, sizeof(SMState->StatusLine));
}

unsigned char PakReadPort (unsigned char port)
{
	vcc::core::utils::section_locker lock(gPakMutex);

	return gActiveCartrige->read_port(port);
}

void PakWritePort(unsigned char Port,unsigned char Data)
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

// Create entries for cartridge menu. The rest will be for MPI
// ControlId(MenuId) set what control does
void BeginCartMenu()
{
	vcc::core::utils::section_locker lock(gPakMutex);

	CartMenu.reserve(0);
	CartMenu.add("", MID_BEGIN, MIT_Head);
	CartMenu.add("Cartridge", MID_ENTRY, MIT_Head);
	if (!gActiveCartrige->name().empty())
	{
		char tmp[64] = {};
		snprintf(tmp, 64, "Eject %s", gActiveCartrige->name().c_str());
		CartMenu.add(tmp, ControlId(2), MIT_Slave);
		CartMenu.add("", MID_FINISH, MIT_Head);
		CartMenu.reserve(2);
	} else {
		CartMenu.add("Load MPI", ControlId(3), MIT_Slave);
		CartMenu.add("Load DLL", ControlId(1), MIT_Slave);
		CartMenu.add("Load ROM", ControlId(4), MIT_Slave);
		CartMenu.add("", MID_FINISH, MIT_Head);
		CartMenu.reserve(3);
	}
}

// Callback for loaded cart DLLs.
void CartMenuCallBack(const char *name, int menu_id, MenuItemType type)
{
	CartMenu.add(name, menu_id, type);
}


void PakLoadCartridgeUI(int type)
{
	char inifile[MAX_PATH];
	GetIniFilePath(inifile);

	static char cartDir[MAX_PATH] = "";
	FileDialog dlg;
	if (type == 0) {
		dlg.setTitle(TEXT("Load Program Pack"));
		dlg.setFilter("Hardware Packs\0*.dll\0All Supported Formats (*.dll;*.ccc;*.rom)\0*.dll;*.ccc;*.rom\0\0");
		GetPrivateProfileString("DefaultPaths", "MPIPath", "", cartDir, MAX_PATH, inifile);
	} else {
		dlg.setTitle(TEXT("Load ROM"));
		dlg.setFilter("Rom Packs(*.ccc;*.rom)\0*.ccc;*.rom\0All Supported Formats (*.dll;*.ccc;*.rom)\0*.dll;*.ccc;*.rom\0\0");
		GetPrivateProfileString("DefaultPaths", "PakPath", "", cartDir, MAX_PATH, inifile);
	}
	dlg.setInitialDir(cartDir);
	dlg.setFlags(OFN_FILEMUSTEXIST);
	if (dlg.show()) {
		if (PakLoadCartridge(dlg.path()) == cartridge_loader_status::success) {
			char filetype[4];
			dlg.getdir(cartDir);
			dlg.gettype(filetype);
			if ((strcmp(filetype,"dll") == 0) | (strcmp(filetype,"DLL") == 0 )) {  // DLL?
				WritePrivateProfileString("DefaultPaths", "MPIPath", cartDir, inifile);
			} else {
				WritePrivateProfileString("DefaultPaths", "PAKPath", cartDir, inifile);
			}
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

    // Tell user why load failed
	auto error_string(vcc::core::cartridge_load_error_string(result));
	error_string += "\n\n";
	error_string += filename;
	MessageBox(EmuState.WindowHandle, error_string.c_str(), "Load Error", MB_OK | MB_ICONERROR);

	return result;
}

// Insert Module returns 0 on success
static cartridge_loader_status load_any_cartridge(const char *filename, const char* iniPath)
{
	cartridge_loader_result loadedCartridge(vcc::core::load_cartridge(
		filename,                                   // Cartridge filename
		std::make_unique<vcc_cartridge_context>(),  // Yet another cartridge context
		nullptr,                                    // Maybe a VCC host context
		iniPath,                                    // Path of ini file
		EmuState.WindowHandle,                      // HWND hVccWnd
		{                                           // Callbacks AKA context
			PakAssertInterupt,
			PakAssertCartrigeLine,
			PakWriteMemoryByte,
			PakReadMemoryByte,
			PakAddMenuItem
		}));
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

	gActiveCartrige->stop();

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
	// Do nothing for now.
	// No clue given what the plan here was.
}

void UnloadPack()
{
	UnloadDll();
	strcpy(DllPath,"");
	SetCart(0);
	EmuState.ResetPending=2;
}

void LoadPack(int type) {
    PakLoadCartridgeUI(type);
	EmuState.ResetPending=2;
}

// CartMenuActivated is called from VCC main when a cartridge menu item is clicked.
void CartMenuActivated(unsigned int MenuID)
{
	switch (MenuID)
	{
	case 1:
		LoadPack(0);
		return;

	case 2:
		UnloadPack();
		return;

	case 3:
	{
		char path[MAX_PATH];
		GetModuleFileName(nullptr,path,MAX_PATH);
		PathRemoveFileSpec(path);
		strncat(path,"mpi.dll",MAX_PATH);
		PakLoadCartridge(path);
		return;
    }
	case 4:
		LoadPack(1);
		return;

	default:
		break;
	}

	vcc::core::utils::section_locker lock(gPakMutex);

	gActiveCartrige->menu_item_clicked(MenuID);
}
