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

// hardisk.cpp : Defines the entry point for the DLL application.

#include <Windows.h>
#include <stdio.h>
#include<iostream>
#include "resource.h"
#include "cc3vhd.h"
#include <vcc/devices/rtc/ds1315.h>
#include <vcc/utils/FileOps.h>
#include <vcc/common/DialogOps.h>
#include "../CartridgeMenu.h"
#include <vcc/utils/winapi.h>
#include <vcc/core/interrupts.h>
#include <vcc/core/cartridge_capi.h>

constexpr auto DEF_HD_SIZE = 132480u;

static char VHDfile0[MAX_PATH] { 0 };
static char VHDfile1[MAX_PATH] { 0 };
static char *VHDfile; // Selected drive file name
static char NewVHDfile[MAX_PATH];
static char IniFile[MAX_PATH]  { 0 };
static char HardDiskPath[MAX_PATH];
static ::vcc::devices::rtc::ds1315 ds1315_rtc;
static const char* const gConfigurationSection = "Hard Drive";

static void* gHostKey = nullptr;
static PakReadMemoryByteHostCallback MemRead8 = nullptr;
static PakWriteMemoryByteHostCallback MemWrite8 = nullptr;
static PakAppendCartridgeMenuHostCallback CartMenuCallback = nullptr;
static bool ClockEnabled = true;
static bool ClockReadOnly = true;
LRESULT CALLBACK NewDisk(HWND,UINT, WPARAM, LPARAM);

///void Load_Disk(unsigned char);
void LoadHardDisk(int drive);
void LoadConfig();
void SaveConfig();
void BuildCartridgeMenu();
int CreateDisk(HWND,int);

static HINSTANCE gModuleInstance;
static HWND hConfDlg = nullptr;
LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM );

using namespace std;

BOOL WINAPI DllMain( HINSTANCE hinstDLL,  // handle to DLL module
                     DWORD fdwReason,     // reason for calling function
                     LPVOID lpReserved )  // reserved
{
    if (fdwReason == DLL_PROCESS_ATTACH)
	{
		gModuleInstance = hinstDLL;
    }

    return TRUE;
}

void MemWrite(unsigned char Data, unsigned short Address)
{
	MemWrite8(gHostKey, Data, Address);
}

unsigned char MemRead(unsigned short Address)
{
	return MemRead8(gHostKey, Address);
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
		strcpy(IniFile, configuration_path);

		LoadConfig();
		ds1315_rtc.set_read_only(ClockReadOnly);
		VhdReset(); // Selects drive zero
		BuildCartridgeMenu();
	}

	__declspec(dllexport) void PakTerminate()
	{
		CloseCartDialog(hConfDlg);
		UnmountHD(0);
		UnmountHD(1);
	}

}


// Configure the hard drive(s).  Called from menu
// Mount or dismount a hard drive and save config
// MountHD and UnmountHD are defined in cc3vhd
extern "C"
{
    __declspec(dllexport) void
	PakMenuItemClicked(unsigned char MenuID)
    {
        switch (MenuID)
        {
        case 10:
            LoadHardDisk(0);
            break;

        case 11:
            UnmountHD(0);
            *VHDfile0 = '\0';
            break;

        case 12:
            LoadHardDisk(1);
            break;

        case 13:
            UnmountHD(1);
            *VHDfile1 = '\0';
            break;

        case 14:
            if (hConfDlg == nullptr)
                hConfDlg = CreateDialog(gModuleInstance,(LPCTSTR)IDD_CONFIG,GetActiveWindow(),(DLGPROC)Config);
            ShowWindow(hConfDlg,1);
            return;
        }
        SaveConfig();
		BuildCartridgeMenu();
        return;
    }
}

LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (message)
    {
    case WM_CLOSE:
        DestroyWindow(hDlg);
        hConfDlg=nullptr;
        return TRUE;

    case WM_INITDIALOG:
        CenterDialog(hDlg);
		SendDlgItemMessage(hDlg, IDC_CLOCK, BM_SETCHECK, ClockEnabled, 0);
		SendDlgItemMessage(hDlg, IDC_READONLY, BM_SETCHECK, ClockReadOnly, 0);
        EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), TRUE);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
			ClockEnabled = SendDlgItemMessage(hDlg, IDC_CLOCK, BM_GETCHECK, 0, 0) != 0;
			ClockReadOnly = SendDlgItemMessage(hDlg, IDC_READONLY, BM_GETCHECK, 0, 0) != 0;
			ds1315_rtc.set_read_only(ClockReadOnly);
            SaveConfig();
            DestroyWindow(hDlg);
            hConfDlg=nullptr;
            break;

        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            DestroyWindow(hDlg);
            hConfDlg=nullptr;
        break;
        }
    }
    return 0;
}

// Export write to HD control port
extern "C"
{
    __declspec(dllexport) void
	PakWritePort(unsigned char Port,unsigned char Data)
    {
        IdeWrite(Data,Port);
        return;
    }
}

// Export read from HD control port
extern "C"
{
    __declspec(dllexport) unsigned char PakReadPort(unsigned char Port)
    {
		if (((Port == 0x78) | (Port == 0x79) | (Port == 0x7C)) & ClockEnabled)
			return ds1315_rtc.read_port(Port);
        return(IdeRead(Port));
    }
}


// Return disk status. (from cc3vhd)
extern "C"
{
    __declspec(dllexport) void
	PakGetStatus(char* text_buffer, size_t buffer_size)
    {
        DiskStatus(text_buffer, buffer_size);
    }
}


// Get filename from user and mount harddrive
void LoadHardDisk(int drive)
{
    char msg[300];

    // Select VHD FileName buffer as per drive
    switch (drive) {
    case 0:
        VHDfile = VHDfile0;
        break;
    case 1:
        VHDfile = VHDfile1;
        break;
    default:
        return;
    }

    HWND hWnd = GetActiveWindow();
    FileDialog dlg;
    dlg.setpath(VHDfile);
    dlg.setFilter("Hard Disk Images\0*.vhd;*.os9;*.img\0All files\0*.*\0\0");
    dlg.setDefExt("vhd");
    dlg.setTitle(TEXT("Load HardDisk Image") );
    dlg.setInitialDir(HardDiskPath);
    dlg.setFlags(OFN_PATHMUSTEXIST);
    if (dlg.show(0,hWnd)) {
        dlg.getpath(NewVHDfile,MAX_PATH);

        // Present new disk dialog if file does not exist
        if (GetFileAttributes(NewVHDfile) == INVALID_FILE_ATTRIBUTES) {
            // Dialog box returns zero if file is not created
            if (DialogBox(gModuleInstance,(LPCTSTR)IDD_NEWDISK,hWnd,(DLGPROC)NewDisk)==0)
                return;
        }

        // Actual file mount is done in cc3vhd
        if (MountHD(NewVHDfile,drive)==0) {
            snprintf(msg,300,"Can't mount %s",NewVHDfile);
            MessageBox(hWnd,msg,"Error",0);
            *VHDfile = '\0';
            return;
        }

        strncpy(VHDfile,NewVHDfile,MAX_PATH);

        // Save vhd directory for config file
        dlg.getdir(HardDiskPath);
        SaveConfig();
    }
    return;
}

// Get configuration items from ini file
void LoadConfig()
{
    HANDLE hr;

    GetPrivateProfileString("DefaultPaths", "HardDiskPath", "",
                             HardDiskPath, MAX_PATH, IniFile);

    // Verify HD0 image file exists and mount it.
    GetPrivateProfileString(gConfigurationSection,"VHDImage" ,"",VHDfile0,MAX_PATH,IniFile);
    CheckPath(VHDfile0);
    hr = CreateFile (VHDfile0,0,FILE_SHARE_READ,nullptr,
                     OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (hr==INVALID_HANDLE_VALUE) {
        strcpy(VHDfile0,"");
        WritePrivateProfileString(gConfigurationSection,"VHDImage","",IniFile);
    } else {
        CloseHandle(hr);
        MountHD(VHDfile0,0);
    }

    // Verify HD1 image file exists and mount it.
    GetPrivateProfileString(gConfigurationSection,"VHDImage1","",VHDfile1,MAX_PATH,IniFile);
    CheckPath(VHDfile1);
    hr = CreateFile (VHDfile1,0,FILE_SHARE_READ,nullptr,
                     OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (hr==INVALID_HANDLE_VALUE) {
        strcpy(VHDfile1,"");
        WritePrivateProfileString(gConfigurationSection,"VHDImage1","",IniFile);
    } else {
        CloseHandle(hr);
        MountHD(VHDfile1,1);
    }

	ClockEnabled = GetPrivateProfileInt(gConfigurationSection, "ClkEnable", 1, IniFile) != 0;
	ClockReadOnly = GetPrivateProfileInt(gConfigurationSection, "ClkRdOnly", 1, IniFile) != 0;

    // Create config menu
	BuildCartridgeMenu();
    return;
}

// Save config saves the hard disk path and vhd file names
void SaveConfig()
{
    ValidatePath(VHDfile0);
    ValidatePath(VHDfile1);
    if (strcmp(HardDiskPath, "") != 0) {
        WritePrivateProfileString
            ("DefaultPaths", "HardDiskPath", HardDiskPath, IniFile);
    }
    WritePrivateProfileString(gConfigurationSection,"VHDImage",VHDfile0 ,IniFile);
    WritePrivateProfileString(gConfigurationSection,"VHDImage1",VHDfile1 ,IniFile);
	WritePrivateProfileInt(gConfigurationSection, "ClkEnable", ClockEnabled, IniFile);
	WritePrivateProfileInt(gConfigurationSection, "ClkRdOnly", ClockReadOnly, IniFile);
    return;
}

// Generate menu for mounting the drives
void BuildCartridgeMenu()
{
	char TempMsg[512] = "";
	char TempBuf[MAX_PATH] = "";

	CartMenuCallback(gHostKey, "", MID_BEGIN, MIT_Head);
	CartMenuCallback(gHostKey, "", MID_ENTRY, MIT_Seperator);

	CartMenuCallback(gHostKey, "HD Drive 0", MID_ENTRY, MIT_Head);
	CartMenuCallback(gHostKey, "Insert", ControlId(10), MIT_Slave);
	strcpy(TempMsg, "Eject: ");
	strcpy(TempBuf, VHDfile0);
	PathStripPath(TempBuf);
	strcat(TempMsg, TempBuf);
	CartMenuCallback(gHostKey, TempMsg, ControlId(11), MIT_Slave);

	CartMenuCallback(gHostKey, "HD Drive 1", MID_ENTRY, MIT_Head);
	CartMenuCallback(gHostKey, "Insert", ControlId(12), MIT_Slave);
	strcpy(TempMsg, "Eject: ");
	strcpy(TempBuf, VHDfile1);
	PathStripPath(TempBuf);
	strcat(TempMsg, TempBuf);
	CartMenuCallback(gHostKey, TempMsg, ControlId(13), MIT_Slave);

	CartMenuCallback(gHostKey, "HD Config", ControlId(14), MIT_StandAlone);
	CartMenuCallback(gHostKey, "", MID_FINISH, MIT_Head);
}

// Dialog for creating a new hard disk
LRESULT CALLBACK NewDisk(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
    unsigned int hdsize=DEF_HD_SIZE;
    switch (message)
    {
        case WM_CLOSE:
            EndDialog(hDlg,LOWORD(wParam));  //Modal dialog
            return TRUE;

        case WM_INITDIALOG:
            SetDlgItemInt(hDlg,IDC_HDSIZE,DEF_HD_SIZE,0);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case IDOK:
                hdsize=GetDlgItemInt(hDlg,IDC_HDSIZE,nullptr,0);
                EndDialog(hDlg,CreateDisk(hDlg,hdsize));
                break;

            case IDCANCEL:
                EndDialog(hDlg,0);
                break;
            }
        break;
    }
    return FALSE;
}

// Create a new disk file, return 1 on success
int CreateDisk(HWND hDlg, int hdsize)
{
    HANDLE hr=CreateFile( NewVHDfile, GENERIC_READ | GENERIC_WRITE,
                          0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (hr==INVALID_HANDLE_VALUE) {
        *NewVHDfile='\0';
        MessageBox(hDlg,"Can't create File","Error",0);
        return 0;
    }

    if (hdsize>0) {
        SetFilePointer(hr, hdsize * 1024, nullptr, FILE_BEGIN);
        SetEndOfFile(hr);
    }

    CloseHandle(hr);
    return 1;
}
