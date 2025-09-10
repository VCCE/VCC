//------------------------------------------------------------------
// Copyright E J Jaquay 2022
//
// This file is part of VCC (Virtual Color Computer).
//
// VCC (Virtual Color Computer) is free software: you can redistribute it
// and/or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.  You should have
// received a copy of the GNU General Public License  along with VCC
// (Virtual Color Computer). If not see <http://www.gnu.org/licenses/>.
//
// NOTE: Due to changes in interupt handling this version will not work
// properly before VCC 2.1.9.2 (Console mode Nitros9 using /t2 broken)
//
//------------------------------------------------------------------

#include "acia.h"
#include "../DialogOps.h"
#include "../logger.h"

//------------------------------------------------------------------------
// Local Functions
//------------------------------------------------------------------------

// FIXME: These typedefs are duplicated across more if not all projects and
// need to be consolidated in one place.
// Transfer points for menu callback and cpu assert interrupt
typedef void (*DYNAMICMENUCALLBACK)(const char * ,int, int);
typedef void (*ASSERTINTERUPT)(unsigned char, unsigned char);

void BuildDynaMenu(void);

LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);
void LoadConfig(void);
void SaveConfig(void);
void CenterDialog(HWND);

//------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------
static HINSTANCE g_hDLL = nullptr;      // DLL handle
static HWND g_hDlg = nullptr;           // Config dialog
static char IniFile[MAX_PATH];       // Ini file name
static char IniSect[MAX_LOADSTRING]; // Ini file section
static DYNAMICMENUCALLBACK DynamicMenuCallback = nullptr;

// Status for Vcc status line
char AciaStat[32];

// Some strings for config dialog
char *IOModeTxt[] = {"RW","R","W"};
char *ComTypeTxt[] = {"CONS","FILE","TCP","COM"};

int  AciaBasePort;             // Base port for sc6651 (0x68 or 0x6C)
int  AciaComType;              // Console,file,tcpip,wincom
int  AciaComMode;              // Duplex,read,write
int  AciaTextMode;             // CR and EOF translations 0=none 1=text
int  AciaLineInput;            // Console line mode 0=Normal 1=Linemode
int  AciaTcpPort;              // TCP port 1024-65536
char AciaComPort[32];          // Windows Serial port eg COM20
char AciaTcpHost[MAX_PATH];    // Tcpip hostname
char AciaFileRdPath[MAX_PATH]; // Path for file reads
char AciaFileWrPath[MAX_PATH]; // Path for file writes

static unsigned char Rom[8192];
void (*AssertInt)(unsigned char, unsigned char);
unsigned char LoadExtRom(char *);

//------------------------------------------------------------------------
//  DLL Entry point
//------------------------------------------------------------------------
BOOL APIENTRY
DllMain(HINSTANCE hinst, DWORD reason, LPVOID foo)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_hDLL = hinst;
        LoadExtRom("RS232.ROM");

    } else if (reason == DLL_PROCESS_DETACH) {
        sc6551_close();
        AciaStat[0]='\0';
        CloseCartDialog(g_hDlg);
    }
    return TRUE;
}

//-----------------------------------------------------------------------
//  Dll export register the DLL and build entry on dynamic menu
//-----------------------------------------------------------------------
extern "C"
__declspec(dllexport) void
ModuleName(char *ModName,char *CatNumber,DYNAMICMENUCALLBACK Temp)
{
    LoadString(g_hDLL,IDS_MODULE_NAME,ModName,MAX_LOADSTRING);
    LoadString(g_hDLL,IDS_CATNUMBER,CatNumber,MAX_LOADSTRING);
    DynamicMenuCallback = Temp;
    strcpy(IniSect,ModName);   // Use module name for ini file section
    if (DynamicMenuCallback != nullptr) BuildDynaMenu();
    sc6551_init();
    return ;
}

//-----------------------------------------------------------------------
// Dll export write to port
//-----------------------------------------------------------------------
extern "C"
__declspec(dllexport) void
PackPortWrite(unsigned char Port,unsigned char Data)
{
    sc6551_write(Data,Port);
    return;
}

//-----------------------------------------------------------------------
// Dll export read from port
//-----------------------------------------------------------------------
extern "C"
__declspec(dllexport) unsigned char
PackPortRead(unsigned char Port)
{
    return sc6551_read(Port);
}

//-----------------------------------------------------------------------
// Dll export module reset
//-----------------------------------------------------------------------
extern "C"
__declspec(dllexport) void ModuleReset(void)
{
    sc6551_close();
    return;
}

//-----------------------------------------------------------------------
// Dll export pak rom read
//-----------------------------------------------------------------------
extern "C"
__declspec(dllexport) unsigned char PakMemRead8(unsigned short Address)
{
    return(Rom[Address & 8191]);
}

//-----------------------------------------------------------------------
// Load Rom. Returns 1 if loaded
//-----------------------------------------------------------------------
unsigned char LoadExtRom(char *FilePath)
{
    FILE *file;
    int cnt = 0;
    memset(Rom, 0xff, 8191);
    file = fopen(FilePath, "rb");
    if (file) {
        unsigned char *p = Rom;
        while (cnt++ < 4096) {
            if (feof(file)) break;
            *p++ = fgetc(file);
        }
        fclose(file);
        return 1;
    }
    return 0;
}

//-----------------------------------------------------------------------
// Dll export Heartbeat (HSYNC)
//-----------------------------------------------------------------------
extern "C"
__declspec(dllexport) void HeartBeat(void)
{
    sc6551_heartbeat();
    return;
}

//-----------------------------------------------------------------------
// Dll export supply transfer point for interrupt
//-----------------------------------------------------------------------
extern "C"
__declspec(dllexport) void AssertInterupt(ASSERTINTERUPT Dummy)
{
    AssertInt=Dummy;
    return;
}

//-----------------------------------------------------------------------
// Dll export return module status for VCC status line
//-----------------------------------------------------------------------
extern "C"
__declspec(dllexport) void ModuleStatus(char *status)
{
    strncpy (status,AciaStat,16);
    status[16]='\n';
    return;
}

//-----------------------------------------------------------------------
//  Dll export run config dialog
//-----------------------------------------------------------------------
extern "C"
__declspec(dllexport) void ModuleConfig(unsigned char /*MenuID*/)
{
    HWND owner = GetActiveWindow();
    CreateDialog(g_hDLL,(LPCTSTR)IDD_PROPPAGE,owner,(DLGPROC)Config);
    ShowWindow(g_hDlg,1);
    return;
}

//-----------------------------------------------------------------------
// Dll export VCC ini file path and load settings
//-----------------------------------------------------------------------
extern "C"
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
    AciaComType=GetPrivateProfileInt("Acia","AciaComType",
                                     COM_CONSOLE,IniFile);
    AciaComMode=GetPrivateProfileInt("Acia","AciaComMode",
                                     COM_MODE_DUPLEX,IniFile);
    AciaBasePort=GetPrivateProfileInt("Acia","AciaBasePort",
                                     BASE_PORT_RS232,IniFile);
    AciaTcpPort=GetPrivateProfileInt("Acia","AciaTcpPort",48000,IniFile);
    AciaTextMode=GetPrivateProfileInt("Acia","AciaTextMode",0,IniFile);
    GetPrivateProfileString("Acia","AciaComPort","COM3",
                            AciaComPort,32,IniFile);
    GetPrivateProfileString("Acia","AciaFileRdPath","AciaFile.txt",
                            AciaFileRdPath, MAX_PATH,IniFile);
    GetPrivateProfileString("Acia","AciaFileWrPath","AciaFile.txt",
                            AciaFileWrPath, MAX_PATH,IniFile);
    GetPrivateProfileString("Acia","AciaTcpHost","localhost",
                            AciaTcpHost, MAX_PATH,IniFile);

    // String for Vcc status line
    switch (AciaComType) {
    case COM_FILE:
        if (AciaComMode == COM_MODE_READ)
            sprintf(AciaStat,"Acia R");
        else
            sprintf(AciaStat,"Acia W");
        break;
    case COM_TCPIP:
        sprintf(AciaStat,"Acia Net");
        break;
    case COM_WINCOM:
        sprintf(AciaStat,"Acia %s",AciaComPort);
        break;
    case COM_CONSOLE:
    default:
        sprintf(AciaStat,"Acia Cons");
        break;
    }
}

//-----------------------------------------------------------------------
//  Save config to ini file
//----------------------------------------------------------------------
void SaveConfig(void)
{
    char txt[16];
    sprintf(txt,"%d",AciaBasePort);
    WritePrivateProfileString("Acia","AciaBasePort",txt,IniFile);
    sprintf(txt,"%d",AciaComType);
    WritePrivateProfileString("Acia","AciaComType",txt,IniFile);
    sprintf(txt,"%d",AciaComMode);
    WritePrivateProfileString("Acia","AciaComMode",txt,IniFile);
    sprintf(txt,"%d",AciaTcpPort);
    WritePrivateProfileString("Acia","AciaTcpPort",txt,IniFile);
    sprintf(txt,"%d",AciaTextMode);

    WritePrivateProfileString("Acia","AciaTextMode",txt,IniFile);
    WritePrivateProfileString("Acia","AciaComPort",AciaComPort,IniFile);
    WritePrivateProfileString("Acia","AciaFileRdPath",AciaFileRdPath,IniFile);
    WritePrivateProfileString("Acia","AciaFileWrPath",AciaFileWrPath,IniFile);
    WritePrivateProfileString("Acia","AciaTcpHost",AciaTcpHost,IniFile);
}

//-----------------------------------------------------------------------
// Config dialog.
// Radio Buttons: (I/O Type) :
//   IDC_T_CONS    I/O is to windows console
//   IDC_T_FILE_R  Input from file (Output to bit bucket)
//   IDC_T_FILE_W  Output to file, Input returns null
//   IDC_T_TCP     I/O to TCPIP port
//   IDC_T_COM     I/O to windows COM port
// Text Boxes
//   IDC_PORT      Port for TCP and COM Number
//   IDC_NAME      File name for FILE_R and FILE_W Host name for TCPIP
// Check Box
//   IDC_TEXTMODE  Translate CR <> CRLF if checked
//-----------------------------------------------------------------------

LRESULT CALLBACK Config(HWND hDlg,UINT msg,WPARAM wParam,LPARAM /*lParam*/)
{
    int button;
    HWND hCtl;
    int port;
    switch (msg) {

    case WM_CLOSE:
        DestroyWindow(hDlg);
        g_hDlg = nullptr;
        return TRUE;

    case WM_INITDIALOG:

        // Kill previous instance
        DestroyWindow(g_hDlg);
        g_hDlg = hDlg;

        CenterDialog(hDlg);

        // Set Button as per Base Port
        switch (AciaBasePort) {
        case BASE_PORT_RS232:
            CheckDlgButton(hDlg,IDC_T_RS232,BST_CHECKED);
            CheckDlgButton(hDlg,IDC_T_MODEM,BST_UNCHECKED);
            break;
        case BASE_PORT_MODEM:
            CheckDlgButton(hDlg,IDC_T_RS232,BST_UNCHECKED);
            CheckDlgButton(hDlg,IDC_T_MODEM,BST_CHECKED);
            break;
        default:
            CheckDlgButton(hDlg,IDC_T_RS232,BST_CHECKED);
            CheckDlgButton(hDlg,IDC_T_MODEM,BST_UNCHECKED);
            AciaBasePort = BASE_PORT_RS232;
            break;
        }

        // Set Radio Button as per com type
        switch (AciaComType) {

        case COM_FILE:
            EnableWindow(GetDlgItem(hDlg,IDC_PORT),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_NAME),TRUE);
            if (AciaComMode == COM_MODE_READ) {
                SetDlgItemText(hDlg,IDC_NAME,AciaFileRdPath);
                button = IDC_T_FILE_R;
                sprintf(AciaStat,"Acia file R");
            } else {
                // If file mode default to write
                AciaComMode = COM_MODE_WRITE;
                SetDlgItemText(hDlg,IDC_NAME,AciaFileWrPath);
                button = IDC_T_FILE_W;
                sprintf(AciaStat,"Acia file W");
            }
            SetDlgItemText(hDlg,IDC_PORT,"");
            break;

        case COM_TCPIP:
            EnableWindow(GetDlgItem(hDlg,IDC_PORT),TRUE);
            EnableWindow(GetDlgItem(hDlg,IDC_NAME),TRUE);
            button = IDC_T_TCP;
            SetDlgItemText(hDlg,IDC_NAME,AciaTcpHost);
            SetDlgItemInt(hDlg,IDC_PORT,AciaTcpPort,FALSE);
            sprintf(AciaStat,"Acia tcpip");
            break;

        case COM_WINCOM:
            EnableWindow(GetDlgItem(hDlg,IDC_PORT),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_NAME),TRUE);
            button = IDC_T_COM;
            SetDlgItemText(hDlg,IDC_NAME,AciaComPort);
            sprintf(AciaStat,"Acia %s",AciaComPort);
            break;

        case COM_CONSOLE:
        default:
            EnableWindow(GetDlgItem(hDlg,IDC_PORT),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_NAME),FALSE);
            button = IDC_T_CONS;
            AciaComType = COM_CONSOLE;
            SetDlgItemText(hDlg,IDC_NAME,"");
            SetDlgItemText(hDlg,IDC_PORT,"");
            sprintf(AciaStat,"Acia Console");
            break;
        }
        CheckRadioButton(hDlg,IDC_T_CONS,IDC_T_COM,button);

        // Text mode check box
        hCtl = GetDlgItem(hDlg,IDC_TXTMODE);
        SendMessage(hCtl,BM_SETCHECK,AciaTextMode,0);

        return TRUE;

    case WM_COMMAND:

        button = LOWORD(wParam);
        switch (button) {

        // Close sc6551 before changing base port or com type
        case IDC_T_RS232:
            sc6551_close();
            AciaBasePort = BASE_PORT_RS232;
            break;

        case IDC_T_MODEM:
            sc6551_close();
            AciaBasePort = BASE_PORT_MODEM;
            break;

        case IDC_T_CONS:
            EnableWindow(GetDlgItem(hDlg,IDC_PORT),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_NAME),FALSE);
            sc6551_close();
            AciaComType  = COM_CONSOLE;
            AciaComMode = COM_MODE_DUPLEX;
            SetDlgItemText(hDlg,IDC_NAME,"");
            SetDlgItemText(hDlg,IDC_PORT,"");
            break;

        case IDC_T_FILE_R:
            EnableWindow(GetDlgItem(hDlg,IDC_PORT),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_NAME),TRUE);
            sc6551_close();
            AciaComType  = COM_FILE;
            AciaComMode = COM_MODE_READ;
            SetDlgItemText(hDlg,IDC_NAME,AciaFileRdPath);
            SetDlgItemText(hDlg,IDC_PORT,"");
            break;

        case IDC_T_FILE_W:
            EnableWindow(GetDlgItem(hDlg,IDC_PORT),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_NAME),TRUE);
            sc6551_close();
            AciaComType  = COM_FILE;
            AciaComMode = COM_MODE_WRITE;
            SetDlgItemText(hDlg,IDC_NAME,AciaFileWrPath);
            SetDlgItemText(hDlg,IDC_PORT,"");
            break;

        case IDC_T_TCP: // tcpip
            EnableWindow(GetDlgItem(hDlg,IDC_PORT),TRUE);
            EnableWindow(GetDlgItem(hDlg,IDC_NAME),TRUE);
            sc6551_close();
            AciaComType = COM_TCPIP;
            AciaComMode = COM_MODE_DUPLEX;
            SetDlgItemText(hDlg,IDC_NAME,AciaTcpHost);
            SetDlgItemInt(hDlg,IDC_PORT,AciaTcpPort,FALSE);
            break;

        case IDC_T_COM: // COMx
            EnableWindow(GetDlgItem(hDlg,IDC_PORT),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_NAME),TRUE);
            sc6551_close();
            AciaComType = COM_WINCOM;
            AciaComMode = COM_MODE_DUPLEX;
            SetDlgItemText(hDlg,IDC_PORT,"");
            SetDlgItemText(hDlg,IDC_NAME,AciaComPort);
            break;

        case IDOK:
        case IDAPPLY:
            // Text mode check box
            hCtl = GetDlgItem(hDlg,IDC_TXTMODE);
            AciaTextMode = SendMessage(hCtl,BM_GETCHECK,0,0);

            // Validate parameters
            switch (AciaComType) {
            case COM_CONSOLE:
                sprintf(AciaStat,"Acia Console");
                break;
            case COM_FILE:
                if (AciaComMode == COM_MODE_READ) {
                    GetDlgItemText(hDlg,IDC_NAME,AciaFileRdPath,MAX_PATH);
                    sprintf(AciaStat,"Acia file R");
                } else if (AciaComMode == COM_MODE_WRITE) {
                    GetDlgItemText(hDlg,IDC_NAME,AciaFileWrPath,MAX_PATH);
                    sprintf(AciaStat,"Acia file W");
                }
                break;
            case COM_TCPIP:
                GetDlgItemText(hDlg,IDC_NAME,AciaTcpHost,MAX_PATH);
                port = GetDlgItemInt(hDlg,IDC_PORT,nullptr,0);
                if ((port < 1) || (port > 65536)) {
                    MessageBox(hDlg,"TCP Port must be 1 thru 65536",
                                    "Error", MB_OK|MB_ICONEXCLAMATION);
                    return TRUE;
                }
                AciaTcpPort = port;
                sprintf(AciaStat,"Acia tcpip");
                break;
            case COM_WINCOM:
                GetDlgItemText(hDlg,IDC_NAME,AciaComPort,32);
                sprintf(AciaStat,"Acia %s",AciaComPort);
                break;
            }

            // Okay exits the dialog
            if (button==IDOK) {
                SaveConfig();       // Should APPLY also save the config?
                DestroyWindow(hDlg);
                g_hDlg = nullptr;
            }
            break;

        case IDHELP:
            break;

        case IDCANCEL:
            DestroyWindow(hDlg);
            g_hDlg = nullptr;
            break;
        }
        return TRUE;
    }
    return FALSE;
}

//------------------------------------------------------------
// Center a dialog box in parent window
//------------------------------------------------------------
void CenterDialog(HWND hDlg)
{
    RECT rPar, rDlg;
    GetWindowRect(GetParent(hDlg), &rPar);
    GetWindowRect(hDlg, &rDlg);
    int x = rPar.left + (rPar.right - rPar.left - (rDlg.right - rDlg.left)) / 2;
    int y = rPar.top + (rPar.bottom - rPar.top - (rDlg.bottom - rDlg.top)) / 2;
    SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

//----------------------------------------------------------------
// Dispatch I/0 to communication type used.
//-----------------------------------------------------------------

int com_open() {
    switch (AciaComType) {
    case COM_CONSOLE:
        return console_open();
    case COM_FILE:
        return file_open();
    case COM_TCPIP:
        return tcpip_open();
    case COM_WINCOM:
        return wincom_open();
    }
    return 0;
}

void com_close() {
    switch (AciaComType) {
    case COM_CONSOLE:
        console_close();
        break;
    case COM_FILE:
        file_close();
        break;
    case COM_TCPIP:
        tcpip_close();
        break;
    case COM_WINCOM:
        wincom_close();
        break;
    }
}

// com_write will block until some data is written or error
int com_write(char * buf, int len) {
    switch (AciaComType) {
    case COM_CONSOLE:
        return console_write(buf,len);
    case COM_FILE:
        return file_write(buf,len);
    case COM_TCPIP:
        return tcpip_write(buf,len);
    case COM_WINCOM:
        return wincom_write(buf,len);
    }
    return 0;
}

// com_read will block until some data is read or error
int com_read(char * buf,int len) {  // returns bytes read
    switch (AciaComType) {
    case COM_CONSOLE:
        return console_read(buf,len);
    case COM_FILE:
        return file_read(buf,len);
    case COM_TCPIP:
        return tcpip_read(buf,len);
    case COM_WINCOM:
        return wincom_read(buf,len);
    }
    return 0;
}
