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
 
#include <windows.h>
#include <stdio.h>
#include<iostream>
#include "resource.h" 
#include "cc3vhd.h"
//#include "diskrom.h"
#include "defines.h"
#include "cloud9.h"
#include "..\fileops.h"

constexpr size_t EXTROMSIZE = 8192;
static char FileName[MAX_PATH] { 0 };
static char IniFile[MAX_PATH] { 0 };
static char HardDiskPath[MAX_PATH];

typedef unsigned char (*MEMREAD8)(unsigned short);
typedef void (*MEMWRITE8)(unsigned char,unsigned short);
typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);
typedef void (*DMAMEMPOINTERS) ( MEMREAD8,MEMWRITE8);
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
static void (*AssertInt)(unsigned char,unsigned char)=NULL;
static unsigned char (*MemRead8)(unsigned short);
static void (*MemWrite8)(unsigned char,unsigned short);
static unsigned char *Memory=NULL;
static unsigned char DiskRom[8192];
static void (*DynamicMenuCallback)( char *,int, int)=NULL;
static unsigned char ClockEnabled=1,ClockReadOnly=1;
LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);
void Load_Disk(unsigned char);
void LoadHardDisk(void);
void LoadConfig(void);
void SaveConfig(void);
void BuildDynaMenu(void);
unsigned char LoadExtRom( char *);
static HINSTANCE g_hinstDLL;

using namespace std;

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
	if (fdwReason == DLL_PROCESS_DETACH ) //Clean Up 
	{
		UnmountHD();
		return(1);
	}
	g_hinstDLL=hinstDLL;
	return(1);
}

void MemWrite(unsigned char Data,unsigned short Address)
{
	MemWrite8(Data,Address);
	return;
}

unsigned char MemRead(unsigned short Address)
{
	return(MemRead8(Address));
}


extern "C" 
{          
	__declspec(dllexport) void ModuleName(char *ModName,char *CatNumber,DYNAMICMENUCALLBACK Temp)
	{
		LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
		LoadString(g_hinstDLL,IDS_CATNUMBER,CatNumber, MAX_LOADSTRING);	
		DynamicMenuCallback =Temp;
		SetClockWrite(!ClockReadOnly);
		if (DynamicMenuCallback  != NULL)
			BuildDynaMenu();		
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
			LoadHardDisk();
			SaveConfig();
			BuildDynaMenu();
			break;

		case 11:
			UnmountHD();
			strcpy(FileName ,"");
			SaveConfig();
			BuildDynaMenu();
			break;

//		case 12:
//			DialogBox(g_hinstDLL, (LPCTSTR)IDD_CONFIG, NULL, (DLGPROC)Config);
//			SaveConfig();
//			break;

		}
//		DialogBox(g_hinstDLL, (LPCTSTR)IDD_CONFIG, NULL, (DLGPROC)Config);
		return;
	}
}

/*
// This captures the Fuction transfer point for the CPU assert interupt 
extern "C" 
{
	__declspec(dllexport) void AssertInterupt(ASSERTINTERUPT Dummy)
	{
		AssertInt=Dummy;
		return;
	}
}
*/
extern "C" 
{         
	__declspec(dllexport) void PackPortWrite(unsigned char Port,unsigned char Data)
	{
		IdeWrite(Data,Port);
		return;
	}
}


extern "C"
{
	__declspec(dllexport) unsigned char PackPortRead(unsigned char Port)
	{
		if ( ((Port==0x78) | (Port==0x79) | (Port==0x7C)) & ClockEnabled)
			return(ReadTime(Port));
		return(IdeRead(Port));
	}
}


/*
extern "C"
{
	__declspec(dllexport) void HeartBeat(void)
	{
		PingFdc();
		return;
	}
}
*/

//This captures the pointers to the MemRead8 and MemWrite8 functions. This allows the DLL to do DMA xfers with CPU ram.

extern "C"
{
	__declspec(dllexport) void MemPointers(MEMREAD8 Temp1,MEMWRITE8 Temp2)
	{
		MemRead8=Temp1;
		MemWrite8=Temp2;
		return;
	}
}

extern "C"
{
	__declspec(dllexport) unsigned char PakMemRead8(unsigned short Address)
	{
		return(DiskRom[Address & 8191]);
	}
}
/*
extern "C"
{
	__declspec(dllexport) void PakMemWrite8(unsigned char Data,unsigned short Address)
	{

		return;
	}
}
*/

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
	__declspec(dllexport) void SetIniPath (char *IniFilePath)
	{
		strcpy(IniFile,IniFilePath);
		LoadConfig();
		return;
	}
}
/*
void CPUAssertInterupt(unsigned char Interupt,unsigned char Latencey)
{
	AssertInt(Interupt,Latencey);
	return;
}
*/

void LoadHardDisk(void)
{
	OPENFILENAME ofn ;	
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner         = NULL;
	ofn.lpstrFilter       =	"HardDisk Images\0*.vhd\0\0";	// filter VHD images
	ofn.nFilterIndex      = 1 ;								// current filter index
	ofn.lpstrFile         = FileName ;						// contains full path and filename on return
	ofn.nMaxFile          = MAX_PATH;						// sizeof lpstrFile
	ofn.lpstrFileTitle    = NULL;							// filename and extension only
	ofn.nMaxFileTitle     = MAX_PATH ;						// sizeof lpstrFileTitle
	ofn.lpstrInitialDir   = HardDiskPath;							// initial directory
	ofn.lpstrTitle        = TEXT("Load HardDisk Image") ;	// title bar string
	ofn.Flags             = OFN_HIDEREADONLY;
	if ( GetOpenFileName (&ofn) )
		if (MountHD(FileName)==0)
		{
			MessageBox(NULL,"Can't open file","Error",0);
		}
	string tmp = ofn.lpstrFile;
	int idx;
	idx = tmp.find_last_of("\\");
	tmp = tmp.substr(0, idx);
	strcpy(HardDiskPath, tmp.c_str());

	return;
}

void LoadConfig(void)
{
	char ModName[MAX_LOADSTRING]="";
	char DiskRomPath[MAX_PATH]; 
	HANDLE hr=NULL;

	GetPrivateProfileString("DefaultPaths", "HardDiskPath", "", HardDiskPath, MAX_PATH, IniFile);
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	
	GetPrivateProfileString(ModName,"VHDImage","",FileName,MAX_PATH,IniFile);
	CheckPath(FileName);
	hr=CreateFile(FileName,NULL,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hr==INVALID_HANDLE_VALUE)
	{
		strcpy(FileName,"");
		WritePrivateProfileString(ModName,"VHDImage",FileName ,IniFile);
	}
	else
	{
		CloseHandle(hr);
		MountHD(FileName);
	}
	BuildDynaMenu();
	GetModuleFileName(NULL, DiskRomPath, MAX_PATH);
	PathRemoveFileSpec(DiskRomPath);
	strcat(DiskRomPath, "rgbdos.rom");
	LoadExtRom(DiskRomPath);
	return;
}

void SaveConfig(void)
{
	char ModName[MAX_LOADSTRING]="";
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	ValidatePath(FileName);
	if (HardDiskPath != "") { WritePrivateProfileString("DefaultPaths", "HardDiskPath", HardDiskPath, IniFile); }
	WritePrivateProfileString(ModName,"VHDImage",FileName ,IniFile);
	return;
}

void BuildDynaMenu(void)
{
	char TempMsg[512]="";
	char TempBuf[MAX_PATH]="";
	DynamicMenuCallback("",0,0);
	DynamicMenuCallback( "",6000,0);
	DynamicMenuCallback( "HD Drive 0",6000,HEAD);
	DynamicMenuCallback( "Insert",5010,SLAVE);
	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf,FileName);
	PathStripPath (TempBuf);
	strcat(TempMsg,TempBuf);
	DynamicMenuCallback( TempMsg,5011,SLAVE);
//	DynamicMenuCallback( "HD Config",5012,STANDALONE);
	DynamicMenuCallback("",1,0);

}

unsigned char LoadExtRom( char *FilePath)	//Returns 1 on if loaded
{

	FILE *rom_handle=NULL;
	unsigned short index=0;
	unsigned char RetVal=0;


	rom_handle=fopen(FilePath,"rb");
	if (rom_handle==NULL)
		memset(DiskRom,0xFF,EXTROMSIZE);
	else
	{
		while ((feof(rom_handle)==0) & (index<EXTROMSIZE))
			DiskRom[index++]=fgetc(rom_handle);
		RetVal=1;
		fclose(rom_handle);
	}
	return(RetVal);
}

