//#define USE_LOGGING
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
//
//======================================================================

#include <Windows.h>
#include <stdio.h>
#include<iostream>
#include "resource.h"
#include "cc3vhd.h"
#include <vcc/devices/cloud9.h>
#include <vcc/util/FileOps.h>
#include <vcc/util/DialogOps.h>
#include "../CartridgeMenu.h"
#include <vcc/util/interrupts.h>
#include <vcc/bus/cpak_cartridge_definitions.h>
// Three includes added for PakGetMenuItem
#include <vcc/bus/cartridge_menu.h>
#include <vcc/bus/cartridge_messages.h>
#include <vcc/util/fileutil.h>
#include <vcc/util/limits.h>

constexpr auto DEF_HD_SIZE = 132480u;

static char VHDfile0[MAX_PATH] { 0 };
static char VHDfile1[MAX_PATH] { 0 };
static char *VHDfile; // Selected drive file name
static char NewVHDfile[MAX_PATH];
static char IniFile[MAX_PATH]  { 0 };
static char HardDiskPath[MAX_PATH];
static ::VCC::Device::rtc::cloud9 cloud9_rtc;

static slot_id_type gSlotId {};
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
void BuildCartridgeMenu(); //OLD
int CreateDisk(HWND,int);

static HINSTANCE gModuleInstance;
static HWND hConfDlg = nullptr;
LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM );

// Added for PakGetMenuItem export
bool get_menu_item(menu_item_entry* item, size_t index);

// Added for PakGetMenuItem export
static HWND gVccWnd = nullptr;

//=================================================================================

using namespace std;

void MemWrite(unsigned char Data, unsigned short Address)
{
	MemWrite8(gSlotId, Data, Address);
}

unsigned char MemRead(unsigned short Address)
{
	return MemRead8(gSlotId, Address);
}


extern "C"
{

	__declspec(dllexport) const char* PakGetName()
	{
		static char string_buffer[MAX_LOADSTRING];

		LoadString(gModuleInstance, IDS_MODULE_NAME, string_buffer, MAX_LOADSTRING);

		return string_buffer;
	}

	__declspec(dllexport) const char* PakGetCatalogId()
	{
		static char string_buffer[MAX_LOADSTRING];

		LoadString(gModuleInstance, IDS_CATNUMBER, string_buffer, MAX_LOADSTRING);

		return string_buffer;
	}

	__declspec(dllexport) const char* PakGetDescription()
	{
		static char string_buffer[MAX_LOADSTRING];

		LoadString(gModuleInstance, IDS_DESCRIPTION, string_buffer, MAX_LOADSTRING);

		return string_buffer;
	}

	__declspec(dllexport) void PakInitialize(
		slot_id_type SlotId,
		const char* const configuration_path,
        HWND hVccWnd,
		const cpak_callbacks* const callbacks)
	{
		gSlotId = SlotId;
		gVccWnd = hVccWnd;
		CartMenuCallback = callbacks->add_menu_item; //OLD
		MemRead8 = callbacks->read_memory_byte;
		MemWrite8 = callbacks->write_memory_byte;
		strcpy(IniFile, configuration_path);

		LoadConfig();
		cloud9_rtc.set_read_only(ClockReadOnly);
		VhdReset(); // Selects drive zero
		BuildCartridgeMenu();
		// Added for PakGetMenuItem export
		// Request menu rebuild
		SendMessage(gVccWnd,WM_COMMAND,(WPARAM) IDC_MSG_UPD_MENU,(LPARAM) 0);
	}

	__declspec(dllexport) bool PakGetMenuItem(menu_item_entry* item, size_t index)
	{
		return get_menu_item(item, index);
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
        // FIXME should only rebuild menus if drive is changed
		BuildCartridgeMenu(); // Old
		SendMessage(gVccWnd,WM_COMMAND,(WPARAM) IDC_MSG_UPD_MENU,(LPARAM) 0);
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
			cloud9_rtc.set_read_only(ClockReadOnly);
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
			return cloud9_rtc.read_port(Port);
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

BOOL WINAPI DllMain( HINSTANCE hinstDLL,  // handle to DLL module
                     DWORD fdwReason,     // reason for calling function
                     LPVOID lpReserved )  // reserved
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
		gModuleInstance = hinstDLL;
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        PakTerminate();
    }
    return TRUE;
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
    char ModName[MAX_LOADSTRING]="";
    HANDLE hr;

    GetPrivateProfileString("DefaultPaths", "HardDiskPath", "",
                             HardDiskPath, MAX_PATH, IniFile);

    // Determine module name for config lookups
    LoadString(gModuleInstance,IDS_MODULE_NAME, ModName, MAX_LOADSTRING);

    // Verify HD0 image file exists and mount it.
    GetPrivateProfileString(ModName,"VHDImage" ,"",VHDfile0,MAX_PATH,IniFile);
    CheckPath(VHDfile0);
    hr = CreateFile (VHDfile0,0,FILE_SHARE_READ,nullptr,
                     OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (hr==INVALID_HANDLE_VALUE) {
        strcpy(VHDfile0,"");
        WritePrivateProfileString(ModName,"VHDImage","",IniFile);
    } else {
        CloseHandle(hr);
        MountHD(VHDfile0,0);
    }

    // Verify HD1 image file exists and mount it.
    GetPrivateProfileString(ModName,"VHDImage1","",VHDfile1,MAX_PATH,IniFile);
    CheckPath(VHDfile1);
    hr = CreateFile (VHDfile1,0,FILE_SHARE_READ,nullptr,
                     OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (hr==INVALID_HANDLE_VALUE) {
        strcpy(VHDfile1,"");
        WritePrivateProfileString(ModName,"VHDImage1","",IniFile);
    } else {
        CloseHandle(hr);
        MountHD(VHDfile1,1);
    }

	ClockEnabled = GetPrivateProfileInt(ModName, "ClkEnable", 1, IniFile) != 0;
	ClockReadOnly = GetPrivateProfileInt(ModName, "ClkRdOnly", 1, IniFile) != 0;

    // Create config menu
	BuildCartridgeMenu(); //OLD
    // Added for PakGetMenuItem
	SendMessage(gVccWnd,WM_COMMAND,(WPARAM) IDC_MSG_UPD_MENU,(LPARAM) 0);
    return;
}

// Save config saves the hard disk path and vhd file names
void SaveConfig()
{
    char ModName[MAX_LOADSTRING]="";
    LoadString(gModuleInstance,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);

    ValidatePath(VHDfile0);
    ValidatePath(VHDfile1);
    if (strcmp(HardDiskPath, "") != 0) {
        WritePrivateProfileString
            ("DefaultPaths", "HardDiskPath", HardDiskPath, IniFile);
    }
    WritePrivateProfileString(ModName,"VHDImage",VHDfile0 ,IniFile);
    WritePrivateProfileString(ModName,"VHDImage1",VHDfile1 ,IniFile);
	WritePrivateProfileInt(ModName, "ClkEnable", ClockEnabled, IniFile);
	WritePrivateProfileInt(ModName, "ClkRdOnly", ClockReadOnly, IniFile);
    return;
}

// Added for PakGetMenuItem export
// Return items for cartridge menu. Called for each item
bool get_menu_item(menu_item_entry* item, size_t index)
{
	using VCC::Bus::gDllCartMenu;
	std::string disk;
	if (!item) return false;
	// request zero rebuilds the menu list
	if (index == 0) {
		gDllCartMenu.clear();
		gDllCartMenu.add("", 0, MIT_Seperator);
		gDllCartMenu.add("HD Drive 0",MID_ENTRY,MIT_Head);
		gDllCartMenu.add("Insert",ControlId(10),MIT_Slave);
		disk = VCC::Util::GetFileNamePart(VHDfile0);
		gDllCartMenu.add("Eject: "+disk,ControlId(11),MIT_Slave);
		gDllCartMenu.add("HD Drive 1",MID_ENTRY,MIT_Head);
		gDllCartMenu.add("Insert",ControlId(12),MIT_Slave);
		disk = VCC::Util::GetFileNamePart(VHDfile1);
		gDllCartMenu.add("Eject: "+disk,ControlId(13),MIT_Slave);
		gDllCartMenu.add("HD Config",ControlId(14),MIT_StandAlone);
	}
	// return requested list item or false
	return gDllCartMenu.copy_item(*item, index);
}

// OLD Generate menu for mounting the drives OLD
void BuildCartridgeMenu()
{
	char TempMsg[512] = "";
	char TempBuf[MAX_PATH] = "";

	CartMenuCallback(gSlotId, "", MID_BEGIN, MIT_Head);
	CartMenuCallback(gSlotId, "", MID_ENTRY, MIT_Seperator);

	CartMenuCallback(gSlotId, "HD Drive 0", MID_ENTRY, MIT_Head);
	CartMenuCallback(gSlotId, "Insert", ControlId(10), MIT_Slave);
	strcpy(TempMsg, "Eject: ");
	strcpy(TempBuf, VHDfile0);
	PathStripPath(TempBuf);
	strcat(TempMsg, TempBuf);
	CartMenuCallback(gSlotId, TempMsg, ControlId(11), MIT_Slave);

	CartMenuCallback(gSlotId, "HD Drive 1", MID_ENTRY, MIT_Head);
	CartMenuCallback(gSlotId, "Insert", ControlId(12), MIT_Slave);
	strcpy(TempMsg, "Eject: ");
	strcpy(TempBuf, VHDfile1);
	PathStripPath(TempBuf);
	strcat(TempMsg, TempBuf);
	CartMenuCallback(gSlotId, TempMsg, ControlId(13), MIT_Slave);

	CartMenuCallback(gSlotId, "HD Config", ControlId(14), MIT_StandAlone);
	CartMenuCallback(gSlotId, "", MID_FINISH, MIT_Head);
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
