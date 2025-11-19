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
//
//----------------------------------------------------------------------
// SDC simulator E J Jaquay 2025 
//----------------------------------------------------------------------
//#define USE_LOGGING

#include <Windows.h>

#include "vcc/bus/cartridge_factory.h"
#include "vcc/devices/rtc/ds1315.h"
#include "vcc/utils/logger.h"
#include "vcc/utils/winapi.h"
#include "vcc/common/DialogOps.h"
#include "resource.h"
#include "sdc_cartridge.h"
#include "sdc_configuration.h"

HINSTANCE gModuleInstance;

//======================================================================
// Internal functions
//======================================================================

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID rsvd)
{
    if (reason == DLL_PROCESS_ATTACH) {
        gModuleInstance = hinst;
    }
    return TRUE;
}


//
extern "C" __declspec(dllexport) ::vcc::bus::cartridge_factory_prototype GetPakFactory()
{
	return [](
		std::shared_ptr<::vcc::bus::expansion_port_host> host,
		std::unique_ptr<::vcc::bus::expansion_port_ui> ui,
		std::unique_ptr<::vcc::bus::expansion_port_bus> bus) -> ::vcc::bus::cartridge_factory_result
		{
			return std::make_unique<sdc_cartridge>(move(host), move(ui), move(bus), gModuleInstance);
		};
}

static_assert(
	std::is_same_v<decltype(&GetPakFactory), ::vcc::bus::create_cartridge_factory_prototype>,
	"GMC GetPakFactory does not have the correct signature.");


sdc_cartridge::sdc_cartridge(
	std::shared_ptr<expansion_port_host_type> host,
	std::unique_ptr<expansion_port_ui_type> ui,
	std::unique_ptr<expansion_port_bus_type> bus,
	HINSTANCE module_instance)
	:
	host_(move(host)),
	ui_(move(ui)),
	bus_(move(bus)),
	module_instance_(module_instance)
{
}


sdc_cartridge::name_type sdc_cartridge::name() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME);
}

sdc_cartridge::catalog_id_type sdc_cartridge::catalog_id() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_CATNUMBER);
}

sdc_cartridge::description_type sdc_cartridge::description() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_DESCRIPTION);
}

void sdc_cartridge::start()
{
    strcpy(IniFile, host_->configuration_path().c_str());

    LoadConfig();
}

void sdc_cartridge::stop()
{
    CloseCartDialog(hControlDlg);
    CloseCartDialog(hConfigureDlg);
    hControlDlg = nullptr;
    hConfigureDlg = nullptr;
    CloseDrive(0);
    CloseDrive(1);
}

// Write to port
void sdc_cartridge::write_port(unsigned char Port, unsigned char Data)
{
    SDCWrite(Data,Port);
    return;
}

// Read from port
unsigned char sdc_cartridge::read_port(unsigned char Port)
{
    if (ClockEnable && ((Port==0x78) | (Port==0x79) | (Port==0x7C))) {
        return ds1315_rtc.read_port(Port);
    } else if ((Port > 0x3F) & (Port < 0x60)) {
        return SDCRead(Port);
    } else {
        return 0;
    }
}

// Reset module
void sdc_cartridge::reset()
{
    SDCInit();
}

//  Dll export run config dialog
void sdc_cartridge::menu_item_clicked(unsigned char MenuID)
{
    switch (MenuID)
    {
    case 10:
        if (hConfigureDlg == nullptr)  // Only create dialog once
            hConfigureDlg = CreateDialog(gModuleInstance, (LPCTSTR) IDD_CONFIG,
                        GetActiveWindow(), (DLGPROC) SDC_Configure);
        ShowWindow(hConfigureDlg,1);
        break;
    case 11:
        if (hControlDlg == nullptr)
            hControlDlg = CreateDialog(gModuleInstance, (LPCTSTR) IDD_CONTROL,
                        GetActiveWindow(), (DLGPROC) SDC_Control);
        ShowWindow(hControlDlg,1);
        break;
    }
}

// Return SDC status.
void sdc_cartridge::status(char* text_buffer, size_t buffer_size)
{
    strncpy(text_buffer,SDC_Status,buffer_size);
    if (idle_ctr < 100) {
        idle_ctr++;
    } else {
        idle_ctr = 0;
        snprintf(SDC_Status,16,"SDC:%d idle",CurrentBank);
    }
}

// Return a byte from the current PAK ROM
unsigned char sdc_cartridge::read_memory_byte(size_type adr)
{
    adr &= 0x3FFF;
    if (EnableBankWrite) {
        return WriteFlashBank(static_cast<unsigned short>(adr));
    } else {
        BankWriteState = 0;  // Any read resets write state
        return(PakRom[adr]);
    }
}

