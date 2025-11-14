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
#include "pakinterface.h"
#include "config.h"
#include "Vcc.h"
#include "mc6821.h"
#include "resource.h"
#include "vcc/bus/expansion_port.h"
#include "vcc/utils/cartridge_catalog.h"
#include "vcc/utils/dll_deleter.h"
#include "vcc/utils/winapi.h"
#include "vcc/utils/logger.h"
#include "vcc/utils/FileOps.h"
#include "vcc/utils/filesystem.h"
#include "vcc/common/DialogOps.h"
#include <fstream>
#include <Windows.h>
#include <commdlg.h>
#include <mutex>


using cartridge_loader_status = vcc::utils::cartridge_loader_status;
using cartridge_loader_result = vcc::utils::cartridge_loader_result;
template<class T_>
using expansion_port = vcc::bus::expansion_port<T_>;


// Storage for Pak ROMs
extern SystemState EmuState;

static std::recursive_mutex gCartridgeMutex;
static std::recursive_mutex gDriverMutex;
static expansion_port<std::recursive_mutex> gExpansionSlot(gCartridgeMutex, gDriverMutex);
const vcc::utils::cartridge_catalog cartridge_catalog_(
	std::filesystem::path(::vcc::utils::get_module_path())
	.parent_path()
	.append("Cartridges"));

void PakAssertInterupt(Interrupt interrupt, InterruptSource source);
static void PakAssertCartrigeLine(void* host_key, bool line_state);
static void PakWriteMemoryByte(void* host_key, unsigned char data, unsigned short address);
static unsigned char PakReadMemoryByte(void* host_key, unsigned short address);
static void PakAssertInterupt(void* host_key, Interrupt interrupt, InterruptSource source);


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

	explicit vcc_expansion_port_host(const catalog_type& cartridge_items)
		: cartridge_catalog_(cartridge_items)
	{}
	

	path_type configuration_path() const override
	{
		char path_buffer[MAX_PATH];

		GetIniFilePath(path_buffer);

		return path_buffer;
	}

	path_type system_cartridge_path() const override
	{
		// TODO-CHET: Once get_module_path is changed to return a path remove
		// the explicit conversion.
		return path_type(::vcc::utils::get_module_path(nullptr))
			.parent_path()
			.append("cartridges");
	}

	path_type system_rom_path() const override
	{
		return PakGetSystemCartridgePath();
	}

	[[nodiscard]] catridge_mutex_type& driver_mutex() const override
	{
		return gDriverMutex;
	}

	const catalog_type& cartridge_catalog() const override
	{
		return cartridge_catalog_;
	}


private:

	const catalog_type& cartridge_catalog_;
};

class vcc_expansion_port_ui : public ::vcc::bus::expansion_port_ui
{
public:

	HWND app_window() const noexcept override
	{
		return EmuState.WindowHandle;
	}

	[[nodiscard]] path_type last_accessed_rompak_path() const override
	{
		return PakGetLastAccessedRomPakPath();
	}

	void last_accessed_rompak_path(path_type path) const override
	{
		PakSetLastAccessedRomPakPath(path);
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

std::filesystem::path PakGetSystemCartridgePath()
{
	// TODO-CHET: Once get_module_path is changed to return a path remove
	// the explicit conversion.
	return std::filesystem::path(::vcc::utils::get_module_path(nullptr))
		.parent_path()
		.append("cartridges");
}

std::filesystem::path PakGetLastAccessedRomPakPath()
{
	char inifile[MAX_PATH];
	GetIniFilePath(inifile);

	static char pakPath[MAX_PATH] = "";
	GetPrivateProfileString(
		"DefaultPaths",
		"PakPath",
		"",
		pakPath,
		MAX_PATH,
		inifile);

	return pakPath;
}

void PakSetLastAccessedRomPakPath(const std::filesystem::path& path)
{
	char inifile[MAX_PATH];
	GetIniFilePath(inifile);

	WritePrivateProfileString(
		"DefaultPaths",
		"PakPath",
		path.string().c_str(),
		inifile);
}

std::string PakGetName()
{
	return gExpansionSlot.name();
}

void PakTimer()
{
	// TODO-CHET: The timing here matches the horizontal sync frequency but should be
	// defined somewhere else and passed to this function.
	gExpansionSlot.update(1.0f / (60 * 262));
}

void ResetBus()
{
	gExpansionSlot.reset();
}

void GetModuleStatus(SystemState *SMState)
{
	strncpy_s(
		SMState->StatusLine,
		gExpansionSlot.status().c_str(),
		sizeof(SMState->StatusLine));
}

unsigned char PakReadPort (unsigned char port)
{
	return gExpansionSlot.read_port(port);
}

void PakWritePort(unsigned char Port,unsigned char Data)
{
	gExpansionSlot.write_port(Port,Data);
}

unsigned char PackMem8Read (unsigned short Address)
{
	return gExpansionSlot.read_memory_byte(Address&32767);
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
	return gExpansionSlot.sample_audio();
}

// Create first two entries for cartridge menu.
::vcc::ui::menu::menu_item_collection PakGetMenuItems()
{
	return gExpansionSlot.get_menu_items();
}



void PakInsertCartridge(
	cartridge_loader_result::handle_type handle,
	cartridge_loader_result::cartridge_ptr_type cartridge);


cartridge_loader_status PakInsertCartridge(const ::vcc::utils::cartridge_catalog::guid_type& id)
{
	if (!cartridge_catalog_.is_valid_cartridge_id(id))
	{
		return cartridge_loader_status::unknown_cartridge;
	}

	const auto filename(cartridge_catalog_.get_item_pathname(id).string());

	cartridge_loader_result loadedCartridge(vcc::utils::load_library_cartridge(
		filename,
		std::make_shared<vcc_expansion_port_host>(cartridge_catalog_),
		std::make_unique<vcc_expansion_port_ui>(),
		std::make_unique<vcc_expansion_port_bus>()));
	if (loadedCartridge.load_result != cartridge_loader_status::success)
	{
		return loadedCartridge.load_result;
	}

	PakInsertCartridge(move(loadedCartridge.handle), move(loadedCartridge.cartridge));

	return loadedCartridge.load_result;
}

cartridge_loader_status PakInsertRom(const std::filesystem::path& filename)
{
	cartridge_loader_result loadedCartridge(vcc::utils::load_rom_cartridge(
		filename,
		std::make_shared<vcc_expansion_port_host>(cartridge_catalog_),
		std::make_unique<vcc_expansion_port_ui>(),
		std::make_unique<vcc_expansion_port_bus>()));
	if (loadedCartridge.load_result != cartridge_loader_status::success)
	{
		return loadedCartridge.load_result;
	}

	PakInsertCartridge(move(loadedCartridge.handle), move(loadedCartridge.cartridge));

	return loadedCartridge.load_result;
}

void PakInsertCartridge(
	cartridge_loader_result::handle_type handle,
	cartridge_loader_result::cartridge_ptr_type cartridge)
{
	std::scoped_lock lock(gCartridgeMutex, gDriverMutex);

	PakEjectCartridge(false);

	gExpansionSlot.insert(move(handle), move(cartridge));
	gExpansionSlot.start();

	// Reset if enabled
	EmuState.ResetPending = 2;
}

void PakEjectCartridge(bool reset_machine)
{
	std::scoped_lock lock(gCartridgeMutex, gDriverMutex);

	gExpansionSlot.stop();
	gExpansionSlot.eject();

	// Clear the cartridge line and start the cartridge
	SetCart(false);
	gExpansionSlot.start();	//	FIXME-CHET: Do we really need to call this here?

	if (reset_machine)
	{
		EmuState.ResetPending = 2;
	}
}

void UpdateBusPointer()
{
	// Do nothing for now
}

// CartMenuActivated is called from VCC main when a cartridge menu item is clicked.
void CartMenuActivated(unsigned int MenuID)
{
	gExpansionSlot.menu_item_clicked(MenuID);
}
