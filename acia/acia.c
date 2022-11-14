/*
Copyright E J Jaquay 2022
This file is part of VCC (Virtual Color Computer).
    VCC (Virtual Color Computer) is free software: you can redistribute it
    and/or modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

#include "acia.h"

//------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------

// Transfer points for menu callback and cpu assert interrupt
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);

void (*DynamicMenuCallback)(char *,int,int)=NULL;
void BuildDynaMenu(void);

LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);
void LoadConfig(void);
void SaveConfig(void);

//------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------
static HINSTANCE g_hDLL = NULL;      // DLL handle
static char IniFile[MAX_PATH];       // Ini file name
static char IniSect[MAX_LOADSTRING]; // Ini file section

//------------------------------------------------------------------------
//  DLL Entry point
//------------------------------------------------------------------------
BOOL APIENTRY
DllMain(HINSTANCE hinst, DWORD reason, LPVOID foo)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_hDLL = hinst;
        sc6551_initialized = 0;
        AciaStat[0]='\0';
    } else if (reason == DLL_PROCESS_DETACH) {
        sc6551_close();
    }
    return TRUE;
}

//-----------------------------------------------------------------------
//  Dll export register the DLL and build entry on dynamic menu
//-----------------------------------------------------------------------
__declspec(dllexport) void
ModuleName(char *ModName,char *CatNumber,DYNAMICMENUCALLBACK Temp)
{
    LoadString(g_hDLL,IDS_MODULE_NAME,ModName,MAX_LOADSTRING);
    LoadString(g_hDLL,IDS_CATNUMBER,CatNumber,MAX_LOADSTRING);
    DynamicMenuCallback = Temp;
    strcpy(IniSect,ModName);   // Use module name for ini file section
    if (DynamicMenuCallback != NULL) BuildDynaMenu();
    return ;
}

//-----------------------------------------------------------------------
// Dll export write to port
//-----------------------------------------------------------------------
__declspec(dllexport) void
PackPortWrite(unsigned char Port,unsigned char Data)
{
    sc6551_write(Data,Port);
    return;
}

//-----------------------------------------------------------------------
// Dll export read from port
//-----------------------------------------------------------------------
__declspec(dllexport) unsigned char
PackPortRead(unsigned char Port)
{
    return sc6551_read(Port);
}

//-----------------------------------------------------------------------
// Dll export Heartbeat (Called right after HSYNC) 
//-----------------------------------------------------------------------
__declspec(dllexport) void HeartBeat(void)
{
    sc6551_ping();
	return;
}

//-----------------------------------------------------------------------
// Dll export supply transfer point for interrupt
//-----------------------------------------------------------------------
__declspec(dllexport) void AssertInterupt(ASSERTINTERUPT Dummy)
{
    AssertInt=Dummy;
    return;
}

//-----------------------------------------------------------------------
// Dll export return module status for VCC status line
//-----------------------------------------------------------------------
__declspec(dllexport) void ModuleStatus(char *status)
{
    strncpy (status,AciaStat,16);
    status[16]='\n';
    return;
}

//-----------------------------------------------------------------------
//  Dll export run config dialog
//-----------------------------------------------------------------------
__declspec(dllexport) void ModuleConfig(unsigned char MenuID)
{
    DialogBox(g_hDLL, (LPCTSTR) IDD_PROPPAGE, NULL, (DLGPROC) Config);
    return;
}

//-----------------------------------------------------------------------
// Dll export VCC ini file path and load settings
//-----------------------------------------------------------------------
__declspec(dllexport) void SetIniPath (char *IniFilePath)
{
    strcpy(IniFile,IniFilePath);
    LoadConfig();
    return;
}

//-----------------------------------------------------------------------
//  Add config option to Cartridge menu
//----------------------------------------------------------------------
void BuildDynaMenu(void)
{
    DynamicMenuCallback("",0,0);     // begin
    DynamicMenuCallback("",6000,0);  // seperator
    DynamicMenuCallback("ACIA Config",5016,STANDALONE); // Config
    DynamicMenuCallback("",1,0);     // end
}

//-----------------------------------------------------------------------
//  Load saved config from ini file
//----------------------------------------------------------------------
void LoadConfig(void)
{
    AciaComType = GetPrivateProfileInt(IniSect,"AciaComType",0,IniFile);
    AciaTcpPort = GetPrivateProfileInt(IniSect,"AciaTcpPort",1024,IniFile);
    AciaComPort = GetPrivateProfileInt(IniSect,"AciaComPort",1,IniFile);
    sprintf(AciaStat,"Acia %d %d",AciaComType,AciaComPort);
}

//-----------------------------------------------------------------------
//  Save config to ini file
//----------------------------------------------------------------------
void SaveConfig(void)
{
    char txt[16];
    sprintf(AciaStat,"Acia %d %d",AciaComType,AciaComPort);
    sprintf(txt,"%d",AciaComType);
    WritePrivateProfileString(IniSect,"AciaComType",txt,IniFile);
    sprintf(txt,"%d",AciaTcpPort);
    WritePrivateProfileString(IniSect,"AciaTcpPort",txt,IniFile);
    sprintf(txt,"%d",AciaComPort);
    WritePrivateProfileString(IniSect,"AciaComPort",txt,IniFile);
}

//-----------------------------------------------------------------------
//  Config dialog. Allows user to select Console, TCP, COMx, port
//-----------------------------------------------------------------------
LRESULT CALLBACK Config(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
    switch (msg) {

    case WM_INITDIALOG:

        // Center dialog on screen
        RECT rDlg, rScr;
        GetWindowRect(hDlg, &rDlg);
        GetWindowRect(GetDesktopWindow(), &rScr);
        DWORD x = (rScr.right - rScr.left - rDlg.right + rDlg.left)/2;
        DWORD y = (rScr.bottom - rScr.top - rDlg.bottom + rDlg.top)/2;
        SetWindowPos(hDlg, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);

        // Fill in current values
        SetDlgItemInt(hDlg,IDC_TYPE,AciaComType,FALSE);
        switch (AciaComType) {
        case 0: // console
            SetDlgItemInt(hDlg,IDC_PORT,0,FALSE);
            break;
        case 1: // tcpip
            SetDlgItemInt(hDlg,IDC_PORT,AciaTcpPort,FALSE);
            break;
        case 2: // COMx
            SetDlgItemInt(hDlg,IDC_PORT,AciaComPort,FALSE);
            break;
        }
        return TRUE;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            int type = GetDlgItemInt(hDlg,IDC_TYPE,NULL,FALSE);
            int port = GetDlgItemInt(hDlg,IDC_PORT,NULL,FALSE);
            switch (type) {
            case 0: // console
                break;
            case 1: // tcpip
                if ((port < 1024) || (port > 65536)) {
                    MessageBox(hDlg,"TCP Port must be 1024 thru 65536","Error",
                                MB_OK | MB_ICONEXCLAMATION);
                    return TRUE;
                }
                AciaTcpPort = port;
                break;
            case 2: // COMx
                if ((port < 1) || (port > 10)) {
                    MessageBox(hDlg,"COM# must be 1 thru 10","Error",
                                MB_OK | MB_ICONEXCLAMATION);
                    return TRUE;
                }
                AciaComPort = port;
                break;
            default:
                MessageBox(hDlg,"Type must be 0, 1, or 2","Error",
                           MB_OK | MB_ICONEXCLAMATION);
                return TRUE;
            }
            AciaComType = type;
            SaveConfig();
            EndDialog(hDlg, LOWORD(wParam));
            break;

        case IDHELP:
            break;

        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            break;
        }
        return TRUE;
    }
    return FALSE;
}
