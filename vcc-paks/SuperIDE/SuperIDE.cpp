/****************************************************************************/
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
/****************************************************************************/
/*
*/
/****************************************************************************/

#include "Superide.h"
#include "idebus.h"
#include "cloud9.h"
#include "logger.h"

#include "../../vcc/defines.h"
#include "../../vcc/fileops.h"
#include "../../vcc/vccPakAPI.h"

#include <windows.h>
#include <stdio.h>
#include "resource.h" 

/****************************************************************************/
/*
	Forward declarations
*/

LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);
void Select_Disk(unsigned char);
void SaveConfig();
void LoadConfig();

/****************************************************************************/
/*
	Global variables
*/

static char FileName[MAX_PATH] { 0 };
static char IniFile[MAX_PATH]  { 0 };
static char LastPath[MAX_PATH] { 0 };

static vccpakapi_assertinterrupt_t	AssertInt			= NULL;
static vcccpu_read8_t				MemRead8			= NULL;
static vcccpu_write8_t				MemWrite8			= NULL;
static vccapi_dynamicmenucallback_t	DynamicMenuCallback	= NULL;

static unsigned char *	Memory		= NULL;
static unsigned char	BaseAddress	= 0x50;

unsigned char BaseTable[4]={0x40,0x50,0x60,0x70};
static unsigned char BaseAddr=1,ClockEnabled=1,ClockReadOnly=1;
static unsigned char DataLatch=0;

static HINSTANCE g_hinstDLL;
static HWND g_hWnd = NULL;
static int g_id = 0;

/****************************************************************************/
/**
*/
void vccPakRebuildMenu()
{
	DynamicMenuCallback(g_id, "", VCC_DYNMENU_FLUSH, DMENU_TYPE_NONE);

	DynamicMenuCallback(g_id, "", VCC_DYNMENU_REFRESH, DMENU_TYPE_NONE);
}

/****************************************************************************/

void Select_Disk(unsigned char Disk)
{
	OPENFILENAME ofn;
	char TempFileName[MAX_PATH] = "";

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = g_hWnd;
	ofn.Flags = OFN_HIDEREADONLY;
	ofn.hInstance = GetModuleHandle(0);
	ofn.lpstrDefExt = "IMG";
	ofn.lpstrFilter = "Hard Disk Images\0*.img;*.vhd;*.os9\0All files\0*.*\0\0";	// filter string "Disks\0*.DSK\0\0";
	ofn.nFilterIndex = 0;								// current filter index
	ofn.lpstrFile = TempFileName;					// contains full path and filename on return
	ofn.nMaxFile = MAX_PATH;						// sizeof lpstrFile
	ofn.lpstrFileTitle = NULL;							// filename and extension only
	ofn.nMaxFileTitle = MAX_PATH;						// sizeof lpstrFileTitle
	ofn.lpstrTitle = "Mount IDE hard Disk Image";	// title bar string
	ofn.lpstrInitialDir = NULL;							// initial directory
	if (strlen(LastPath) > 0)
	{
		ofn.lpstrInitialDir = LastPath;
	}

	if (GetOpenFileName(&ofn))
	{
		// save last path
		strcpy(LastPath, TempFileName);
		PathRemoveFileSpec(LastPath);

		if (!MountDisk(TempFileName, Disk))
		{
			MessageBox(0, "Can't Open File", "Can't open the Image specified.", 0);
		}
	}
}

/****************************************************************************/

void SaveConfig(void)
{
	unsigned char Index = 0;
	char ModName[MAX_LOADSTRING] = "";
	LoadString(g_hinstDLL, IDS_MODULE_NAME, ModName, MAX_LOADSTRING);
	QueryDisk(MASTER, FileName);
	WritePrivateProfileString(ModName, "Master", FileName, IniFile);
	QueryDisk(SLAVE, FileName);
	WritePrivateProfileString(ModName, "Slave", FileName, IniFile);
	WritePrivateProfileInt(ModName, "BaseAddr", BaseAddr, IniFile);
	WritePrivateProfileInt(ModName, "ClkEnable", ClockEnabled, IniFile);
	WritePrivateProfileInt(ModName, "ClkRdOnly", ClockReadOnly, IniFile);

	WritePrivateProfileString(ModName, "LastPath", LastPath, IniFile);
}

/****************************************************************************/

void LoadConfig(void)
{
	char ModName[MAX_LOADSTRING] = "";
	unsigned char Index = 0;
	char Temp[16] = "";
	char DiskName[MAX_PATH] = "";
	unsigned int RetVal = 0;
	HANDLE hr = NULL;
	LoadString(g_hinstDLL, IDS_MODULE_NAME, ModName, MAX_LOADSTRING);

	GetPrivateProfileString(ModName, "Master", "", FileName, MAX_PATH, IniFile);
	MountDisk(FileName, MASTER);
	GetPrivateProfileString(ModName, "Slave", "", FileName, MAX_PATH, IniFile);
	BaseAddr = GetPrivateProfileInt(ModName, "BaseAddr", 1, IniFile);
	ClockEnabled = GetPrivateProfileInt(ModName, "ClkEnable", 1, IniFile);
	ClockReadOnly = GetPrivateProfileInt(ModName, "ClkRdOnly", 1, IniFile);

	GetPrivateProfileString(ModName, "LastPath", "", LastPath, MAX_PATH, IniFile);

	BaseAddr &= 3;
	if (BaseAddr == 3)
	{
		ClockEnabled = 0;
	}
	BaseAddress = BaseTable[BaseAddr];
	SetClockWrite(!ClockReadOnly);
	MountDisk(FileName, SLAVE);

	vccPakRebuildMenu();
}

/****************************************************************************/
/****************************************************************************/
/*
*/

/****************************************************************************/
/**
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_INIT(int id, void * wndHandle, vccapi_dynamicmenucallback_t Temp)
{
	g_id = id;
	g_hWnd = (HWND)wndHandle;

	DynamicMenuCallback = Temp;

	IdeInit();
}

/**
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_GETNAME(char * ModName, char * CatNumber)
{
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	LoadString(g_hinstDLL,IDS_CATNUMBER,CatNumber, MAX_LOADSTRING);
}

/**
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_PORTWRITE(unsigned char Port,unsigned char Data)
{
	if ((Port >= BaseAddress) & (Port <= (BaseAddress + 8)))
	{
		switch (Port - BaseAddress)
		{
		case 0x0:
			IdeRegWrite(Port - BaseAddress, (DataLatch << 8) + Data);
			break;

		case 0x8:
			DataLatch = Data & 0xFF;		//Latch
			break;

		default:
			IdeRegWrite(Port - BaseAddress, Data);
			break;
		}	//End port switch
	}
}

/**
*/
extern "C" __declspec(dllexport) unsigned char VCC_PAKAPI_DEF_PORTREAD(unsigned char Port)
{
	unsigned char RetVal=0;
	unsigned short Temp=0;

	if (((Port == 0x78) | (Port == 0x79) | (Port == 0x7C)) & ClockEnabled)
	{
		RetVal = ReadTime(Port);
	}

	if ((Port >= BaseAddress) & (Port <= (BaseAddress + 8)))
	{
		switch (Port - BaseAddress)
		{
		case 0x0:
			Temp = IdeRegRead(Port - BaseAddress);

			RetVal = Temp & 0xFF;
			DataLatch = Temp >> 8;
			break;

		case 0x8:
			RetVal = DataLatch;		//Latch
			break;
		default:
			RetVal = IdeRegRead(Port - BaseAddress) & 0xFF;
			break;

		}	//End port switch	
	}

	return(RetVal);
}

/**
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_STATUS(char * buffer, size_t bufferSize)
{
	// TODO: use buffer size
	DiskStatus(buffer);
}

/**
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_CONFIG(int MenuID)
{
	switch (MenuID)
	{
	case 10:
		Select_Disk(MASTER);
		vccPakRebuildMenu();
		SaveConfig();
		break;

	case 11:
		DropDisk(MASTER);
		vccPakRebuildMenu();
		SaveConfig();
		break;

	case 12:
		Select_Disk(SLAVE);
		vccPakRebuildMenu();
		SaveConfig();
		break;

	case 13:
		DropDisk(SLAVE);
		vccPakRebuildMenu();
		SaveConfig();
		break;

	case 14:
		DialogBox(g_hinstDLL, (LPCTSTR)IDD_CONFIG, NULL, (DLGPROC)Config);
		break;
	}
	return;
}

/**
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_SETINIPATH(char *IniFilePath)
{
	strcpy(IniFile,IniFilePath);
	LoadConfig();
}

/**
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_DYNMENUBUILD(void)
{
	char TempMsg[512]="";
	char TempBuf[MAX_PATH]="";

	DynamicMenuCallback(g_id, "",6000, DMENU_TYPE_SEPARATOR);

	DynamicMenuCallback(g_id, "IDE Master",6000,DMENU_TYPE_HEAD);
	DynamicMenuCallback(g_id, "Insert",5010,DMENU_TYPE_SLAVE);
	QueryDisk(MASTER,TempBuf);
	strcpy(TempMsg,"Eject: ");
	PathStripPath (TempBuf);
	strcat(TempMsg,TempBuf);

	DynamicMenuCallback(g_id, TempMsg,5011, DMENU_TYPE_SLAVE);
	DynamicMenuCallback(g_id, "IDE Slave",6000, DMENU_TYPE_HEAD);
	DynamicMenuCallback(g_id, "Insert",5012, DMENU_TYPE_SLAVE);
	QueryDisk(SLAVE,TempBuf);
	strcpy(TempMsg,"Eject: ");
	PathStripPath (TempBuf);
	strcat(TempMsg,TempBuf);
	DynamicMenuCallback(g_id, TempMsg,5013, DMENU_TYPE_SLAVE);
	DynamicMenuCallback(g_id, "IDE Config",5014, DMENU_TYPE_STANDALONE);
	DynamicMenuCallback(g_id, "",1, DMENU_TYPE_NONE);
}

/****************************************************************************/
/*
	Debug only simple check to verify API matches the definitions
	If the Pak API changes for one of our defined functions it
	should produce a compile error
*/

#ifdef _DEBUG

static vccpakapi_init_t				__init				= VCC_PAKAPI_DEF_INIT;
static vccpakapi_getname_t			__getName			= VCC_PAKAPI_DEF_GETNAME;
static vccpakapi_dynmenubuild_t		__dynMenuBuild		= VCC_PAKAPI_DEF_DYNMENUBUILD;
static vccpakapi_config_t			__config			= VCC_PAKAPI_DEF_CONFIG;
//static vccpakapi_heartbeat_t		__heartbeat			= VCC_PAKAPI_DEF_HEARTBEAT;
static vccpakapi_status_t			__status			= VCC_PAKAPI_DEF_STATUS;
//static vccpakapi_getaudiosample_t	__getAudioSample	= VCC_PAKAPI_DEF_AUDIOSAMPLE;
//static vccpakapi_reset_t			__reset				= VCC_PAKAPI_DEF_RESET;
static vccpakapi_portread_t			__portRead			= VCC_PAKAPI_DEF_PORTREAD;
static vccpakapi_portwrite_t		__portWrite			= VCC_PAKAPI_DEF_PORTWRITE;
//static vcccpu_read8_t				__memRead			= VCC_PAKAPI_DEF_MEMREAD;
//static vcccpu_write8_t			__memWrite			= VCC_PAKAPI_DEF_MEMWRITE;
//static vccpakapi_setmemptrs_t		__memPointers		= VCC_PAKAPI_DEF_MEMPOINTERS;
//static vccpakapi_setcartptr_t		__setCartPtr		= VCC_PAKAPI_DEF_SETCART;
//static vccpakapi_setintptr_t		__assertInterrupt	= VCC_PAKAPI_DEF_ASSERTINTERRUPT;
static vccpakapi_setinipath_t		__setINIPath		= VCC_PAKAPI_DEF_SETINIPATH;

#endif // _DEBUG

/****************************************************************************/

LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned char BTemp=0;
	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnabled,0);
			SendDlgItemMessage(hDlg,IDC_READONLY,BM_SETCHECK,ClockReadOnly,0);
			SendDlgItemMessage (hDlg,IDC_BASEADDR, CB_ADDSTRING, NULL,(LPARAM) "40");
			SendDlgItemMessage (hDlg,IDC_BASEADDR, CB_ADDSTRING, NULL,(LPARAM) "50");
			SendDlgItemMessage (hDlg,IDC_BASEADDR, CB_ADDSTRING, NULL,(LPARAM) "60");
			SendDlgItemMessage (hDlg,IDC_BASEADDR, CB_ADDSTRING, NULL,(LPARAM) "70");
			SendDlgItemMessage (hDlg,IDC_BASEADDR, CB_SETCURSEL,BaseAddr,(LPARAM)0);
			EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), TRUE);
			if (BaseAddr==3)
			{
				ClockEnabled=0;
				SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnabled,0);
				EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), FALSE);
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					BaseAddr=(unsigned char)SendDlgItemMessage(hDlg,IDC_BASEADDR,CB_GETCURSEL,0,0);
					ClockEnabled=(unsigned char)SendDlgItemMessage(hDlg,IDC_CLOCK,BM_GETCHECK,0,0);
					ClockReadOnly=(unsigned char)SendDlgItemMessage(hDlg,IDC_READONLY,BM_GETCHECK,0,0);
					BaseAddress=BaseTable[BaseAddr & 3];
					SetClockWrite(!ClockReadOnly);
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
						ClockEnabled=0;
						SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnabled,0);
						EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), FALSE);
					}
					break;
			}
	}
	return(0);
}

/****************************************************************************/
/*
	DLL Main entry point (Windows)
*/
BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpReserved)  // reserved
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			// one-time init
			g_hinstDLL = hinstDLL;
		}
		break;

		case DLL_PROCESS_DETACH:
		{
			// one time destruction
		}
		break;
	}

	return(1);
}

/****************************************************************************/
