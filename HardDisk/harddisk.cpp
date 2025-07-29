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
#include "defines.h"
#include "cloud9.h"
#include "../fileops.h"
#include "../MachineDefs.h"

#define DEF_HD_SIZE 132480

static char VHDfile0[MAX_PATH] { 0 };
static char VHDfile1[MAX_PATH] { 0 };
static char *VHDfile; // Selected drive file name
static char NewVHDfile[MAX_PATH];
static char IniFile[MAX_PATH]  { 0 };
static char HardDiskPath[MAX_PATH];

typedef unsigned char (*MEMREAD8)(unsigned short);
typedef void (*MEMWRITE8)(unsigned char,unsigned short);
//typedef void (*ASSERTINTERUPT)(InterruptSource, Interrupt);
typedef void (*DMAMEMPOINTERS) ( MEMREAD8,MEMWRITE8);
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
//static void (*AssertInt)(InterruptSource, Interrupt)=NULL;
static unsigned char (*MemRead8)(unsigned short);
static void (*MemWrite8)(unsigned char,unsigned short);
static unsigned char *Memory=NULL;
static void (*DynamicMenuCallback)( char *,int, int)=NULL;
static unsigned char ClockEnabled=1,ClockReadOnly=1;
LRESULT CALLBACK NewDisk(HWND,UINT, WPARAM, LPARAM);

///void Load_Disk(unsigned char);
void LoadHardDisk(int drive);
void LoadConfig(void);
void SaveConfig(void);
void BuildDynaMenu(void);
int CreateDisk(HWND,int);
void CenterDialog(HWND hDlg);

static HINSTANCE g_hinstDLL;
static HWND hConfDlg = NULL;
LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM );

using namespace std;

BOOL WINAPI DllMain( HINSTANCE hinstDLL,  // handle to DLL module
                     DWORD fdwReason,     // reason for calling function
                     LPVOID lpReserved )  // reserved
{
    if (fdwReason == DLL_PROCESS_DETACH ) {
        SendMessage(hConfDlg,WM_CLOSE,0,0);
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

        case 14:
            CreateDialog(g_hinstDLL,(LPCTSTR)IDD_CONFIG,GetActiveWindow(),(DLGPROC)Config);
            ShowWindow(hConfDlg,1);
            return;
        }
        SaveConfig();
        BuildDynaMenu();
        return;
    }
}

void CenterDialog(HWND hDlg)
{
    RECT rPar, rDlg;
    GetWindowRect(GetParent(hDlg), &rPar);
    GetWindowRect(hDlg, &rDlg);
    int x = rPar.left + (rPar.right - rPar.left - (rDlg.right - rDlg.left)) / 2;
    int y = rPar.top + (rPar.bottom - rPar.top - (rDlg.bottom - rDlg.top)) / 2;
    SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        EndDialog(hDlg,LOWORD(wParam));
        DestroyWindow(hDlg);
        return TRUE;
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        hConfDlg=NULL;
        return TRUE;
        break;

    case WM_INITDIALOG:
        hConfDlg=hDlg;
        CenterDialog(hDlg);
        SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnabled,0);
        SendDlgItemMessage(hDlg,IDC_READONLY,BM_SETCHECK,ClockReadOnly,0);
        EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), TRUE);
        return TRUE;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            ClockEnabled=(unsigned char)SendDlgItemMessage(hDlg,IDC_CLOCK,BM_GETCHECK,0,0);
            ClockReadOnly=(unsigned char)SendDlgItemMessage(hDlg,IDC_READONLY,BM_GETCHECK,0,0);
            SetClockWrite(!ClockReadOnly);
            SaveConfig();
            EndDialog(hDlg, LOWORD(wParam));
            break;

        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
        break;
        }
    }
    return(0);
}

/*
// This captures the Fuction transfer point for the CPU assert interupt
// NOTE: Vcc does not do interupts after DMA transfers
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
        VhdReset(); // Selects drive zero
        return;
    }
}

// Hook to read disk rom
/* extern "C"
{
    __declspec(dllexport) unsigned char
    PakMemRead8(unsigned short Address)
    {
        return(DiskRom[Address & 8191]);
    }
}

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

//extern "C"
//{
//    __declspec(dllexport) unsigned char
//    ModuleReset(void)
//    {
//        VhdReset();
//        return 0;
//    }
//}

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

    strncpy(NewVHDfile,VHDfile,MAX_PATH);
    HWND hWnd = GetActiveWindow();

    // Prompt user for vhd filename
    memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize     = sizeof (OPENFILENAME) ;
    ofn.hwndOwner       = hWnd;
    // Allow hard disk images *.img;*.vhd;*.os9 or All Files
    ofn.lpstrFilter     = "Hard Disk Images\0*.img;*.vhd;*.os9\0All files\0*.*\0\0";
    ofn.nFilterIndex    = 1 ;                           // current filter index
    ofn.lpstrFile       = NewVHDfile;                   // full filename on return
    ofn.nMaxFile        = MAX_PATH;                     // sizeof lpstrFile
    ofn.lpstrFileTitle  = NULL;                         // filename only
    ofn.nMaxFileTitle   = MAX_PATH ;                    // sizeof lpstrFileTitle
    ofn.lpstrInitialDir = HardDiskPath;                 // vhd directory
    ofn.lpstrTitle      = TEXT("Load HardDisk Image") ; // title bar string
    ofn.Flags           = OFN_HIDEREADONLY;

    if (GetOpenFileName (&ofn)) {

        // Append .vhd file type if type missing
        if (ofn.nFileExtension==0) strncat(NewVHDfile,".vhd",MAX_PATH);

        // Present new disk dialog if file does not exist
        if (GetFileAttributes(NewVHDfile) == INVALID_FILE_ATTRIBUTES) {
            // Dialog box returns zero if file is not created
            if (DialogBox(g_hinstDLL,(LPCTSTR)IDD_NEWDISK,hWnd,(DLGPROC)NewDisk)==0)
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
        string tmp = ofn.lpstrFile;
        int idx;
        idx = tmp.find_last_of("\\");
        tmp = tmp.substr(0, idx);
        strcpy(HardDiskPath, tmp.c_str());
        SaveConfig();
    }
    return;
}

// Get configuration items from ini file
void LoadConfig(void)
{
    char ModName[MAX_LOADSTRING]="";
    // char DiskRomPath[MAX_PATH];
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

    ClockEnabled=GetPrivateProfileInt(ModName,"ClkEnable",1,IniFile);
    ClockReadOnly=GetPrivateProfileInt(ModName,"ClkRdOnly",1,IniFile);

    // Create config menu
    BuildDynaMenu();
    return;
}

// Save config saves the hard disk path and vhd file names
void SaveConfig(void)
{
    char ModName[MAX_LOADSTRING]="";
    LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);

    ValidatePath(VHDfile0);
    ValidatePath(VHDfile1);
    if (strcmp(HardDiskPath, "") != 0) {
        WritePrivateProfileString
            ("DefaultPaths", "HardDiskPath", HardDiskPath, IniFile);
    }
    WritePrivateProfileString(ModName,"VHDImage",VHDfile0 ,IniFile);
    WritePrivateProfileString(ModName,"VHDImage1",VHDfile1 ,IniFile);
    WritePrivateProfileInt(ModName,"ClkEnable",ClockEnabled ,IniFile);
    WritePrivateProfileInt(ModName,"ClkRdOnly",ClockReadOnly ,IniFile);
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

    DynamicMenuCallback( "HD Config",5014,STANDALONE);

    DynamicMenuCallback("",1,0);
}


// Dialog for creating a new hard disk
LRESULT CALLBACK NewDisk(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    unsigned int hdsize=DEF_HD_SIZE;
    switch (message)
    {
        case WM_INITDIALOG:
            SetDlgItemInt(hDlg,IDC_HDSIZE,DEF_HD_SIZE,0);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case IDOK:
                hdsize=GetDlgItemInt(hDlg,IDC_HDSIZE,NULL,0);
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
                          0,0,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
    if (hr==INVALID_HANDLE_VALUE) {
        *NewVHDfile='\0';
        MessageBox(hDlg,"Can't create File","Error",0);
        return 0;
    }

    if (hdsize>0) {
        SetFilePointer(hr, hdsize * 1024, 0, FILE_BEGIN);
        SetEndOfFile(hr);
    }

    CloseHandle(hr);
    return 1;
}
