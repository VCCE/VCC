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
/*
	RTC: $FF78-$FF7C
	IDE: $FF80-$FF86
*/

#include "cc3vhd.h"
#include "defines.h"
#include "cloud9.h"
#include "../fileops.h"
#include "../pakinterface.h"

#include <windows.h>
#include <stdio.h>
#include "resource.h" 

LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);
void Load_Disk(unsigned char);
void LoadHardDisk(int iDiskNum);
void LoadConfig(void);
void SaveConfig(void);
void BuildDynaMenu(void);
unsigned char LoadExtRom(char *);

//
// VCC Pak API
//
static vccpakapi_assertinterrupt_t	AssertInt			= NULL;
static vcccpu_read8_t				MemRead8			= NULL;
static vcccpu_write8_t				MemWrite8			= NULL;
static vccapi_dynamicmenucallback_t	DynamicMenuCallback = NULL;

static char VHD_FileName[2][MAX_PATH]	= { "","" };
static char IniFile[MAX_PATH]		{ 0 };
static char LastPath[MAX_PATH]		{ 0 };

constexpr size_t EXTROMSIZE = 8192;
static unsigned char *Memory=NULL;
static unsigned char DiskRom[8192];
static unsigned char ClockEnabled=1,ClockReadOnly=1;

static HINSTANCE g_hinstDLL;
static HWND g_hWnd = NULL;

void MemWrite(unsigned char Data,unsigned short Address)
{
	MemWrite8(Data,Address);
}

unsigned char MemRead(unsigned short Address)
{
	return MemRead8(Address);
}


void LoadHardDisk(int iDiskNum)
{
	OPENFILENAME ofn ;	
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner         = g_hWnd;
	ofn.lpstrFilter       =	"HardDisk Images\0*.vhd\0\0";	// filter VHD images
	ofn.nFilterIndex      = 1 ;								// current filter index
	ofn.lpstrFile         = VHD_FileName[iDiskNum];			// contains full path and filename on return
	ofn.nMaxFile          = MAX_PATH;						// sizeof lpstrFile
	ofn.lpstrFileTitle    = NULL;							// filename and extension only
	ofn.nMaxFileTitle     = MAX_PATH ;						// sizeof lpstrFileTitle
	ofn.lpstrInitialDir   = NULL;							// initial directory
	ofn.lpstrTitle        = TEXT("Load HardDisk Image") ;	// title bar string
	ofn.Flags             = OFN_HIDEREADONLY;

	if (strlen(LastPath) > 0)
	{
		ofn.lpstrInitialDir = LastPath;
	}

	if (GetOpenFileName(&ofn))
	{
		// save last path
		strcpy(LastPath, VHD_FileName[iDiskNum]);
		PathRemoveFileSpec(LastPath);

		if (MountHD(VHD_FileName[iDiskNum], iDiskNum) == 0)
		{
			MessageBox(NULL, "Can't open file", "Error", 0);
		}
	}
}

void LoadConfig(void)
{
	char ModName[MAX_LOADSTRING]="";
	char DiskRomPath[MAX_PATH]; 
	HANDLE hr=NULL;

	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);

	GetPrivateProfileString(ModName,"VHDImage","", VHD_FileName[0],MAX_PATH,IniFile);
	CheckPath(VHD_FileName[0]);
	hr=CreateFile(VHD_FileName[0],NULL,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hr==INVALID_HANDLE_VALUE)
	{
		strcpy(VHD_FileName[0],"");
		WritePrivateProfileString(ModName,"VHDImage", VHD_FileName[0],IniFile);
	}
	else
	{
		CloseHandle(hr);
		MountHD(VHD_FileName[0],0);
	}

	GetPrivateProfileString(ModName, "VHDImage1", "", VHD_FileName[1], MAX_PATH, IniFile);
	CheckPath(VHD_FileName[1]);
	hr = CreateFile(VHD_FileName[1], NULL, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hr == INVALID_HANDLE_VALUE)
	{
		strcpy(VHD_FileName[1], "");
		WritePrivateProfileString(ModName, "VHDImage1", VHD_FileName[1], IniFile);
	}
	else
	{
		CloseHandle(hr);
		MountHD(VHD_FileName[1], 1);
	}

	GetPrivateProfileString(ModName, "LastPath", "", LastPath, MAX_PATH, IniFile);

	BuildDynaMenu();
	GetModuleFileName(NULL, DiskRomPath, MAX_PATH);
	PathRemoveFileSpec(DiskRomPath);
	strcat(DiskRomPath, "rgbdos.rom");
	LoadExtRom(DiskRomPath);
}

void SaveConfig(void)
{
	char ModName[MAX_LOADSTRING]="";
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	ValidatePath(VHD_FileName[0]);
	WritePrivateProfileString(ModName, "VHDImage", VHD_FileName[0] ,IniFile);
	ValidatePath(VHD_FileName[1]);
	WritePrivateProfileString(ModName, "VHDImage1", VHD_FileName[1], IniFile);
	WritePrivateProfileString(ModName, "LastPath", LastPath, IniFile);
}

void BuildDynaMenu(void)
{
	char TempMsg[512]="";
	char TempBuf[MAX_PATH]="";

	DynamicMenuCallback("",0,0);
	DynamicMenuCallback("",6000,0);
	DynamicMenuCallback("HD Drive 0",6000, DMENU_HEAD);
	DynamicMenuCallback("Insert",5010, DMENU_SLAVE);
	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf, VHD_FileName[0]);
	PathStripPath (TempBuf);
	strcat(TempMsg,TempBuf);
	DynamicMenuCallback( TempMsg,5011, DMENU_SLAVE);

	DynamicMenuCallback("HD Drive 1", 6001, DMENU_HEAD);
	DynamicMenuCallback("Insert", 5012, DMENU_SLAVE);
	strcpy(TempMsg, "Eject: ");
	strcpy(TempBuf, VHD_FileName[1]);
	PathStripPath(TempBuf);
	strcat(TempMsg, TempBuf);
	DynamicMenuCallback(TempMsg, 5013, DMENU_SLAVE);
	
	//DynamicMenuCallback( "HD Config",5012,DMENU_STANDALONE);

	DynamicMenuCallback("",1,0);
}

unsigned char LoadExtRom( char *FilePath)	//Returns 1 on if loaded
{
	FILE *rom_handle=NULL;
	unsigned short index=0;
	unsigned char RetVal=0;


	rom_handle=fopen(FilePath,"rb");
	if (rom_handle == NULL)
	{
		memset(DiskRom, 0xFF, EXTROMSIZE);
	}
	else
	{
		while ((feof(rom_handle) == 0) & (index < EXTROMSIZE))
		{
			DiskRom[index++] = fgetc(rom_handle);
		}
		RetVal = 1;
		fclose(rom_handle);
	}

	return(RetVal);
}

/****************************************************************************/
/****************************************************************************/
/*
	VCC Pak API
*/
/****************************************************************************/

extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_GETNAME(char *ModName, char *CatNumber, vccapi_dynamicmenucallback_t Temp, void * wndHandle)
{
	g_hWnd = (HWND)wndHandle;
	LoadString(g_hinstDLL, IDS_MODULE_NAME, ModName, MAX_LOADSTRING);
	LoadString(g_hinstDLL, IDS_CATNUMBER, CatNumber, MAX_LOADSTRING);
	DynamicMenuCallback = Temp;
	SetClockWrite(!ClockReadOnly);

	if (DynamicMenuCallback != NULL)
	{
		BuildDynaMenu();
	}
}

extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_CONFIG(unsigned char MenuID)
{
	switch (MenuID)
	{
	case 10:
		LoadHardDisk(0);
		SaveConfig();
		BuildDynaMenu();
		break;

	case 11:
		UnmountHD(0);
		strcpy(VHD_FileName[0], "");
		SaveConfig();
		BuildDynaMenu();
		break;

	case 12:
		LoadHardDisk(1);
		SaveConfig();
		BuildDynaMenu();
		break;

	case 13:
		UnmountHD(1);
		strcpy(VHD_FileName[1], "");
		SaveConfig();
		BuildDynaMenu();
		break;

		//case 14:
		//	DialogBox(g_hinstDLL, (LPCTSTR)IDD_CONFIG, NULL, (DLGPROC)Config);
		//	SaveConfig();
		//	break;

	}
	//DialogBox(g_hinstDLL, (LPCTSTR)IDD_CONFIG, NULL, (DLGPROC)Config);
}

extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_PORTWRITE(unsigned char Port, unsigned char Data)
{
	IdeWrite(Data, Port);
}

extern "C" __declspec(dllexport) unsigned char VCC_PAKAPI_DEF_PORTREAD(unsigned char Port)
{
	if (  (Port == 0x78) 
		| (Port == 0x79) 
		| ((Port == 0x7C) & ClockEnabled)
		)
	{
		return(ReadTime(Port));
	}

	return (IdeRead(Port));
}


//This captures the pointers to the MemRead8 and MemWrite8 functions. This allows the DLL to do DMA xfers with CPU ram.
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_MEMPOINTERS(vcccpu_read8_t Temp1, vcccpu_write8_t Temp2)
{
	MemRead8 = Temp1;
	MemWrite8 = Temp2;
}

extern "C" __declspec(dllexport) unsigned char VCC_PAKAPI_DEF_MEMREAD(unsigned short Address)
{
	return DiskRom[Address & 8191];
}

extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_STATUS(char *MyStatus)
{
	DiskStatus(MyStatus);
}

extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_SETINIPATH(char *IniFilePath)
{
	strcpy(IniFile, IniFilePath);
	LoadConfig();
}

/****************************************************************************/

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpReserved)  // reserved
{
	if (fdwReason == DLL_PROCESS_DETACH) //Clean Up 
	{
		UnmountHD(0);
		UnmountHD(1);

		return(1);
	}

	g_hinstDLL = hinstDLL;

	return(1);
}

/****************************************************************************/
