//---------------------------------------------------------------------------------
// Copyright 2015 by Joseph Forgione
// This file is part of VCC (Virtual Color Computer).
//
// VCC (Virtual Color Computer) is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or any later
// version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.  You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//
//---------------------------------------------------------------------------------

#include <Windows.h>
#include <stdio.h>
#include "becker.h"
#include "resource.h"
#include "../CartridgeMenu.h"
#include <vcc/utils/winapi.h>
#include <vcc/core/cartridge_capi.h>
#include <vcc/devices/becker/beckerport.h>
#include <vcc/core/limits.h>
#include <vcc/common/logger.h>

// vcc stuff
static HINSTANCE gModuleInstance;
static void* gHostKey = nullptr;
static PakAppendCartridgeMenuHostCallback CartMenuCallback = nullptr;
static PakAssertCartridgeLineHostCallback PakSetCart = nullptr;
LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);
static char IniFile[MAX_PATH]="";
static unsigned char HDBRom[8192];
static bool DWTCPEnabled = false;

static HWND g_hConfigDlg;

static char dwaddress[MAX_LOADSTRING];
static char dwsport[MAX_LOADSTRING];

void BuildCartridgeMenu();
void LoadConfig();
void SaveConfig();

static ::vcc::devices::beckerport::Becker gBecker;

BOOL APIENTRY DllMain( HINSTANCE  hinstDLL,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			gModuleInstance = hinstDLL;
			break;
		case DLL_PROCESS_DETACH:
			break;
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
		PakSetCart = context->assert_cartridge_line;
		strcpy(IniFile, configuration_path);
		LoadConfig();
		gBecker.enable(1);
		BuildCartridgeMenu();
	}
}

extern "C" __declspec(dllexport) void PakWritePort(unsigned char Port,unsigned char Data)
	{
		switch (Port)
		{
			// write data
			case 0x42:
				gBecker.write(Data,Port);
				//dw_write(Data);
				break;
		}
		return;
	}

extern "C" __declspec(dllexport) unsigned char PakReadPort(unsigned char Port)
	{
		switch (Port)
		{
			// read status
			case 0x41:
				if (gBecker.read(Port) != 0)
					return 2;
				else
					return 0;
				break;
			// read data
			case 0x42:
				return(gBecker.read(Port));
				break;
		}
		return 0;
	}

extern "C" __declspec(dllexport) void PakGetStatus(char* text_buffer, size_t buffer_size)
	{
		gBecker.status(text_buffer);  // text buffer size??
		return;
	}


void BuildCartridgeMenu()
{
	CartMenuCallback(gHostKey, "", MID_BEGIN, MIT_Head);
	CartMenuCallback(gHostKey, "", MID_ENTRY, MIT_Seperator);
	CartMenuCallback(gHostKey, "DriveWire Server..", ControlId(16), MIT_StandAlone);
	CartMenuCallback(gHostKey, "", MID_FINISH, MIT_Head);
}

extern "C" __declspec(dllexport) void PakMenuItemClicked(unsigned char MenuID)
	{
		HWND h_own = GetActiveWindow();
		CreateDialog(gModuleInstance,(LPCTSTR)IDD_PROPPAGE,h_own,(DLGPROC)Config);
		ShowWindow(g_hConfigDlg,1);
		BuildCartridgeMenu();
		return;
	}


LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hwndOwner;
	RECT rc, rcDlg, rcOwner;

	switch (message)
	{
		case WM_INITDIALOG:

			if ((hwndOwner = GetParent(hDlg)) == NULL)
			{
				hwndOwner = GetDesktopWindow();
			}

			g_hConfigDlg=hDlg;

			GetWindowRect(hwndOwner, &rcOwner);
			GetWindowRect(hDlg, &rcDlg);
			CopyRect(&rc, &rcOwner);

			OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
			OffsetRect(&rc, -rc.left, -rc.top);
			OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

		    SetWindowPos(hDlg,
                 HWND_TOP,
                 rcOwner.left + (rc.right / 2),
                 rcOwner.top + (rc.bottom / 2),
                 0, 0,          // Ignores size arguments.
                 SWP_NOSIZE);

			SendDlgItemMessage (hDlg,IDC_TCPHOST, WM_SETTEXT, 0,(LPARAM)(LPCSTR)dwaddress);
			SendDlgItemMessage (hDlg,IDC_TCPPORT, WM_SETTEXT, 0,(LPARAM)(LPCSTR)dwsport);
			return TRUE;
		break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					SendDlgItemMessage (hDlg,IDC_TCPHOST, WM_GETTEXT, MAX_PATH,(LPARAM)(LPCSTR)dwaddress);
					SendDlgItemMessage (hDlg,IDC_TCPPORT, WM_GETTEXT, MAX_PATH,(LPARAM)(LPCSTR)dwsport);
					gBecker.sethost(dwaddress,dwsport);
					SaveConfig();

					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;
				break;

				case IDHELP:
					return TRUE;
				break;

				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
				break;
			}
			return TRUE;
		break;

	}
    return FALSE;
}

void LoadConfig()
{
	char ModName[MAX_LOADSTRING]="";

	LoadString(gModuleInstance,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	GetPrivateProfileString(ModName,"DWServerAddr","127.0.0.1",dwaddress, MAX_LOADSTRING,IniFile);
	GetPrivateProfileString(ModName,"DWServerPort","65504",dwsport, MAX_LOADSTRING,IniFile);
	gBecker.sethost(dwaddress,dwsport);
	BuildCartridgeMenu();
	return;
}

void SaveConfig()
{
	char ModName[MAX_LOADSTRING]="";
	LoadString(gModuleInstance,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	WritePrivateProfileString(ModName,"DWServerAddr",dwaddress,IniFile);
	WritePrivateProfileString(ModName,"DWServerPort",dwsport,IniFile);
	return;
}
