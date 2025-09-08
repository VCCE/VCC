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
/********************************************************************************************
*	fd502.cpp : Defines the entry point for the DLL application.							*
*	DLL for use with Vcc 1.0 or higher. DLL interface 1.0 Beta								*
*	This Module will emulate a Tandy Floppy Disk Model FD-502 With 3 DSDD drives attached	*
*	Copyright 2006 (c) by Joseph Forgione 													*
*********************************************************************************************/
#pragma warning( disable : 4800 ) // For legacy builds

//#define USE_LOGGING
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include "resource.h"
#include "wd1793.h"
#include "distortc.h"
#include "fd502.h"
#include "../fileops.h"
#include "../DialogOps.h"
#include "../logger.h"

//include becker code if COMBINE_BECKER is defined in fd502.h
#ifdef COMBINE_BECKER
#include "becker.h"
#endif

#define EXTROMSIZE 16384

using namespace std;

extern DiskInfo Drive[5];
typedef unsigned char (*MEMREAD8)(unsigned short);
typedef void (*MEMWRITE8)(unsigned char,unsigned short);
typedef void (*PAKINTERUPT)(unsigned char, unsigned char);
typedef void (*DMAMEMPOINTERS) ( MEMREAD8,MEMWRITE8);
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
static unsigned char ExternalRom[EXTROMSIZE];
static unsigned char DiskRom[EXTROMSIZE];
static unsigned char RGBDiskRom[EXTROMSIZE];
static char FloppyPath[MAX_PATH];
static char RomFileName[MAX_PATH]="";
static char TempRomFileName[MAX_PATH]="";
void (*AssertInt)(unsigned char,unsigned char)=nullptr;
static void (*DynamicMenuCallback)( char *,int, int)=nullptr;
static unsigned char (*MemRead8)(unsigned short);
static void (*MemWrite8)(unsigned char,unsigned short);
static unsigned char *Memory=nullptr;
unsigned char PhysicalDriveA=0,PhysicalDriveB=0,OldPhysicalDriveA=0,OldPhysicalDriveB=0;
static unsigned char *RomPointer[3]={ExternalRom,DiskRom,RGBDiskRom};
static unsigned char SelectRom=0;
unsigned char SetChip(unsigned char);
static unsigned char NewDiskNumber=0,DialogOpen=0,CreateFlag=0;
static unsigned char PersistDisks=0;
static char IniFile[MAX_PATH]="";
static unsigned char TempSelectRom=0;
static unsigned char ClockEnabled=1,ClockReadOnly=1;
LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK NewDisk(HWND,UINT, WPARAM, LPARAM);
void LoadConfig(void);
void SaveConfig(void);
long CreateDiskHeader(char *,unsigned char,unsigned char,unsigned char);
void Load_Disk(unsigned char);
void CenterDialog(HWND hDlg);

static HWND g_hConfDlg = nullptr;
static HINSTANCE g_hinstDLL;
static unsigned long RealDisks=0;
long CreateDisk (unsigned char);
static char TempFileName[MAX_PATH]="";
unsigned char LoadExtRom( unsigned char,char *);

int BeckerEnabled=0;
char BeckerAddr[MAX_PATH]="";
char BeckerPort[32]="";

//----------------------------------------------------------------------------
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
	if (fdwReason == DLL_PROCESS_DETACH ) //Clean Up
	{
        CloseCartDialog(g_hConfDlg);
		for (unsigned char Drive=0;Drive<=3;Drive++)
			unmount_disk_image(Drive);
	}
	else
	{
		g_hinstDLL=hinstDLL;
		RealDisks=InitController();
	}
	return 1;
}

extern "C"
{
	__declspec(dllexport) void ModuleName(char *ModName,char *CatNumber,DYNAMICMENUCALLBACK Temp)
	{
		int ErrorNumber=0;
		LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
		LoadString(g_hinstDLL,IDS_CATNUMBER,CatNumber, MAX_LOADSTRING);
		DynamicMenuCallback =Temp;
		if (DynamicMenuCallback  != nullptr)
			BuildDynaMenu();
		return ;
	}
}

extern "C"
{
	__declspec(dllexport) void ModuleConfig(unsigned char MenuID)
	{
		HWND h_own = GetActiveWindow();
		switch (MenuID)
		{
			case 10:
				Load_Disk(0);
				break;
			case 11:
				unmount_disk_image(0);
				SaveConfig();
				break;
			case 12:
				Load_Disk(1);
				break;
			case 13:
				unmount_disk_image(1);
				SaveConfig();
				break;
			case 14:
				Load_Disk(2);
				break;
			case 15:
				unmount_disk_image(2);
				SaveConfig();
				break;
			case 16:
				if (g_hConfDlg == nullptr)
					g_hConfDlg = CreateDialog(g_hinstDLL,(LPCTSTR)IDD_CONFIG,h_own,(DLGPROC)Config);
				ShowWindow(g_hConfDlg,1);
				break;
			case 17:
				Load_Disk(3);
				break;
			case 18:
				unmount_disk_image(3);
				SaveConfig();
				break;
		}
		BuildDynaMenu();
		return;
	}
}

extern "C"
{
	__declspec(dllexport) void SetIniPath (char *IniFilePath)
	{
		strcpy(IniFile,IniFilePath);
		LoadConfig();
		return;
	}
}

// This captures the Fuction transfer point for the CPU assert interupt
extern "C"
{
	__declspec(dllexport) void AssertInterupt(PAKINTERUPT Dummy)
	{
		AssertInt=Dummy;
		return;
	}
}

extern "C"
{
	__declspec(dllexport) void PackPortWrite(unsigned char Port,unsigned char Data)
	{
#ifdef COMBINE_BECKER
		if (BeckerEnabled && (Port == 0x42)) {
			becker_write(Data,Port);
		} else //if ( ((Port == 0x50) | (Port==0x51)) & ClockEnabled) {
#endif
		if ( ((Port == 0x50) | (Port==0x51)) & ClockEnabled) {
			write_time(Data,Port);
		} else {
			disk_io_write(Data,Port);
		}
		return;
	}
}

extern "C"
{
	__declspec(dllexport) unsigned char PackPortRead(unsigned char Port)
	{
#ifdef COMBINE_BECKER
		if (BeckerEnabled && ((Port == 0x41) | (Port== 0x42 )))
			return(becker_read(Port));
#endif
		if ( ((Port == 0x50) | (Port== 0x51 )) & ClockEnabled)
			return(read_time(Port));
		return(disk_io_read(Port));
	}
}

extern "C"
{
	__declspec(dllexport) void HeartBeat(void)
	{
		PingFdc();
		return;
	}
}

extern "C"
{
	__declspec(dllexport) unsigned char PakMemRead8(unsigned short Address)
	{
		return(RomPointer[SelectRom][Address & (EXTROMSIZE-1)]);
	}
}

extern "C"
{
	__declspec(dllexport) void ModuleStatus(char *MyStatus)
	{
		char diskstat[64];
		DiskStatus(diskstat);
		strcpy(MyStatus,diskstat);
#ifdef COMBINE_BECKER
		char beckerstat[64];
		if(BeckerEnabled) {
			becker_status(beckerstat);
			strcat(MyStatus," | ");
			strcat(MyStatus,beckerstat);
		}
#endif
		return ;
	}
}

void CenterDialog(HWND hDlg)
{
    RECT rPar, rDlg;
    GetWindowRect(GetParent(hDlg), &rPar);
    GetWindowRect(hDlg, &rDlg);
    int x = rPar.left + (rPar.right - rPar.left - (rDlg.right - rDlg.left)) / 2;
    int y = rPar.top + (rPar.bottom - rPar.top - (rDlg.bottom - rDlg.top)) / 2;
    SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static unsigned char CurrentDisk=0,temp=0,temp2=0;
	long ChipChoice[3]={IDC_EXTROM,IDC_TRSDOS,IDC_RGB};
	long VirtualDrive[2]={IDC_DISKA,IDC_DISKB};
	char VirtualNames[5][16]={"None","Drive 0","Drive 1","Drive 2","Drive 3"};

	switch (message)
	{
		case WM_CLOSE:
			DestroyWindow(hDlg);
			g_hConfDlg = nullptr;
			return TRUE;
			break;

		case WM_INITDIALOG:
			CenterDialog(hDlg);
			TempSelectRom=SelectRom;
			if (!RealDisks)
			{
				EnableWindow(GetDlgItem(hDlg, IDC_DISKA), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_DISKB), FALSE);
				PhysicalDriveA=0;
				PhysicalDriveB=0;
			}
			SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnabled,0);
			OldPhysicalDriveA=PhysicalDriveA;
			OldPhysicalDriveB=PhysicalDriveB;
			strcpy(TempRomFileName,RomFileName);
			SendDlgItemMessage(hDlg,IDC_TURBO,BM_SETCHECK,SetTurboDisk(QUERY),0);
			SendDlgItemMessage(hDlg,IDC_PERSIST,BM_SETCHECK,PersistDisks,0);
			for (temp=0;temp<sizeof(ChipChoice) / sizeof(*ChipChoice);temp++)
				SendDlgItemMessage(hDlg,ChipChoice[temp],BM_SETCHECK,(temp==TempSelectRom),0);
			for (temp=0;temp<2;temp++)
				for (temp2=0;temp2<5;temp2++)
						SendDlgItemMessage (hDlg,VirtualDrive[temp], CB_ADDSTRING, 0, (LPARAM) VirtualNames[temp2]);

			SendDlgItemMessage (hDlg, IDC_DISKA,CB_SETCURSEL,(WPARAM)PhysicalDriveA,(LPARAM)0);
			SendDlgItemMessage (hDlg, IDC_DISKB,CB_SETCURSEL,(WPARAM)PhysicalDriveB,(LPARAM)0);
			SendDlgItemMessage (hDlg,IDC_ROMPATH,WM_SETTEXT,0,(LPARAM)(LPCSTR)TempRomFileName);

			SendDlgItemMessage (hDlg,IDC_BECKER_ENAB,BM_SETCHECK,BeckerEnabled,0);
			SendDlgItemMessage (hDlg,IDC_BECKER_HOST,WM_SETTEXT,0,(LPARAM)(LPSTR)BeckerAddr);
			SendDlgItemMessage (hDlg,IDC_BECKER_PORT,WM_SETTEXT,0,(LPARAM)(LPCSTR)BeckerPort);

			return TRUE;
		break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					ClockEnabled=(unsigned char)SendDlgItemMessage(hDlg,IDC_CLOCK,BM_GETCHECK,0,0);
					SetTurboDisk((unsigned char)SendDlgItemMessage(hDlg,IDC_TURBO,BM_GETCHECK,0,0));
					PersistDisks=(unsigned char) SendDlgItemMessage(hDlg,IDC_PERSIST,BM_GETCHECK,0,0);
					PhysicalDriveA=(unsigned char)SendDlgItemMessage(hDlg,IDC_DISKA,CB_GETCURSEL,0,0);
					PhysicalDriveB=(unsigned char)SendDlgItemMessage(hDlg,IDC_DISKB,CB_GETCURSEL,0,0);
					if (!RealDisks)
					{
						PhysicalDriveA=0;
						PhysicalDriveB=0;
					//	MessageBox(0,"Wrong Version or Driver not installed","FDRAWCMD Driver",0);
					}
					else
					{
						if (PhysicalDriveA != OldPhysicalDriveA)	//Drive changed
						{
							if (OldPhysicalDriveA!=0)
								unmount_disk_image(OldPhysicalDriveA-1);
							if (PhysicalDriveA!=0)
								mount_disk_image("*Floppy A:",PhysicalDriveA-1);
						}
						if (PhysicalDriveB != OldPhysicalDriveB)	//Drive changed
						{
							if (OldPhysicalDriveB!=0)
								unmount_disk_image(OldPhysicalDriveB-1);
							if (PhysicalDriveB!=0)
								mount_disk_image("*Floppy B:",PhysicalDriveB-1);
						}
					}
					SelectRom=TempSelectRom;
					strcpy(RomFileName,TempRomFileName );
					CheckPath(RomFileName);
					LoadExtRom(External,RomFileName); //JF
#ifdef COMBINE_BECKER
					GetDlgItemText(hDlg,IDC_BECKER_HOST,BeckerAddr,MAX_PATH);
					GetDlgItemText(hDlg,IDC_BECKER_PORT,BeckerPort,32);
					becker_sethost(BeckerAddr,BeckerPort);
					BeckerEnabled = SendDlgItemMessage (hDlg,IDC_BECKER_ENAB,BM_GETCHECK,0,0);
					becker_enable(BeckerEnabled);
#endif
					SaveConfig();
					DestroyWindow(hDlg);
					g_hConfDlg = nullptr;
					return TRUE;
					break;

				case IDC_EXTROM:
				case IDC_TRSDOS:
				case IDC_RGB:
					for (temp=0;temp<sizeof(ChipChoice) / sizeof(*ChipChoice);temp++)
						if (LOWORD(wParam)==ChipChoice[temp])
						{
							for (temp2=0;temp2<sizeof(ChipChoice) / sizeof(*ChipChoice);temp2++)
								SendDlgItemMessage(hDlg,ChipChoice[temp2],BM_SETCHECK,0,0);
							SendDlgItemMessage(hDlg,ChipChoice[temp],BM_SETCHECK,1,0);
							TempSelectRom=temp;
						}
					break;
				case IDC_BROWSE:
					{
						char dir[MAX_PATH];
						strncpy(dir,TempRomFileName,MAX_PATH);
						if (char * p = strrchr(dir,'\\')) *p = '\0';

						FileDialog dlg;
						dlg.setFilter("Disk Rom Images\0*.rom;\0All files\0*.*\0\0");
						dlg.setTitle("Select Disk Rom Image");
						dlg.setInitialDir(dir);
						if (dlg.show(0,g_hConfDlg)) {
							dlg.getpath(TempRomFileName,MAX_PATH);
							SendDlgItemMessage
								(hDlg,IDC_ROMPATH,WM_SETTEXT,0,(LPARAM)dlg.path());
						}
					}
					break;
				case IDC_CLOCK:
				case IDC_READONLY:
					break;

				case IDCANCEL:
					DestroyWindow(hDlg);
					g_hConfDlg = nullptr;
					break;
			}
			return TRUE;
		break;

	}
    return FALSE;
}

void Load_Disk(unsigned char disk)
{
	HWND h_own = GetActiveWindow();
	FileDialog dlg;
	dlg.setInitialDir(FloppyPath);
	dlg.setFilter("Disk Images\0*.dsk;*.os9\0\0");
	dlg.setDefExt("dsk");
	dlg.setTitle("Insert Disk Image");
	dlg.setFlags(OFN_PATHMUSTEXIST);
	if (dlg.show(0,h_own)) {
		CreateFlag = 1;
		HANDLE hr = nullptr;
		dlg.getpath(TempFileName,MAX_PATH);
		// Verify file exists
		hr = CreateFile
			(TempFileName,0,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
		// Create new disk file if it does not
		if (hr == INVALID_HANDLE_VALUE) {
			NewDiskNumber = disk;
			//DialogBox will set CreateFlag = 0 on cancel
			DialogBox(g_hinstDLL, (LPCTSTR)IDD_NEWDISK, h_own, (DLGPROC)NewDisk);
		} else {
			CloseHandle(hr);
		}
		// Attempt mount if file existed or file create was not canceled
		if (CreateFlag==1) {
			if (mount_disk_image(TempFileName,disk)==0) {
				MessageBox(g_hConfDlg,"Can't open file","Error",0);
			} else {
				dlg.getdir(FloppyPath);
				SaveConfig();
			}
		}
	}
	return;
}

unsigned char SetChip(unsigned char Tmp)
{
	if (Tmp!=QUERY)
		SelectRom=Tmp;
	return SelectRom;
}

void BuildDynaMenu(void)
{
	char TempMsg[64]="";
	char TempBuf[MAX_PATH]="";
	if (DynamicMenuCallback ==nullptr)
		MessageBox(g_hConfDlg,"No good","Ok",0);
	DynamicMenuCallback("",0,0);
	DynamicMenuCallback( "",6000,0);
	DynamicMenuCallback( "FD-502 Drive 0",6000,HEAD);
	DynamicMenuCallback( "Insert",5010,SLAVE);
	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf,Drive[0].ImageName);
	PathStripPath(TempBuf);
	strcat(TempMsg,TempBuf);
	DynamicMenuCallback( TempMsg,5011,SLAVE);

	DynamicMenuCallback( "FD-502 Drive 1",6000,HEAD);
	DynamicMenuCallback( "Insert",5012,SLAVE);
	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf,Drive[1].ImageName);
	PathStripPath(TempBuf);
	strcat(TempMsg,TempBuf);
	DynamicMenuCallback( TempMsg,5013,SLAVE);

	DynamicMenuCallback( "FD-502 Drive 2",6000,HEAD);
	DynamicMenuCallback( "Insert",5014,SLAVE);
	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf,Drive[2].ImageName);
	PathStripPath(TempBuf);
	strcat(TempMsg,TempBuf);
	DynamicMenuCallback( TempMsg,5015,SLAVE);
//NEW
	DynamicMenuCallback( "FD-502 Drive 3",6000,HEAD);
	DynamicMenuCallback( "Insert",5017,SLAVE);
	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf,Drive[3].ImageName);
	PathStripPath(TempBuf);
	strcat(TempMsg,TempBuf);
	DynamicMenuCallback( TempMsg,5018,SLAVE);
//NEW
	DynamicMenuCallback( "FD-502 Config",5016,STANDALONE);
	DynamicMenuCallback( "",1,0);
}

long CreateDisk (unsigned char Disk)
{
	NewDiskNumber=Disk;
	HWND h_own = GetActiveWindow();
	DialogBox(g_hinstDLL, (LPCTSTR)IDD_NEWDISK, h_own, (DLGPROC)NewDisk);
	return 0;
}

LRESULT CALLBACK NewDisk(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned char temp=0,temp2=0;
	static unsigned char NewDiskType=JVC,NewDiskTracks=2,DblSided=1;
	char Dummy[MAX_PATH]="";
	long DiskType[3]={IDC_JVC,IDC_VDK,IDC_DMK};
	long DiskTracks[3]={IDC_TR35,IDC_TR40,IDC_TR80};

	switch (message)
	{
		case WM_CLOSE:
			EndDialog(hDlg,LOWORD(wParam));  //Modal dialog
			return TRUE;
			break;

		case WM_INITDIALOG:
			for (temp=0;temp<=2;temp++)
				SendDlgItemMessage(hDlg,DiskType[temp],BM_SETCHECK,(temp==NewDiskType),0);
			for (temp=0;temp<=2;temp++)
				SendDlgItemMessage(hDlg,DiskTracks[temp],BM_SETCHECK,(temp==NewDiskTracks),0);
			SendDlgItemMessage(hDlg,IDC_DBLSIDE,BM_SETCHECK,DblSided,0);
			strcpy(Dummy,TempFileName);
			PathStripPath(Dummy);
			SendDlgItemMessage(hDlg,IDC_TEXT1,WM_SETTEXT,0,(LPARAM)(LPCSTR)Dummy);
			return TRUE;
		break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_DMK:
				case IDC_JVC:
				case IDC_VDK:
					for (temp=0;temp<=2;temp++)
						if (LOWORD(wParam)==DiskType[temp])
						{
							for (temp2=0;temp2<=2;temp2++)
								SendDlgItemMessage(hDlg,DiskType[temp2],BM_SETCHECK,0,0);
							SendDlgItemMessage(hDlg,DiskType[temp],BM_SETCHECK,1,0);
							NewDiskType=temp;
						}
				break;

				case IDC_TR35:
				case IDC_TR40:
				case IDC_TR80:
					for (temp=0;temp<=2;temp++)
						if (LOWORD(wParam)==DiskTracks[temp])
						{
							for (temp2=0;temp2<=2;temp2++)
								SendDlgItemMessage(hDlg,DiskTracks[temp2],BM_SETCHECK,0,0);
							SendDlgItemMessage(hDlg,DiskTracks[temp],BM_SETCHECK,1,0);
							NewDiskTracks=temp;
						}
				break;

				case IDC_DBLSIDE:
					DblSided=!DblSided;
					SendDlgItemMessage(hDlg,IDC_DBLSIDE,BM_SETCHECK,DblSided,0);
				break;

				case IDOK:
				{
					EndDialog(hDlg, LOWORD(wParam));
					const char *ext = TempFileName + strlen(TempFileName) - 4;
					if (ext < TempFileName) ext = TempFileName;
					// if it doesn't end with .dsk, append it
					if (_stricmp(ext, ".dsk") != 0)
					{
						PathRemoveExtension(TempFileName);
						strcat(TempFileName, ".dsk");
					}
					if (CreateDiskHeader(TempFileName, NewDiskType, NewDiskTracks, DblSided))
					{
						strcpy(TempFileName, "");
						MessageBox(g_hConfDlg, "Can't create File", "Error", 0);
					}
					return TRUE;
				}
				break;

				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					CreateFlag=0;
					return FALSE;
				break;
			}
			return TRUE;
		break;
	}
    return FALSE;
}

long CreateDiskHeader(char *FileName,unsigned char Type,unsigned char Tracks,unsigned char DblSided)
{
	HANDLE hr=nullptr;
	unsigned char Dummy=0;
	unsigned char HeaderBuffer[16]="";
	unsigned char TrackTable[3]={35,40,80};
	unsigned short TrackSize=0x1900;
	unsigned char IgnoreDensity=0,SingleDensity=0,HeaderSize=0;
	unsigned long BytesWritten=0,FileSize=0;
	hr=CreateFile( FileName,GENERIC_READ | GENERIC_WRITE,0,0,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
	if (hr==INVALID_HANDLE_VALUE)
		return 1; //Failed to create File

	switch (Type)
	{
		case DMK:
			HeaderBuffer[0]=0;
			HeaderBuffer[1]=TrackTable[Tracks];
			HeaderBuffer[2]=(TrackSize & 0xFF);
			HeaderBuffer[3]=(TrackSize >>8);
			HeaderBuffer[4]=(IgnoreDensity<<7) | (SingleDensity<<6) | ((!DblSided)<<4);
			HeaderBuffer[0xC]=0;
			HeaderBuffer[0xD]=0;
			HeaderBuffer[0xE]=0;
			HeaderBuffer[0xF]=0;
			HeaderSize=0x10;
			FileSize= HeaderSize + (HeaderBuffer[1] * TrackSize * (DblSided+1) );
		break;

		case JVC:	// has variable header size
			HeaderSize=0;
			HeaderBuffer[0]=18;			//18 Sectors
			HeaderBuffer[1]=DblSided+1;	// Double or single Sided;
			FileSize = (HeaderBuffer[0] * 0x100 * TrackTable[Tracks] * (DblSided+1));
			if (DblSided)
			{
				FileSize+=2;
				HeaderSize=2;
			}
		break;

		case VDK:
			HeaderBuffer[9]=DblSided+1;
			HeaderSize=12;
			FileSize = ( 18 * 0x100 * TrackTable[Tracks] * (DblSided+1));
			FileSize+=HeaderSize;
		break;

		case 3:
			HeaderBuffer[0]=0;
			HeaderBuffer[1]=TrackTable[Tracks];
			HeaderBuffer[2]=(TrackSize & 0xFF);
			HeaderBuffer[3]=(TrackSize >>8);
			HeaderBuffer[4]=((!DblSided)<<4);
			HeaderBuffer[0xC]=0x12;
			HeaderBuffer[0xD]=0x34;
			HeaderBuffer[0xE]=0x56;
			HeaderBuffer[0xF]=0x78;
			HeaderSize=0x10;
			FileSize=1;
		break;

	}
	SetFilePointer(hr,0,0,FILE_BEGIN);
	WriteFile(hr,HeaderBuffer,HeaderSize,&BytesWritten,nullptr);
	SetFilePointer(hr,FileSize-1,0,FILE_BEGIN);
	WriteFile(hr,&Dummy,1,&BytesWritten,nullptr);
	CloseHandle(hr);
	return 0;
}

void LoadConfig(void)  // Called on SetIniPath
{
	char ModName[MAX_LOADSTRING]="";
	unsigned char Index=0;
	char Temp[16]="";
	char DiskRomPath[MAX_PATH], RGBRomPath[MAX_PATH];
	char DiskName[MAX_PATH]="";
	unsigned int RetVal=0;
	HANDLE hr=nullptr;

#ifdef COMBINE_BECKER
	strcpy(ModName,"HDBDOS/DW/Becker");
	BeckerEnabled=GetPrivateProfileInt(ModName,"DWEnable", 0, IniFile);
	GetPrivateProfileString(ModName,"DWServerAddr","127.0.0.1",BeckerAddr,MAX_PATH,IniFile);
	GetPrivateProfileString(ModName,"DWServerPort","65504",BeckerPort,32,IniFile);
	if(*BeckerAddr == '\0') strcpy(BeckerAddr,"127.0.0.1");
	if(*BeckerPort == '\0') strcpy(BeckerPort,"65504");
	becker_sethost(BeckerAddr,BeckerPort);
	becker_enable(BeckerEnabled);
#endif

	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	GetPrivateProfileString("DefaultPaths", "FloppyPath", "", FloppyPath, MAX_PATH, IniFile);

	SelectRom = GetPrivateProfileInt(ModName,"DiskRom",1,IniFile);  //0 External 1=TRSDOS 2=RGB Dos
	GetPrivateProfileString(ModName,"RomPath","",RomFileName,MAX_PATH,IniFile);
	PersistDisks=GetPrivateProfileInt(ModName,"Persist",1,IniFile);
	CheckPath(RomFileName);
	LoadExtRom(External,RomFileName); //JF
	GetModuleFileName(nullptr, DiskRomPath, MAX_PATH);
	PathRemoveFileSpec(DiskRomPath);
	strcpy(RGBRomPath, DiskRomPath);
	strcat(DiskRomPath, "disk11.rom"); //Failing silent, Maybe we should throw a warning?
	strcat(RGBRomPath, "rgbdos.rom");  //Future, Grey out dialog option if can't find file
	LoadExtRom(TandyDisk, DiskRomPath);
	LoadExtRom(RGBDisk, RGBRomPath);
	if (PersistDisks)
		for (Index=0;Index<4;Index++)
		{
			sprintf(Temp,"Disk#%i",Index);
			GetPrivateProfileString(ModName,Temp,"",DiskName,MAX_PATH,IniFile);
			if (strlen(DiskName))
			{
				RetVal=mount_disk_image(DiskName,Index);
				//MessageBox(0, "Disk load attempt", "OK", 0);
				if (RetVal)
				{
					if (!strcmp(DiskName,"*Floppy A:"))	//RealDisks
						PhysicalDriveA=Index+1;
					if (!strcmp(DiskName,"*Floppy B:"))
						PhysicalDriveB=Index+1;
				}
			}
		}
	ClockEnabled=GetPrivateProfileInt(ModName,"ClkEnable",1,IniFile);
	SetTurboDisk(GetPrivateProfileInt(ModName, "TurboDisk", 1, IniFile));

	BuildDynaMenu();
	return;
}

void SaveConfig(void)
{
	unsigned char Index=0;
	char ModName[MAX_LOADSTRING]="";
	char Temp[16]="";
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	ValidatePath(RomFileName);
	WritePrivateProfileInt(ModName,"DiskRom",SelectRom ,IniFile);
	WritePrivateProfileString(ModName,"RomPath",RomFileName,IniFile);
	WritePrivateProfileInt(ModName,"Persist",PersistDisks ,IniFile);
	if (PersistDisks)
		for (Index=0;Index<4;Index++)
		{
			sprintf(Temp,"Disk#%i",Index);
			WritePrivateProfileString(ModName,Temp,Drive[Index].ImageName,IniFile);
		}
	WritePrivateProfileInt(ModName,"ClkEnable",ClockEnabled ,IniFile);
	WritePrivateProfileInt(ModName, "TurboDisk", SetTurboDisk(QUERY), IniFile);
	if (strcmp(FloppyPath, "") != 0) {
		WritePrivateProfileString("DefaultPaths", "FloppyPath", FloppyPath, IniFile);
	}
#ifdef COMBINE_BECKER
    strcpy(ModName,"HDBDOS/DW/Becker");
	WritePrivateProfileInt(ModName,"DWEnable", BeckerEnabled, IniFile);
	WritePrivateProfileString(ModName,"DWServerAddr", BeckerAddr, IniFile);
	WritePrivateProfileString(ModName,"DWServerPort", BeckerPort, IniFile);
#endif
	return;
}

unsigned char LoadExtRom( unsigned char RomType,char *FilePath)	//Returns 1 on if loaded
{

	FILE *rom_handle=nullptr;
	unsigned short index=0;
	unsigned char RetVal=0;
	unsigned char *ThisRom[3]={ExternalRom,DiskRom,RGBDiskRom};

	rom_handle=fopen(FilePath,"rb");
	if (rom_handle==nullptr)
		memset(ThisRom[RomType],0xFF,EXTROMSIZE);
	else
	{
		while ((feof(rom_handle)==0) & (index<EXTROMSIZE))
			ThisRom[RomType][index++]=fgetc(rom_handle);
		RetVal=1;
		fclose(rom_handle);
	}

	return RetVal;
}
