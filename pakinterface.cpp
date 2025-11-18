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
#include "defines.h"
#include "tcc1014mmu.h"
#include "tcc1014registers.h"
#include "CartridgeMenu.h"
#include "pakinterface.h"
#include "config.h"
#include "Vcc.h"
#include "mc6821.h"
#include "resource.h"
#include "vcc/bus/cartridges/empty_cartridge.h"
#include "vcc/utils/dll_deleter.h"
#include "vcc/utils/winapi.h"
#include "vcc/utils/logger.h"
#include "vcc/utils/FileOps.h"
#include "vcc/utils/filesystem.h"
#include "vcc/common/DialogOps.h"
#include <fstream>
#include <Windows.h>
#include <commdlg.h>


using cartridge_loader_status = vcc::utils::cartridge_loader_status;
using cartridge_loader_result = vcc::utils::cartridge_loader_result;


// Storage for Pak ROMs
extern SystemState EmuState;

static vcc::utils::critical_section gPakMutex;
static char DllPath[MAX_PATH] = "";
static cartridge_loader_result::handle_type gActiveModule;
static cartridge_loader_result::cartridge_ptr_type gActiveCartridge(std::make_unique<::vcc::bus::cartridges::empty_cartridge>());

static cartridge_loader_status load_any_cartridge(const char* filename, const char* iniPath);

void CartMenuCallBack(const char* name, int menu_id, MenuItemType type);
void PakAssertInterupt(Interrupt interrupt, InterruptSource source);
static void PakAssertCartrigeLine(void* host_key, bool line_state);
static void PakWriteMemoryByte(void* host_key, unsigned char data, unsigned short address);
static unsigned char PakReadMemoryByte(void* host_key, unsigned short address);
static void PakAssertInterupt(void* host_key, Interrupt interrupt, InterruptSource source);
static void PakAddMenuItem(void* host_key, const char* name, int menu_id, MenuItemType type);


class vcc_expansion_port_bus : public ::vcc::bus::expansion_port_bus
{
public:

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

	void set_cartridge_select_line(bool line_state) override
	{
		SetCart(line_state);
	}

	void assert_irq_interrupt_line() override
	{
		PakAssertInterupt(INT_IRQ, IS_NMI);
	}

	void assert_nmi_interrupt_line() override
	{
		PakAssertInterupt(INT_NMI, IS_NMI);
	}

	void assert_cartridge_interrupt_line() override
	{
		PakAssertInterupt(INT_CART, IS_NMI);
	}

};


class vcc_expansion_port_host : public ::vcc::bus::expansion_port_host
{
public:

	path_type configuration_path() const override
	{
		char path_buffer[MAX_PATH];

		GetIniFilePath(path_buffer);

		return path_buffer;
	}

	path_type system_rom_path() const override
	{
		return ::vcc::utils::get_directory_from_path(::vcc::utils::get_module_path(nullptr)) + "/ROMS/";
	}

};

class vcc_expansion_port_ui : public ::vcc::bus::expansion_port_ui
{
public:

	void add_menu_item(const char* menu_name, int menu_id, MenuItemType menu_type) override
	{
		CartMenuCallBack(menu_name, menu_id, menu_type);
	}
};

static void PakAssertCartrigeLine(void* /*host_key*/, bool line_state)
{
	SetCart(line_state);
}

static void PakWriteMemoryByte(void* /*host_key*/, unsigned char data, unsigned short address)
{
	MemWrite8(data, address);
}

static unsigned char PakReadMemoryByte(void* /*host_key*/, unsigned short address)
{
	return MemRead8(address);
}

static void PakAssertInterupt(void* /*host_key*/, Interrupt interrupt, InterruptSource source)
{
	PakAssertInterupt(interrupt, source);
}

static void PakAddMenuItem(void* /*host_key*/, const char* name, int menu_id, MenuItemType type)
{
	CartMenuCallBack(name, menu_id, type);
}

void PakTimer()
{
	vcc::utils::section_locker lock(gPakMutex);

	// FIXME: The timing here matches the horizontal sync frequency but should be
	// defined somewhere else and passed to this function.
	gActiveCartridge->update(1.0f / (60 * 262));
}

void ResetBus()
{
	vcc::utils::section_locker lock(gPakMutex);

	gActiveCartridge->reset();
}

void GetModuleStatus(SystemState *SMState)
{
	vcc::utils::section_locker lock(gPakMutex);

	gActiveCartridge->status(SMState->StatusLine, sizeof(SMState->StatusLine));
}

unsigned char PakReadPort (unsigned char port)
{
	vcc::utils::section_locker lock(gPakMutex);

	return gActiveCartridge->read_port(port);
}

void PakWritePort(unsigned char Port,unsigned char Data)
{
	vcc::utils::section_locker lock(gPakMutex);

	gActiveCartridge->write_port(Port,Data);
}

unsigned char PackMem8Read (unsigned short Address)
{
	vcc::utils::section_locker lock(gPakMutex);

	return gActiveCartridge->read_memory_byte(Address&32767);
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
	vcc::utils::section_locker lock(gPakMutex);

	return gActiveCartridge->sample_audio();
}

// Create first two entries for cartridge menu.
void BeginCartMenu()
{
	vcc::utils::section_locker lock(gPakMutex);

	CartMenu.add("", MID_BEGIN, MIT_Head, 0);
	CartMenu.add("Cartridge", MID_ENTRY, MIT_Head);
	if (!gActiveCartridge->name().empty())
	{
		char tmp[64] = {};
		snprintf(tmp, 64, "Eject %s", gActiveCartridge->name().c_str());
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

	auto error_string(load_error_string(result) + "\n\n" + filename);

	MessageBox(EmuState.WindowHandle, error_string.c_str(), "Load Error", MB_OK | MB_ICONERROR);

	return result;
}

// Insert Module returns 0 on success
static cartridge_loader_status load_any_cartridge(const char *filename, const char* iniPath)
{
	const auto gExpansionPortHost(std::make_shared<vcc_expansion_port_host>());
	cartridge_loader_result loadedCartridge(vcc::utils::load_cartridge(
		filename,
		gExpansionPortHost,
		std::make_unique<vcc_expansion_port_ui>(),
		std::make_unique<vcc_expansion_port_bus>()));
	if (loadedCartridge.load_result != cartridge_loader_status::success)
	{
		return loadedCartridge.load_result;
	}

	UnloadDll();

	vcc::utils::section_locker lock(gPakMutex);

	strcpy(DllPath, filename);
	gActiveCartridge = move(loadedCartridge.cartridge);
	gActiveModule = move(loadedCartridge.handle);
	BeginCartMenu();
	gActiveCartridge->start();

	// Reset if enabled
	EmuState.ResetPending = 2;

	return loadedCartridge.load_result;
}


void UnloadDll()
{
	vcc::utils::section_locker lock(gPakMutex);

	gActiveCartridge->stop();

	gActiveCartridge = std::make_unique<::vcc::bus::cartridges::empty_cartridge>();
	gActiveModule.reset();

	BeginCartMenu();
	gActiveCartridge->start();
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

	vcc::utils::section_locker lock(gPakMutex);

	gActiveCartridge->menu_item_clicked(MenuID);
}
