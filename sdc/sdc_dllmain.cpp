
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

#include "vcc/devices/rtc/ds1315.h"
#include "vcc/utils/logger.h"
#include "vcc/utils/winapi.h"
#include "vcc/common/DialogOps.h"
#include "vcc/bus/interrupts.h"
#include "vcc/bus/cartridge_capi.h"
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
        CartMenuCallback = context->add_menu_item;
        MemRead8 = context->read_memory_byte;
        MemWrite8 = context->write_memory_byte;
        AssertInt = context->assert_interrupt;
        strcpy(IniFile, configuration_path);

        LoadConfig();
        BuildCartridgeMenu();
    }

    __declspec(dllexport) void PakTerminate()
    {
        CloseCartDialog(hControlDlg);
        CloseCartDialog(hConfigureDlg);
        hControlDlg = nullptr;
        hConfigureDlg = nullptr;
        CloseDrive(0);
        CloseDrive(1);
    }

    // Write to port
    __declspec(dllexport) void PakWritePort(unsigned char Port,unsigned char Data)
    {
        SDCWrite(Data,Port);
        return;
    }

    // Read from port
    __declspec(dllexport) unsigned char PakReadPort(unsigned char Port)
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
    __declspec(dllexport) void PakReset()
    {
        SDCInit();
    }

    //  Dll export run config dialog
    __declspec(dllexport) void PakMenuItemClicked(unsigned char MenuID)
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
        BuildCartridgeMenu();
        return;
    }

    // Return SDC status.
    __declspec(dllexport) void PakGetStatus(char* text_buffer, size_t buffer_size)
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
    __declspec(dllexport) unsigned char PakReadMemoryByte(unsigned short adr)
    {
        adr &= 0x3FFF;
        if (EnableBankWrite) {
            return WriteFlashBank(adr);
        } else {
            BankWriteState = 0;  // Any read resets write state
            return(PakRom[adr]);
        }
    }
}

