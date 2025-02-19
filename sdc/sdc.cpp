//----------------------------------------------------------------------
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
//----------------------------------------------------------------------

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include "../defines.h"
#include "../fileops.h"
#include "../logger.h"
#include "resource.h"
#include "sdcavr.h"

#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

static char IniFile[MAX_PATH]={0};  // Ini file name from config
static char SDCard[MAX_PATH]={0};   // Path to SD card contents
static HINSTANCE hinstDLL;          // DLL handle
static HWND hConfDlg = NULL;        // Config dialog

typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);

static void (*AssertInt)(unsigned char,unsigned char)=NULL;
static void (*DynamicMenuCallback)( char *,int, int)=NULL;

LRESULT CALLBACK SDC_Config(HWND, UINT, WPARAM, LPARAM);

void LoadConfig(void);
void SaveConfig(void);
void BuildDynaMenu(void);
void SDC_Load_Card(void);
unsigned char PakRom[0x4000];

using namespace std;

//------------------------------------------------------------
// DLL entry point
//------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID rsvd)
{

    if (reason == DLL_PROCESS_ATTACH) {
        hinstDLL = hinst;

    } else if (reason == DLL_PROCESS_DETACH) {
        if (hConfDlg) {
            DestroyWindow(hConfDlg);
            hConfDlg = NULL;
        }
        hinstDLL = NULL;
        SDCReset();
    }
    return TRUE;
}
//------------------------------------------------------------
// Register the DLL and build menu
//------------------------------------------------------------
extern "C"
{
    __declspec(dllexport) void
    ModuleName(char *ModName,char *CatNumber,DYNAMICMENUCALLBACK Temp)
    {
        LoadString(hinstDLL, IDS_MODULE_NAME, ModName, MAX_LOADSTRING);
        LoadString(hinstDLL, IDS_CATNUMBER, CatNumber, MAX_LOADSTRING);
        DynamicMenuCallback = Temp;
        if (DynamicMenuCallback != NULL) BuildDynaMenu();
        return ;
    }
}
//------------------------------------------------------------
// Write to port
//------------------------------------------------------------
extern "C"
{
    __declspec(dllexport) void
    PackPortWrite(unsigned char Port,unsigned char Data)
    {
        SDCWrite(Data,Port);
        return;
    }
}
//------------------------------------------------------------
// Read from port
//------------------------------------------------------------
extern "C"
{
    __declspec(dllexport) unsigned char PackPortRead(unsigned char Port)
    {
        return(SDCRead(Port));
    }
}
//------------------------------------------------------------
// Reset module
//------------------------------------------------------------
extern "C"
{
    __declspec(dllexport) unsigned char
    ModuleReset(void)
    {        //SDCReset();
        return 0;
    }
}
//-------------------------------------------------------------------
//  Dll export run config dialog
//-------------------------------------------------------------------
extern "C"
{
    __declspec(dllexport) void ModuleConfig(unsigned char MenuID)
    {
    switch (MenuID)
        {
        case 10:
            SDC_Load_Card();
            break;
        case 20:
            CreateDialog(hinstDLL, (LPCTSTR) IDD_CONFIG,
                         GetActiveWindow(), (DLGPROC) SDC_Config);
            ShowWindow(hConfDlg,1);
            return;
        }
        BuildDynaMenu();
        return;
    }
}
//------------------------------------------------------------
// Configure the SDC
//------------------------------------------------------------
LRESULT CALLBACK
SDC_Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CLOSE:
        EndDialog(hDlg,LOWORD(wParam));
    case WM_INITDIALOG:
        hConfDlg=hDlg;
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            EndDialog(hDlg,LOWORD(wParam));
            break;
        case IDCANCEL:
            EndDialog(hDlg,LOWORD(wParam));
            break;
        default:
            break;
        }
    }
    return (INT_PTR) 0;
}
//------------------------------------------------------------
// Capture the Fuction transfer point for the CPU assert interupt
//------------------------------------------------------------
extern "C"
{
    __declspec(dllexport)
    void AssertInterupt(ASSERTINTERUPT Dummy)
    {
        AssertInt=Dummy;
        return;
    }
}
//------------------------------------------------------------
// Return SDC status.
//------------------------------------------------------------
extern "C"
{
    __declspec(dllexport) void
    ModuleStatus(char *MyStatus)
    {
        strcpy(MyStatus,"SDC");
        return ;
    }
}
//------------------------------------------------------------
// Set ini file path and load HD config settings
//------------------------------------------------------------
extern "C"
{
    __declspec(dllexport) void
    SetIniPath (char *IniFilePath)
    {
        strcpy(IniFile,IniFilePath);
        LoadConfig();
        SDCInit();
        return;
    }
}
//------------------------------------------------------------
// Return a byte from the current PAK ROM
//------------------------------------------------------------
extern "C"
{
	__declspec(dllexport) unsigned char
    PakMemRead8(unsigned short adr)
	{
		return(PakRom[adr & 0x3ffff]);
	}
}
//------------------------------------------------------------
//   Assert Interupt
//------------------------------------------------------------
/*
void CPUAssertInterupt(unsigned char Interupt,unsigned char Latencey)
{
    AssertInt(Interupt,Latencey);
    return;
}
*/
//-------------------------------------------------------------
// Generate menu for configuring the SD
//-------------------------------------------------------------
void BuildDynaMenu(void)
{
    char label[MAX_PATH];
    strncpy(label,"SD Card: ",40);

    // Maybe better set/show card path in config.
    char tmp[MAX_PATH];
    PathCompactPathEx(tmp,SDCard,20,0);
    strncat(label,tmp,MAX_PATH);

    //Call back type 0=Head 1=Slave 2=StandAlone
    DynamicMenuCallback("",0,0);
    DynamicMenuCallback("",6000,0);
    DynamicMenuCallback(label,5010,2);
    DynamicMenuCallback("SDC Config",5020,2);
    DynamicMenuCallback("",1,0);
}
//------------------------------------------------------------
// Get SDC pathname from user
//------------------------------------------------------------
void SDC_Load_Card(void)
{
    // Prompt user for path
    BROWSEINFO bi = { 0 };
    bi.hwndOwner = GetActiveWindow();
    bi.lpszTitle = "Set the SD card path";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NONEWFOLDERBUTTON;
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != 0) {
        SHGetPathFromIDList(pidl,SDCard);
        CoTaskMemFree(pidl);
    }
    // Sanitize slashes
    for(unsigned int i=0; i<strlen(SDCard); i++)
        if (SDCard[i] == '\\') SDCard[i] = '/';

    SetSDRoot(SDCard);
    SaveConfig();
    return;
}
//------------------------------------------------------------
// Get configuration items from ini file
//------------------------------------------------------------
void LoadConfig(void)
{
    GetPrivateProfileString("SDC", "SDCardPath", "",
                             SDCard, MAX_PATH, IniFile);
    SetSDRoot(SDCard);
    // Create config menu
    BuildDynaMenu();
    return;
}

//------------------------------------------------------------
// Save config to ini file
//------------------------------------------------------------
void SaveConfig(void)
{
    WritePrivateProfileString("SDC", "SDCardPath",SDCard,IniFile);
    return;
}

//-------------------------------------------------------------
// Load SDC rom
//-------------------------------------------------------------

bool LoadRom(char *RomName)	//Returns true if loaded
{
    int ch;
    int ctr = 0;
	FILE *h = fopen(RomName, "rb");
	memset(PakRom, 0xFF, 0x4000);
	if (h == NULL) return false;

    unsigned char *p = PakRom;
    while (ctr++ < 0x4000) {
        if ((ch = fgetc(h)) < 0) break;
        *p++ = (unsigned char) ch;
    }

	fclose(h);
	return true;
}
