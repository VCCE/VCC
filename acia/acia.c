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
//------------------------------------------------------------------

#include "acia.h"
#include "sc6551.h"
#include "logger.h"

//------------------------------------------------------------------------
// Local Functions
//------------------------------------------------------------------------

// Transfer points for menu callback and cpu assert interrupt
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);

void (*DynamicMenuCallback)(char *,int,int)=NULL;
void BuildDynaMenu(void);

LRESULT CALLBACK ConfigDlg(HWND, UINT, WPARAM, LPARAM);
void LoadConfig(void);
void SaveConfig(void);

//------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------
static HINSTANCE g_hDLL = NULL;      // DLL handle
static HWND hConfigDlg = NULL;       // Config dialog
static char IniFile[MAX_PATH];       // Ini file name
static char IniSect[MAX_LOADSTRING]; // Ini file section

// Some strings for config dialog
char *IOModeTxt[] = {"RW","R","W"};
char *ComTypeTxt[] = {"CONS","FILE","TCP","COM"};

static unsigned char Rom[8191];
static void (*PakSetCart)(unsigned char)=NULL;
unsigned char LoadExtRom(char *);

typedef void (*SETCART)(unsigned char);
typedef void (*SETCARTPOINTER)(SETCART);
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);

//------------------------------------------------------------------------
//  DLL Entry point
//------------------------------------------------------------------------
BOOL APIENTRY
DllMain(HINSTANCE hinst, DWORD reason, LPVOID foo)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_hDLL = hinst;

    } else if (reason == DLL_PROCESS_DETACH) {
        if (hConfigDlg) SendMessage(hConfigDlg,WM_CLOSE,6666,0);
        sc6551_close();
        AciaStat[0]='\0';
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
    sc6551_init();
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
// Dll export module reset
//-----------------------------------------------------------------------
__declspec(dllexport) void ModuleReset(void)
{
    LoadExtRom("RS232.ROM");
    SendMessage(hConfigDlg, WM_CLOSE, 0, 0);
    sc6551_close();
    return;
}

//-----------------------------------------------------------------------
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
        char *p = Rom;
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

__declspec(dllexport) void HeartBeat(void)
{
    sc6551_heartbeat();
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
    DialogBox(g_hDLL, (LPCTSTR) IDD_PROPPAGE, NULL, (DLGPROC) ConfigDlg);
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
            sprintf(AciaStat,"Acia file R");
        else
            sprintf(AciaStat,"Acia file W");
        break;
    case COM_TCPIP:
        sprintf(AciaStat,"Acia tcpip");
        break;
    case COM_WINCOM:
        sprintf(AciaStat,"Acia %s",AciaComPort);
        break;
    case COM_CONSOLE:
    default:
        sprintf(AciaStat,"Acia Console");
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

LRESULT CALLBACK ConfigDlg(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
    int button;
    HANDLE hCtl;
    switch (msg) {

    case WM_CLOSE:
        EndDialog(hDlg,0);
        break;

    case WM_INITDIALOG:

        // Kill previous instance
        if (hConfigDlg) EndDialog(hConfigDlg,0);
        hConfigDlg = hDlg;

        SetWindowPos(hDlg, HWND_TOP, 10, 10, 0, 0, SWP_NOSIZE);

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
        break;

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
                int port = GetDlgItemInt(hDlg,IDC_PORT,NULL,0);
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

            SaveConfig();

            if (button==IDOK) EndDialog(hDlg,button);
            break;

        case IDHELP:
            break;

        case IDCANCEL:
            EndDialog(hDlg, button);
            break;
        }
        return TRUE;
    }
    return FALSE;
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
