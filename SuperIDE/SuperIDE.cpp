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
#include "stdio.h"
#include <iostream>
#include "resource.h" 
#include "IdeBus.h"
#include <vcc/devices/cloud9.h>
#include <vcc/util/FileOps.h>
#include "../CartridgeMenu.h"
#include <vcc/util/DialogOps.h>
#include <vcc/bus/cpak_cartridge_definitions.h>
#include <vcc/util/limits.h>
#include <vcc/util/logger.h>
// Three includes added for PakGetMenuItem
#include <vcc/bus/cartridge_menu.h>
#include <vcc/bus/cartridge_messages.h>
#include <vcc/util/fileutil.h>

static char FileName[MAX_PATH] { 0 };
static char IniFile[MAX_PATH]  { 0 };
static char SuperIDEPath[MAX_PATH];
static ::VCC::Device::rtc::cloud9 cloud9_rtc;
static PakAppendCartridgeMenuHostCallback CartMenuCallback = nullptr;
static unsigned char BaseAddress=0x50;
void BuildCartridgeMenu();
LRESULT CALLBACK IDE_Config(HWND, UINT, WPARAM, LPARAM );
void Select_Disk(unsigned char);
void SaveConfig();
void LoadConfig();
unsigned char BaseTable[4]={0x40,0x50,0x60,0x70};
bool get_menu_item(menu_item_entry*, size_t index);

static unsigned char BaseAddr=1;
static bool ClockEnabled = true;
static bool ClockReadOnly = true;
static unsigned char DataLatch=0;
static HINSTANCE gModuleInstance;
static HWND hConfDlg = nullptr;
static slot_id_type gSlotId {};

static HWND gVccWnd = nullptr;

using namespace std;


//----------------------------------------------------------------------------
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
		CartMenuCallback = callbacks->add_menu_item;
		strcpy(IniFile, configuration_path);

		LoadConfig();
		IdeInit();

		BuildCartridgeMenu(); // REMOVE THIS 
		//SendMessage(gVccWnd,WM_COMMAND,(WPARAM) IDC_MSG_UPD_MENU,(LPARAM) 0);
	}

	__declspec(dllexport) void PakTerminate()
	{
		CloseCartDialog(hConfDlg);
		hConfDlg = nullptr;
	}

	__declspec(dllexport) void PakWritePort(unsigned char Port,unsigned char Data)
	{
		if ( (Port >=BaseAddress) & (Port <= (BaseAddress+8)))
			switch (Port-BaseAddress)
			{
				case 0x0:	
					IdeRegWrite(Port-BaseAddress,(DataLatch<<8)+ Data );
					break;

				case 0x8:
					DataLatch=Data & 0xFF;		//Latch
					break;

				default:
					IdeRegWrite(Port-BaseAddress,Data);
					break;
			}	//End port switch	
		return;
	}

	__declspec(dllexport) unsigned char PakReadPort(unsigned char Port)
	{
		unsigned char RetVal=0;
		unsigned short Temp=0;
		if (((Port == 0x78) | (Port == 0x79) | (Port == 0x7C)) & ClockEnabled)
			RetVal = cloud9_rtc.read_port(Port);

		if ( (Port >=BaseAddress) & (Port <= (BaseAddress+8)))
			switch (Port-BaseAddress)
			{
				case 0x0:	
					Temp=IdeRegRead(Port-BaseAddress);

					RetVal=Temp & 0xFF;
					DataLatch= Temp>>8;
					break;

				case 0x8:
					RetVal=DataLatch;		//Latch
					break;
				default:
					RetVal=IdeRegRead(Port-BaseAddress)& 0xFF;
					break;

			}	//End port switch	
			
		return RetVal;
	}
          
	__declspec(dllexport) void PakGetStatus(char* text_buffer, size_t buffer_size)
	{
		DiskStatus(text_buffer, buffer_size);
	}
 

	//PakGetMenuItem export
	__declspec(dllexport) bool PakGetMenuItem(menu_item_entry* item, size_t index)
	{
		return get_menu_item(item, index);
	}

	__declspec(dllexport) void PakMenuItemClicked(unsigned char MenuID)
	{
		switch (MenuID)
		{
		case 10:
			Select_Disk(MASTER);
			BuildCartridgeMenu();
			SaveConfig();
		    SendMessage(gVccWnd,WM_COMMAND,(WPARAM) IDC_MSG_UPD_MENU,(LPARAM) 0);
			break;

		case 11:
			DropDisk(MASTER);
			BuildCartridgeMenu();
			SaveConfig();
		    SendMessage(gVccWnd,WM_COMMAND,(WPARAM) IDC_MSG_UPD_MENU,(LPARAM) 0);
			break;

		case 12:
			Select_Disk(SLAVE);
			BuildCartridgeMenu();
			SaveConfig();
		    SendMessage(gVccWnd,WM_COMMAND,(WPARAM) IDC_MSG_UPD_MENU,(LPARAM) 0);
			break;

		case 13:
			DropDisk(SLAVE);
			BuildCartridgeMenu();
			SaveConfig();
		    SendMessage(gVccWnd,WM_COMMAND,(WPARAM) IDC_MSG_UPD_MENU,(LPARAM) 0);
			break;

		case 14:
			CreateDialog( gModuleInstance, (LPCTSTR) IDD_CONFIG,
					GetActiveWindow(), (DLGPROC) IDE_Config );
			ShowWindow(hConfDlg,1);
			break;
		}
		return;
	}
}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD reason,
    LPVOID /*lpReserved*/ )
{
	switch (reason) {
	case DLL_PROCESS_ATTACH:
		gModuleInstance = hinstDLL;
		break;
	case DLL_PROCESS_DETACH:
		PakTerminate();
		break;
	}
	return TRUE;
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
		gDllCartMenu.add("IDE Master",MID_ENTRY,MIT_Head);
		gDllCartMenu.add("Insert",ControlId(10),MIT_Slave);
		disk = VCC::Util::GetFileNamePart(QueryDisk(MASTER));
		gDllCartMenu.add("Eject: "+disk,ControlId(11),MIT_Slave);
		gDllCartMenu.add("IDE Slave",MID_ENTRY,MIT_Head);
		gDllCartMenu.add("Insert",ControlId(12),MIT_Slave);
		disk = VCC::Util::GetFileNamePart(QueryDisk(SLAVE));
		gDllCartMenu.add("Eject: "+disk,ControlId(13),MIT_Slave);
		gDllCartMenu.add("IDE Config",ControlId(14),MIT_StandAlone);
	}
	// return requested list item or false
	return gDllCartMenu.copy_item(*item, index);
}

void BuildCartridgeMenu()
{
	char TempMsg[512]="";
	char TempBuf[MAX_PATH]="";
	CartMenuCallback(gSlotId, "", MID_BEGIN, MIT_Head);
	CartMenuCallback(gSlotId, "", MID_ENTRY, MIT_Seperator);
	CartMenuCallback(gSlotId, "IDE Master",MID_ENTRY,MIT_Head);
	CartMenuCallback(gSlotId, "Insert",ControlId(10),MIT_Slave);
	QueryDisk(MASTER,TempBuf);
	strcpy(TempMsg,"Eject: ");
	PathStripPath (TempBuf);
	strcat(TempMsg,TempBuf);
	CartMenuCallback(gSlotId, TempMsg,ControlId(11),MIT_Slave);
	CartMenuCallback(gSlotId, "IDE Slave",MID_ENTRY,MIT_Head);
	CartMenuCallback(gSlotId, "Insert",ControlId(12),MIT_Slave);
	QueryDisk(SLAVE,TempBuf);
	strcpy(TempMsg,"Eject: ");
	PathStripPath (TempBuf);
	strcat(TempMsg,TempBuf);
	CartMenuCallback(gSlotId, TempMsg,ControlId(13),MIT_Slave);
	CartMenuCallback(gSlotId, "IDE Config",ControlId(14),MIT_StandAlone);
	CartMenuCallback(gSlotId, "", MID_FINISH, MIT_Head);
}

LRESULT CALLBACK IDE_Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	unsigned char BTemp=0;
	switch (message)
	{
    	case WM_CLOSE:
        	DestroyWindow(hDlg);
        	hConfDlg=nullptr;
        	return TRUE;
        	break;

		case WM_INITDIALOG:
			hConfDlg=hDlg;
			SendDlgItemMessage(hDlg, IDC_CLOCK, BM_SETCHECK, ClockEnabled, 0);
			SendDlgItemMessage(hDlg, IDC_READONLY, BM_SETCHECK, ClockReadOnly, 0);
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_ADDSTRING, 0, (LPARAM) "40");
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_ADDSTRING, 0, (LPARAM) "50");
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_ADDSTRING, 0, (LPARAM) "60");
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_ADDSTRING, 0, (LPARAM) "70");
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_SETCURSEL,BaseAddr,(LPARAM)0);
			EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), TRUE);
			if (BaseAddr==3)
			{
				ClockEnabled = false;
				SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnabled,0);
				EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), FALSE);
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					BaseAddr=(unsigned char)SendDlgItemMessage(hDlg,IDC_BASEADDR,CB_GETCURSEL,0,0);
					ClockEnabled = (unsigned char)SendDlgItemMessage(hDlg, IDC_CLOCK, BM_GETCHECK, 0, 0) != 0;
					ClockReadOnly = (unsigned char)SendDlgItemMessage(hDlg, IDC_READONLY, BM_GETCHECK, 0, 0) != 0;
					BaseAddress=BaseTable[BaseAddr & 3];
					cloud9_rtc.set_read_only(ClockReadOnly);
					SaveConfig();
					EndDialog(hDlg, LOWORD(wParam));

					break;

				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					break;

				case IDC_CLOCK:

					break;

				case IDC_READONLY:
					break;

				case IDC_BASEADDR:
					BTemp=(unsigned char)SendDlgItemMessage(hDlg,IDC_BASEADDR,CB_GETCURSEL,0,0);
					EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), TRUE);
					if (BTemp==3)
					{
						ClockEnabled = false;
						SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnabled,0);
						EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), FALSE);
					}
					break;
			}
	}
	return 0;
}

void Select_Disk(unsigned char Disk)
{
	FileDialog dlg;
	dlg.setDefExt("IMG");
	dlg.setTitle("Mount IDE hard Disk Image");
	dlg.setFilter("Hard Disk Images\0*.img;*.vhd;*.os9\0All files\0*.*\0\0");
	dlg.setInitialDir(SuperIDEPath);
	if (dlg.show()) {
		if (MountDisk(dlg.path(),Disk)) {
			dlg.getdir(SuperIDEPath);
		} else {
			MessageBox(GetActiveWindow(),"Can't Open Image","Error",0);
		}
	}
	return;
}

void SaveConfig()
{
	char ModName[MAX_LOADSTRING]="";
	LoadString(gModuleInstance,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	QueryDisk(MASTER,FileName);
	WritePrivateProfileString(ModName,"Master",FileName,IniFile);
	QueryDisk(SLAVE,FileName);
	WritePrivateProfileString(ModName,"Slave",FileName,IniFile);
	WritePrivateProfileInt(ModName,"BaseAddr",BaseAddr ,IniFile);
	WritePrivateProfileInt(ModName,"ClkEnable",ClockEnabled ,IniFile);
	WritePrivateProfileInt(ModName, "ClkRdOnly", ClockReadOnly, IniFile);
	if (strcmp(SuperIDEPath, "") != 0) { 
		WritePrivateProfileString("DefaultPaths", "SuperIDEPath", SuperIDEPath, IniFile); 
	}

	return;
}

void LoadConfig()
{
	char ModName[MAX_LOADSTRING]="";

	LoadString(gModuleInstance,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	GetPrivateProfileString("DefaultPaths", "SuperIDEPath", "", SuperIDEPath, MAX_PATH, IniFile);
	GetPrivateProfileString(ModName,"Master","",FileName,MAX_PATH,IniFile);
	MountDisk(FileName ,MASTER);
	GetPrivateProfileString(ModName,"Slave","",FileName,MAX_PATH,IniFile);
	BaseAddr=GetPrivateProfileInt(ModName,"BaseAddr",1,IniFile); 
	ClockEnabled = GetPrivateProfileInt(ModName, "ClkEnable", true, IniFile) != 0;
	ClockReadOnly = GetPrivateProfileInt(ModName, "ClkRdOnly", true, IniFile) != 0;
	BaseAddr&=3;
	if (BaseAddr == 3)
		ClockEnabled = false;
	BaseAddress=BaseTable[BaseAddr];
	cloud9_rtc.set_read_only(ClockReadOnly);
	MountDisk(FileName ,SLAVE);
	BuildCartridgeMenu();
	SendMessage(gVccWnd,WM_COMMAND,(WPARAM) IDC_MSG_UPD_MENU,(LPARAM) 0);
	return;
}
