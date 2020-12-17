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

static char VHDfile0[MAX_PATH] { 0 };
static char VHDfile1[MAX_PATH] { 0 };
static char IniFile[MAX_PATH]  { 0 };
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

///void Load_Disk(unsigned char);
void LoadHardDisk(int drive);
void LoadConfig(void);
void SaveConfig(void);
void BuildDynaMenu(void);
unsigned char LoadExtRom(char *);
static HINSTANCE g_hinstDLL;

using namespace std;

BOOL WINAPI DllMain( HINSTANCE hinstDLL,  // handle to DLL module
                     DWORD fdwReason,     // reason for calling function
                     LPVOID lpReserved )  // reserved
{
    if (fdwReason == DLL_PROCESS_DETACH ) {
        UnmountHD(0);
        UnmountHD(1);
    } else {
        g_hinstDLL=hinstDLL;
    }
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

// Register the DLL
extern "C"
{
    __declspec(dllexport) void
    ModuleName(char *ModName,char *CatNumber,DYNAMICMENUCALLBACK Temp)
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

// Configure the hard drive(s).  Called from menu
// Mount or dismount a hard drive and save config
// MountHD and UnmountHD are defined in cc3vhd
extern "C"
{
    __declspec(dllexport) void
    ModuleConfig(unsigned char MenuID)
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

//      case 14:
//          DialogBox(g_hinstDLL, (LPCTSTR)IDD_CONFIG, NULL, (DLGPROC)Config);
//          SaveConfig();
//          break;
        }
        SaveConfig();
        BuildDynaMenu();
        return;
    }
}

/*
// This captures the Fuction transfer point for the CPU assert interupt
//
// NOTE: Vcc currently is not doing interupts after DMA transfers!
//       This only works because it is single threaded which causes 6x09
//       emulation to halt until transfers complete. Real hardware differs.
//
// TODO: Asyncronous Disk I/O
//

extern "C"
{
    __declspec(dllexport)
    void AssertInterupt(ASSERTINTERUPT Dummy)
    {
        AssertInt=Dummy;
        return;
    }
}
*/

// Export write to HD control port
extern "C"
{
    __declspec(dllexport) void
    PackPortWrite(unsigned char Port,unsigned char Data)
    {
        IdeWrite(Data,Port);
        return;
    }
}

// Export read from HD control port
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

// Set pointers to the MemRead8 and MemWrite8 functions.
// This allows the DLL to do DMA xfers with CPU ram.
extern "C"
{
    __declspec(dllexport) void
    MemPointers(MEMREAD8 Temp1,MEMWRITE8 Temp2)
    {
        MemRead8  = Temp1;
        MemWrite8 = Temp2;
        return;
    }
}

// Hook to read disk rom
extern "C"
{
    __declspec(dllexport) unsigned char
    PakMemRead8(unsigned short Address)
    {
        return(DiskRom[Address & 8191]);
    }
}

/*
extern "C"
{
    __declspec(dllexport) void
    PakMemWrite8(unsigned char Data,unsigned short Address)
    {
        return;
    }
}
*/

// Return disk status. (from cc3vhd)
extern "C"
{
    __declspec(dllexport) void
    ModuleStatus(char *MyStatus)
    {
        DiskStatus(MyStatus);
        return ;
    }
}

// Set ini file path and load HD config settings
extern "C"
{
    __declspec(dllexport) void
	SetIniPath (char *IniFilePath)
    {
        strcpy(IniFile,IniFilePath);
        LoadConfig();
        return;
    }
}

extern "C"
{
    __declspec(dllexport) unsigned char
    ModuleReset(void)
    {
        VhdReset();
        return 0;
    }
}

/*
void CPUAssertInterupt(unsigned char Interupt,unsigned char Latencey)
{
    AssertInt(Interupt,Latencey);
    return;
}
*/

// Get filename from user and mount harddrive
void LoadHardDisk(int drive)
{
    OPENFILENAME ofn ;
    char * FileName;

    // Select drive
    switch (drive) {
    case 0:
        FileName = VHDfile0;
        break;
    case 1:
        FileName = VHDfile1;
        break;
    default:
        return;
    }

    // Prompt user for vhd filename
    memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize     = sizeof (OPENFILENAME) ;
    ofn.hwndOwner       = NULL;
    ofn.lpstrFilter     = "HardDisk Images\0*.vhd\0\0"; // filter VHD images
    ofn.nFilterIndex    = 1 ;                           // current filter index
    ofn.lpstrFile       = FileName;                     // full filename on return
    ofn.nMaxFile        = MAX_PATH;                     // sizeof lpstrFile
    ofn.lpstrFileTitle  = NULL;                         // filename only
    ofn.nMaxFileTitle   = MAX_PATH ;                    // sizeof lpstrFileTitle
    ofn.lpstrInitialDir = HardDiskPath;                 // vhd directory
    ofn.lpstrTitle      = TEXT("Load HardDisk Image") ; // title bar string
    ofn.Flags           = OFN_HIDEREADONLY;

    if ( GetOpenFileName (&ofn)) {
        // MountHD is defined in cc3vhd
        if (MountHD(FileName,drive)==0) {
            MessageBox(NULL,"Can't open file","Error",0);
        }
    }

    //  Keep vhd directory for config file
    string tmp = ofn.lpstrFile;
    int idx;
    idx = tmp.find_last_of("\\");
    tmp = tmp.substr(0, idx);
    strcpy(HardDiskPath, tmp.c_str());

    return;
}

// Get configuration items from ini file
void LoadConfig(void)
{
    char ModName[MAX_LOADSTRING]="";
    char DiskRomPath[MAX_PATH];
    HANDLE hr;

    GetPrivateProfileString("DefaultPaths", "HardDiskPath", "",
                             HardDiskPath, MAX_PATH, IniFile);

    // Determine module name for config lookups
    LoadString(g_hinstDLL,IDS_MODULE_NAME, ModName, MAX_LOADSTRING);

    // Verify HD0 image file exists and mount it.
    GetPrivateProfileString(ModName,"VHDImage" ,"",VHDfile0,MAX_PATH,IniFile);
    CheckPath(VHDfile0);
    hr = CreateFile (VHDfile0,NULL,FILE_SHARE_READ,NULL,
                     OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
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
    hr = CreateFile (VHDfile1,NULL,FILE_SHARE_READ,NULL,
                     OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (hr==INVALID_HANDLE_VALUE) {
        strcpy(VHDfile1,"");
        WritePrivateProfileString(ModName,"VHDImage1","",IniFile);
    } else {
        CloseHandle(hr);
        MountHD(VHDfile1,1);
    }

    // Create config menu
    BuildDynaMenu();

    // Load rgbdos rom for Hard Disk support
    GetModuleFileName(NULL, DiskRomPath, MAX_PATH);
    PathRemoveFileSpec(DiskRomPath);
    strcat(DiskRomPath, "rgbdos.rom");
    LoadExtRom(DiskRomPath);

    return;
}

// Save config saves the hard disk path and vhd file names
void SaveConfig(void)
{
    char ModName[MAX_LOADSTRING]="";
    LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);

    ValidatePath(VHDfile0);
    if (HardDiskPath != "") {
        WritePrivateProfileString
            ("DefaultPaths", "HardDiskPath", HardDiskPath, IniFile);
    }
    WritePrivateProfileString(ModName,"VHDImage",VHDfile0 ,IniFile);
    WritePrivateProfileString(ModName,"VHDImage1",VHDfile1 ,IniFile);

    return;
}

// Generate menu for mounting the drives
void BuildDynaMenu(void)
{
    char TempMsg[512]="";
    char TempBuf[MAX_PATH]="";

    DynamicMenuCallback("",0,0);
    DynamicMenuCallback( "",6000,0);

    DynamicMenuCallback( "HD Drive 0",6000,HEAD);
    DynamicMenuCallback( "Insert",5010,SLAVE);
    strcpy(TempMsg,"Eject: ");
    strcpy(TempBuf,VHDfile0);
    PathStripPath (TempBuf);
    strcat(TempMsg,TempBuf);
    DynamicMenuCallback(TempMsg,5011,SLAVE);

    DynamicMenuCallback( "HD Drive 1",6000,HEAD);
    DynamicMenuCallback( "Insert",5012,SLAVE);
    strcpy(TempMsg,"Eject: ");
    strcpy(TempBuf,VHDfile1);
    PathStripPath(TempBuf);
    strcat(TempMsg,TempBuf);
    DynamicMenuCallback(TempMsg,5013,SLAVE);

//  DynamicMenuCallback( "HD Config",5014,STANDALONE);
    DynamicMenuCallback("",1,0);
}

// Load the disk rom
unsigned char LoadExtRom( char *FilePath)
{
    FILE *rom_handle=NULL;
    unsigned short index=0;
    unsigned char RetVal=0;

    rom_handle=fopen(FilePath,"rb");
    if (rom_handle==NULL)
        memset(DiskRom,0xFF,EXTROMSIZE);
    else {
        while ((feof(rom_handle)==0) & (index<EXTROMSIZE))
            DiskRom[index++]=fgetc(rom_handle);
        RetVal=1;
        fclose(rom_handle);
    }
    return(RetVal);
}

