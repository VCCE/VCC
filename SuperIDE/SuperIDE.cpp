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

#include <Windows.h>
#include "stdio.h"
#include <iostream>
#include "resource.h" 
#include "defines.h"
#include "SuperIDE.h"
#include "IdeBus.h"
#include "cloud9.h"
#include "logger.h"
#include "../fileops.h"
#include "../DialogOps.h"

static char FileName[MAX_PATH] { 0 };
static char IniFile[MAX_PATH]  { 0 };
static char SuperIDEPath[MAX_PATH];
// FIXME: These typedefs are duplicated across more if not all projects and
// need to be consolidated in one place.
typedef unsigned char (*MEMREAD8)(unsigned short);
typedef void (*MEMWRITE8)(unsigned char,unsigned short);
typedef void (*DMAMEMPOINTERS) ( MEMREAD8,MEMWRITE8);
typedef void (*DYNAMICMENUCALLBACK)( const char *,int, int);
static DYNAMICMENUCALLBACK DynamicMenuCallback = nullptr;
static unsigned char BaseAddress=0x50;
void BuildDynaMenu(void);
LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM );
void Select_Disk(unsigned char);
void SaveConfig();
void LoadConfig();
unsigned char BaseTable[4]={0x40,0x50,0x60,0x70};

static unsigned char BaseAddr=1,ClockEnabled=1,ClockReadOnly=1;
static unsigned char DataLatch=0;
static HINSTANCE g_hinstDLL;
static HWND hConfDlg = nullptr;

using namespace std;

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
	if (fdwReason == DLL_PROCESS_DETACH ) //Clean Up 
	{
		if (hConfDlg) DestroyWindow(hConfDlg);
	}
	g_hinstDLL=hinstDLL;
	return 1;
}

extern "C" 
{          
	__declspec(dllexport) void ModuleName(char *ModName,char *CatNumber,DYNAMICMENUCALLBACK Temp)
	{
		LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
		LoadString(g_hinstDLL,IDS_CATNUMBER,CatNumber, MAX_LOADSTRING);
		DynamicMenuCallback =Temp;
		IdeInit();
		if (DynamicMenuCallback  != nullptr)
			BuildDynaMenu();		
		return ;
	}
}

extern "C" 
{         
	__declspec(dllexport) void PackPortWrite(unsigned char Port,unsigned char Data)
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
}

extern "C"
{
	__declspec(dllexport) unsigned char PackPortRead(unsigned char Port)
	{
		unsigned char RetVal=0;
		unsigned short Temp=0;
		if ( ((Port==0x78) | (Port==0x79) | (Port==0x7C)) & ClockEnabled)
			RetVal=ReadTime(Port);

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
}

extern "C" 
{          
	__declspec(dllexport) void ModuleStatus(char *MyStatus)
	{
		DiskStatus(MyStatus);
		return ;
	}
}

extern "C" 
{          
	__declspec(dllexport) void ModuleConfig(unsigned char MenuID)
	{
		switch (MenuID)
		{
		case 10:
			Select_Disk(MASTER);
			BuildDynaMenu();
			SaveConfig();
			break;

		case 11:
			DropDisk(MASTER);
			BuildDynaMenu();
			SaveConfig();
			break;

		case 12:
			Select_Disk(SLAVE);
			BuildDynaMenu();
			SaveConfig();
			break;

		case 13:
			DropDisk(SLAVE);
			BuildDynaMenu();
			SaveConfig();
			break;

		case 14:
			CreateDialog( g_hinstDLL, (LPCTSTR) IDD_CONFIG,
					GetActiveWindow(), (DLGPROC) Config );
			ShowWindow(hConfDlg,1);
			break;
		}
		return;
	}
}

extern "C" 
{
	__declspec(dllexport) void SetIniPath (const char *IniFilePath)
	{
		strcpy(IniFile,IniFilePath);
		LoadConfig();
		return;
	}
}

void BuildDynaMenu(void)
{
	char TempMsg[512]="";
	char TempBuf[MAX_PATH]="";
	DynamicMenuCallback("",0,0);
	DynamicMenuCallback( "",6000,0);
	DynamicMenuCallback( "IDE Master",6000,HEAD);
	DynamicMenuCallback( "Insert",5010,SLAVE);
	QueryDisk(MASTER,TempBuf);
	strcpy(TempMsg,"Eject: ");
	PathStripPath (TempBuf);
	strcat(TempMsg,TempBuf);

	DynamicMenuCallback( TempMsg,5011,SLAVE);
	DynamicMenuCallback( "IDE Slave",6000,HEAD);
	DynamicMenuCallback( "Insert",5012,SLAVE);
	QueryDisk(SLAVE,TempBuf);
	strcpy(TempMsg,"Eject: ");
	PathStripPath (TempBuf);
	strcat(TempMsg,TempBuf);
	DynamicMenuCallback( TempMsg,5013,SLAVE);
	DynamicMenuCallback( "IDE Config",5014,STANDALONE);
	DynamicMenuCallback("",1,0);
}

LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	unsigned char BTemp=0;
	switch (message)
	{
		case WM_INITDIALOG:
			hConfDlg=hDlg;
			SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnabled,0);
			SendDlgItemMessage(hDlg,IDC_READONLY,BM_SETCHECK,ClockReadOnly,0);
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_ADDSTRING, 0, (LPARAM) "40");
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_ADDSTRING, 0, (LPARAM) "50");
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_ADDSTRING, 0, (LPARAM) "60");
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_ADDSTRING, 0, (LPARAM) "70");
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_SETCURSEL,BaseAddr,(LPARAM)0);
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

void SaveConfig(void)
{
	char ModName[MAX_LOADSTRING]="";
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	QueryDisk(MASTER,FileName);
	WritePrivateProfileString(ModName,"Master",FileName,IniFile);
	QueryDisk(SLAVE,FileName);
	WritePrivateProfileString(ModName,"Slave",FileName,IniFile);
	WritePrivateProfileInt(ModName,"BaseAddr",BaseAddr ,IniFile);
	WritePrivateProfileInt(ModName,"ClkEnable",ClockEnabled ,IniFile);
	WritePrivateProfileInt(ModName,"ClkRdOnly",ClockReadOnly ,IniFile);
	if (strcmp(SuperIDEPath, "") != 0) { 
		WritePrivateProfileString("DefaultPaths", "SuperIDEPath", SuperIDEPath, IniFile); 
	}

	return;
}

void LoadConfig(void)
{
	char ModName[MAX_LOADSTRING]="";

	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	GetPrivateProfileString("DefaultPaths", "SuperIDEPath", "", SuperIDEPath, MAX_PATH, IniFile);
	GetPrivateProfileString(ModName,"Master","",FileName,MAX_PATH,IniFile);
	MountDisk(FileName ,MASTER);
	GetPrivateProfileString(ModName,"Slave","",FileName,MAX_PATH,IniFile);
	BaseAddr=GetPrivateProfileInt(ModName,"BaseAddr",1,IniFile); 
	ClockEnabled=GetPrivateProfileInt(ModName,"ClkEnable",1,IniFile); 
	ClockReadOnly=GetPrivateProfileInt(ModName,"ClkRdOnly",1,IniFile); 
	BaseAddr&=3;
	if (BaseAddr==3)
		ClockEnabled=0;
	BaseAddress=BaseTable[BaseAddr];
	SetClockWrite(!ClockReadOnly);
	MountDisk(FileName ,SLAVE);
	BuildDynaMenu();
	return;
}
