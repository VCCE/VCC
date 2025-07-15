// SDC simulator DLL
//
// By E J Jaquay 2025
//
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
//
//----------------------------------------------------------------------
//
//  SDC Floppy port conflicts
//  -------------------------
//  The SDC interface shares ports with the FD502 floppy controller.
//
//  CTRLATCH  $FF40 ; controller latch (write)
//  FLSHDAT   $FF42 ; flash data register
//  FLSHCTRL  $FF43 ; flash control register
//  CMDREG    $FF48 ; command register (write)
//  STATREG   $FF48 ; status register (read)
//  PREG1     $FF49 ; param register 1
//  PREG2     $FF4A ; param register 2
//  PREG3     $FF4B ; param register 3
//
//  Port conflicts are resolved in the MPI by using the SCS (select
//  cart signal) to direct the floppy ports ($FF40-$FF5F) to the
//  selected cartridge slot. Sometime in the past the VCC cart select
//  was disabled in mmi.cpp. This had to be be re-enabled for for sdc
//  to co-exist with FD502. Additionally the becker port (drivewire)
//  uses port $FF41 for status and port $FF42 for data so these must be
//  always alowed for becker.dll to work. This means sdc.dll requires the
//  new version of mmi.dll to work properly.
//
//  NOTE: Stock SDCDOS does not in support the becker ports, it expects
//  drivewire to be on the bitbanger ports. RGBDOS does, however, and will
//  still work with sdc.dll installed.
//
//  MPI control is accomplished by writing $FF7F (65407) multi-pak slot
//  selection. $FF7F bits 1-0 set the SCS slot while bits 5-4 set the
//  CTS/CART slot. Other bits are set if the MPI selector switch is used
//  but these do nothing. SCS selects disk I/O ($FF40-$FF5F) and CTS/CART
//  selects ROM ($C000-$DFFF)
//
//  To select a slot keeping CTS and SCS the same:
//    POKE 65407,0   ($00) for slot 1
//    POKE 65407,17  ($11) for slot 2
//    POKE 65407,34  ($22) for slot 3
//    POLE 65407,51  ($33) for slot 4
//  Vcc will reset if CTS/CART is changed.
//
//  SDC commands
//  ------------
//  A commmand sent to the SDC consists of a single byte followed
//  by 0 to three 3 parameter bytes.  For example Data registers 1-3
//  are used to specify the LSN and Data registers 2-3 contain 16 bit
//  data for disk I/O. Some commands require a 256 byte block of data,
//  typically a null-terminated ASCII command string. File path names
//  must confirm to MS-DOS 8.3 naming.  Forward slashes '/' delimit
//  directories.  All path names are relative, if the first char is
//  a '/' the path is relative to the windows device or directory
//  containing the contents of the SDcard.
//
//  Extended commands are in the data block, example M:<path> mounts a
//  disk. See the SDC User Guide for detailed command descriptions. It
//  is likely that some commands mentioned there are not yet supported
//  by this interface.
//
//  Flash data
//  ----------
//  There are eight 16K banks of programable flash memory. In this
//  simulator the banks are set to files where the ROMs for the
//  bank is stored. When a bank is selected the file is read into ROM.
//  The SDC-DOS RUN@n command can be used to select a bank.
//
//  This simulator has no provision for writing to the banks or the
//  associated files. These are easily managed using the host system.
//
//  Data written to the flash data port (0x42) can be read back.
//  When the flash data port (0x43) is written the three low bits
//  select the ROM from the corresponding flash bank (0-7). When
//  read the flash control port returns the currently selected bank
//  in the three low bits and the five bits from the Flash Data port.
//
//  SDC-DOS is typically in bank zero and disk basic in bank one. These
//  ROMS require their respective .DLL's to be installed to function,
//  these are typically in MMI slot 3 and 4 respectively.
//
//----------------------------------------------------------------------

#include <iostream>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include "sdc.h"

//======================================================================
// Functions
//======================================================================

void LoadRom(unsigned char);
void SDCWrite(unsigned char,unsigned char);
unsigned char SDCRead(unsigned char port);
void SDCInit(void);
void MemWrite(unsigned char,unsigned short);
unsigned char MemRead(unsigned short);
typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
typedef unsigned char (*MEMREAD8)(unsigned short);
typedef void (*MEMWRITE8)(unsigned char,unsigned short);
static void (*DynamicMenuCallback)( char *,int, int)=NULL;
static unsigned char (*MemRead8)(unsigned short)=NULL;
static void (*MemWrite8)(unsigned char,unsigned short)=NULL;
LRESULT CALLBACK SDC_Config(HWND, UINT, WPARAM, LPARAM);

void LoadConfig(void);
bool SaveConfig(HWND);
void BuildDynaMenu(void);
void CenterDialog(HWND);
void SelectCardBox(void);
void UpdateFlashItem(void);
void DeleteFlashItem(void);
void InitCardBox(void);
void InitFlashBox(void);
void ParseStartup(void);
void SDCCommand(void);
void ReadSector(void);
void StreamImage(void);
void WriteSector(void);
bool SeekSector(unsigned char,unsigned int);
bool ReadDrive(unsigned char,unsigned int);
void GetDriveInfo(void);
void SDCControl(void);
void UpdateSD(void);
void AppendPathChar(char *,char c);
bool LoadFindRecord(struct FileRecord *);
bool LoadDotRecord(struct FileRecord *);
void FixSDCPath(char *,const char *);
void MountDisk(int,const char *,int);
void MountNewDisk(int,const char *,int);
bool MountNext(int);
void OpenNew(int,const char *,int);
void CloseDrive(int);
void OpenFound(int,int);
void LoadReply(void *, int);
void BlockReceive(unsigned char);
char * LastErrorTxt(void);
void FlashControl(unsigned char);
void LoadDirPage(void);
void SetCurDir(char * path);
bool SearchFile(const char *);
void InitiateDir(const char *);
void RenameFile(const char *);
bool IsDirectory(const char *);
void GetMountedImageRec(void);
void GetSectorCount(void);
void GetDirectoryLeaf(void);
unsigned char PickReplyByte(unsigned char);
unsigned char WriteFlashBank(unsigned short);


//======================================================================
// Globals
//======================================================================

// Idle Status counter
int idle_ctr = 0;

// SDC CoCo Interface
typedef struct {
    int sdclatch;
    unsigned char cmdcode;
    unsigned char status;
    unsigned char reply1;
    unsigned char reply2;
    unsigned char reply3;
    unsigned char param1;
    unsigned char param2;
    unsigned char param3;
    unsigned char reply_mode;  // 0=words, 1=bytes
    unsigned char reply_status;
    unsigned char half_sent;
    int bufcnt;
    char *bufptr;
    char blkbuf[600];
} Interface;
Interface IF;

// Cart ROM
char PakRom[0x4000];

// Host paths for SDC
char IniFile[MAX_PATH] = {};  // Vcc ini file name
char SDCard[MAX_PATH]  = {};  // SD card root directory
char CurDir[256]       = {};  // SDC current directory
char SeaDir[MAX_PATH]  = {};  // Last directory searched

// Packed file records for SDC file commands
#pragma pack(1)
struct FileRecord {
    char name[8];
    char type[3];
    char attrib;
    char hihi_size;
    char lohi_size;
    char hilo_size;
    char lolo_size;
};
#pragma pack()
struct FileRecord DirPage[16];

// Mounted image data
struct _Disk {
    HANDLE hFile;
    unsigned int size;
    unsigned int headersize;
    DWORD sectorsize;
    DWORD tracksectors;
    char doublesided;
    char name[MAX_PATH];
    struct FileRecord filerec;
} Disk[2];

// Flash banks
char FlashFile[8][MAX_PATH];
FILE *h_RomFile = NULL;
unsigned char StartupBank = 0;
unsigned char CurrentBank = 0xff;
unsigned char EnableBankWrite = 0;
unsigned char BankWriteNum = 0;
unsigned char BankWriteState = 0;
unsigned char BankDirty = 0;
unsigned char BankData = 0;

// Dll handle
static HINSTANCE hinstDLL;

// Clock enable IDC_CLOCK
int ClockEnable;

// Windows file lookup handle and data
HANDLE hFind;
WIN32_FIND_DATAA dFound;

// config control handles
static HWND hConfDlg = NULL;
static HWND hFlashBox = NULL;
static HWND hSDCardBox = NULL;
static HWND hStartupBank = NULL;

// Streaming control
int streaming;
unsigned char stream_cmdcode;
unsigned int stream_lsn;

char Status[16] = {};
bool StartupComplete = false;

//======================================================================
// DLL exports
//======================================================================
extern "C"
{
    // Register the DLL and build menu
    __declspec(dllexport) void ModuleName
        (char *ModName,char *CatNumber,DYNAMICMENUCALLBACK Temp)
    {
        LoadString(hinstDLL, IDS_MODULE_NAME, ModName, MAX_LOADSTRING);
        LoadString(hinstDLL, IDS_CATNUMBER, CatNumber, MAX_LOADSTRING);
        DynamicMenuCallback = Temp;
        if (DynamicMenuCallback != NULL) BuildDynaMenu();
        return;
    }

    // Write to port
    __declspec(dllexport) void PackPortWrite
        (unsigned char Port,unsigned char Data)
    {
        SDCWrite(Data,Port);
        return;
    }

    // Read from port
    __declspec(dllexport) unsigned char PackPortRead(unsigned char Port)
    {
        if (ClockEnable && ((Port==0x78) | (Port==0x79) | (Port==0x7C))) {
            return ReadTime(Port);
        } else {
            return SDCRead(Port);
        }
    }

    // Reset module
    __declspec(dllexport) unsigned char ModuleReset(void)
    {
        _DLOG("ModuleReset\n");
        SDCInit();
        return 0;
    }

    //  Dll export run config dialog
    __declspec(dllexport) void ModuleConfig(unsigned char MenuID)
    {
        switch (MenuID)
        {
        case 20:
            CreateDialog(hinstDLL, (LPCTSTR) IDD_CONFIG,
                         GetActiveWindow(), (DLGPROC) SDC_Config);
            ShowWindow(hConfDlg,1);
            return;
        }
        BuildDynaMenu();
        return;
    }

    // Return SDC status.
    __declspec(dllexport) void ModuleStatus(char *MyStatus)
    {
        strncpy(MyStatus,Status,16);
        if (idle_ctr < 100) {
            idle_ctr++;
        } else {
            idle_ctr = 0;
            snprintf(Status,16,"SDC:%d idle",CurrentBank);
        }
        return ;
    }

    // Set ini file path and Initialize SDC
    __declspec(dllexport) void SetIniPath (char *IniFilePath)
    {
        strncpy(IniFile,IniFilePath,MAX_PATH);
        return;
    }

    // Return a byte from the current PAK ROM
    __declspec(dllexport) unsigned char PakMemRead8(unsigned short adr)
    {
        adr &= 0x3FFF;
        if (EnableBankWrite) {
            return WriteFlashBank(adr);
        } else {
            BankWriteState = 0;  // Any read resets write state
            return(PakRom[adr]);
        }
    }

    // Capture pointers for MemRead8 and MemWrite8 functions.
    __declspec(dllexport) void MemPointers(MEMREAD8 Temp1,MEMWRITE8 Temp2)
    {
        MemRead8=Temp1;
        MemWrite8=Temp2;
        return;
    }
}

//======================================================================
// Internal functions
//======================================================================

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID rsvd)
{
    if (reason == DLL_PROCESS_ATTACH) {
        hinstDLL = hinst;
        StartupComplete = false;

    } else if (reason == DLL_PROCESS_DETACH) {
        CloseDrive(0);
        CloseDrive(1);
        if (hConfDlg) {
            DestroyWindow(hConfDlg);
            hConfDlg = NULL;
        }
    }

    return TRUE;
}

//-------------------------------------------------------------
// Generate menu for configuring the SDC
//-------------------------------------------------------------
void BuildDynaMenu(void)
{
    DynamicMenuCallback("",0,0);
    DynamicMenuCallback("",6000,0);
    DynamicMenuCallback("SDC Config",5020,2);
    DynamicMenuCallback("",1,0);
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
    SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
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
        break;
    case WM_INITDIALOG:
        CenterDialog(hDlg);
        hConfDlg=hDlg;
        LoadConfig();
        InitFlashBox();
        InitCardBox();
        SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnable,0);
        SetFocus(GetDlgItem(hDlg,ID_NEXT));
        hStartupBank = GetDlgItem(hDlg,ID_STARTUP_BANK);
        char tmp[4];
        snprintf(tmp,4,"%d",(StartupBank & 7));
        SetWindowText(hStartupBank,tmp);
        break;
    case WM_VKEYTOITEM:
        switch (LOWORD(wParam)) {
        case VK_RETURN:
        case VK_SPACE:
            UpdateFlashItem();
            return -2; //no further processing
        case VK_BACK:
        case VK_DELETE:
            DeleteFlashItem();
            return -2;
        }
        return -1;
    case WM_COMMAND:
        switch (HIWORD(wParam)) {
        case LBN_KILLFOCUS:
            SendMessage((HWND)lParam, LB_SETCURSEL, -1, 0);
            return 0;
        case LBN_DBLCLK:
            UpdateFlashItem();
            return 0;
        }
        switch (LOWORD(wParam)) {
        case ID_SD_SELECT:
            SelectCardBox();
            break;
        case ID_SD_BOX:
            if (HIWORD(wParam) == EN_CHANGE) {
                char tmp[MAX_PATH];
                GetWindowText(hSDCardBox,tmp,MAX_PATH);
                if (*tmp != '\0') strncpy(SDCard,tmp,MAX_PATH);
            }
            break;
        case ID_STARTUP_BANK:
            if (HIWORD(wParam) == EN_CHANGE) {
                char tmp[4];
                GetWindowText(hStartupBank,tmp,4);
                StartupBank = atoi(tmp);
                if (StartupBank > 7) StartupBank &= 7;
            }
            break;
        case IDOK:
            if (SaveConfig(hDlg))
                EndDialog(hDlg,LOWORD(wParam));
            break;
        case IDCANCEL:
            EndDialog(hDlg,LOWORD(wParam));
            break;
        case ID_NEXT:
            MountNext (0);
            SetFocus(GetParent(hDlg));
            break;
        }
    }
    return 0;
}

//------------------------------------------------------------
// Get SDC settings from ini file
//------------------------------------------------------------
void LoadConfig(void)
{
    GetPrivateProfileString
        ("SDC", "SDCardPath", "", SDCard, MAX_PATH, IniFile);

    if (!IsDirectory(SDCard)) {
        MessageBox (0,"Invalid SDCard Path in VCC init","Error",0);
    }

    for (int i=0;i<8;i++) {
        char txt[32];
        snprintf(txt,MAX_PATH,"FlashFile_%d",i);
        GetPrivateProfileString
            ("SDC", txt, "", FlashFile[i], MAX_PATH, IniFile);
    }

    ClockEnable = GetPrivateProfileInt("SDC","ClockEnable",1,IniFile);
    StartupBank = GetPrivateProfileInt("SDC","StarupBank",0,IniFile);
}

//------------------------------------------------------------
// Save config to ini file
//------------------------------------------------------------
bool SaveConfig(HWND hDlg)
{
    if (!IsDirectory(SDCard)) {
        MessageBox(0,"Invalid SDCard Path\n","Error",0);
        return false;
    }
    WritePrivateProfileString("SDC","SDCardPath",SDCard,IniFile);

    for (int i=0;i<8;i++) {
        char txt[32];
        sprintf(txt,"FlashFile_%d",i);
        WritePrivateProfileString("SDC",txt,FlashFile[i],IniFile);
    }

    if (SendDlgItemMessage(hDlg,IDC_CLOCK,BM_GETCHECK,0,0)) {
        WritePrivateProfileString("SDC","ClockEnable","1",IniFile);
    } else {
        WritePrivateProfileString("SDC","ClockEnable","0",IniFile);
    }
    char tmp[4];
    snprintf(tmp,4,"%d",(StartupBank & 7));
    WritePrivateProfileString("SDC","StarupBank",tmp,IniFile);
    return true;
}

//----------------------------------------------------------------------
// Init the controller. This gets called by ModuleReset
//----------------------------------------------------------------------
void SDCInit(void)
{

#ifdef USE_LOGGING
    _DLOG("\nSDCInit\n");
    MoveWindow(GetConsoleWindow(),0,0,300,800,TRUE);
#endif

    // Make sure drives are unloaded
    MountDisk (0,"",0);
    MountDisk (1,"",0);

    // Load SDC settings
    LoadConfig();
    LoadRom(StartupBank);

    SetCurDir(""); // May be changed by ParseStartup()

    memset((void *) &Disk,0,sizeof(Disk));

    // Process the startup config file
    ParseStartup();

    // init the interface
    memset(&IF,0,sizeof(IF));

    return;
}

//------------------------------------------------------------
// Init flash box
//------------------------------------------------------------
void InitFlashBox(void)
{
    char path[MAX_PATH];
    char text[MAX_PATH];
    hFlashBox = GetDlgItem(hConfDlg,ID_FLASH_BOX);

    // Set height of items and initialize the listbox
    SendMessage(hFlashBox,LB_SETHORIZONTALEXTENT,0,200);
    SendMessage(hFlashBox, LB_SETITEMHEIGHT, 0, 18);
    SendMessage(hFlashBox,LB_RESETCONTENT,0,0);

    HDC hdc = GetDC(hConfDlg);

    // Add items to the listbox
    for (int index=0; index<8; index++) {
        // Compact the path to fit in flash box. 385 is width pixels
        strncpy(path,FlashFile[index],MAX_PATH);
        PathCompactPath(hdc,path,385);
        sprintf(text,"%d %s",index,path);
        SendMessage(hFlashBox, LB_ADDSTRING, 0, (LPARAM)text);
    }
    ReleaseDC(hFlashBox,hdc);
}

//------------------------------------------------------------
// Init SD card box
//------------------------------------------------------------

void InitCardBox(void)
{
    hSDCardBox = GetDlgItem(hConfDlg,ID_SD_BOX);
    SendMessage(hSDCardBox, WM_SETTEXT, 0, (LPARAM)SDCard);
}
//------------------------------------------------------------
// Delete flash box item
//------------------------------------------------------------
void DeleteFlashItem(void)
{
    int index = SendMessage(hFlashBox, LB_GETCURSEL, 0, 0);
    _DLOG("DeleteFlashItem %d\n",index);
    if ((index < 0) | (index > 7)) return;
    *FlashFile[index] = '\0';
    InitFlashBox();
}

//------------------------------------------------------------
// Update flash box item
//------------------------------------------------------------
void UpdateFlashItem(void)
{
    char filename[MAX_PATH]={};

    int index = SendMessage(hFlashBox, LB_GETCURSEL, 0, 0);
    if ((index < 0) | (index > 7)) return;

    char title[64];
    snprintf(title,64,"Update Flash Bank %d",index);

    OPENFILENAME ofn ;
    strncpy(filename,FlashFile[index],MAX_PATH);
    // DeDanitize
    for(unsigned int i=0; i<strlen(filename); i++) {
        if (filename[i] == '/') filename[i] = '\\';
    }

    memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize       = sizeof (OPENFILENAME);
    ofn.hwndOwner         = hFlashBox;          // hConfigDlg?
    ofn.Flags             = OFN_HIDEREADONLY;
    ofn.lpstrDefExt       = ".rom";
    ofn.lpstrFilter       = "Rom File\0*.rom\0All Files\0*.*\0\0";
    ofn.nFilterIndex      = 0 ;
    ofn.lpstrFile         = filename;
    ofn.nMaxFile          = MAX_PATH;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrTitle        = title;

    // Sanitize
    if (GetOpenFileName(&ofn)) {
        for(unsigned int i=0; i<strlen(filename); i++) {
            if (filename[i] == '\\') filename[i] = '/';
        }
        strncpy(FlashFile[index],filename,MAX_PATH);
    }

    _DLOG("UpdateFlashItem %d %s\n",index,FlashFile[index]);

    InitFlashBox();
}

//------------------------------------------------------------
// Dialog to select SD card path in user home directory
//------------------------------------------------------------
void SelectCardBox(void)
{
    // Prompt user for path
    BROWSEINFO bi = { 0 };
    bi.hwndOwner = GetActiveWindow();
    bi.lpszTitle = "Set the SD card path";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NONEWFOLDERBUTTON;

    // Start from user home diretory
    SHGetSpecialFolderLocation
        (NULL,CSIDL_PROFILE,(LPITEMIDLIST *) &bi.pidlRoot);

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != 0) {
        SHGetPathFromIDList(pidl,SDCard);
        CoTaskMemFree(pidl);
    }

    // Sanitize slashes
    for(unsigned int i=0; i<strlen(SDCard); i++) {
        if (SDCard[i] == '\\') SDCard[i] = '/';
    }

    SendMessage(hSDCardBox, WM_SETTEXT, 0, (LPARAM)SDCard);
}

//-------------------------------------------------------------
// Load rom from flash bank
//-------------------------------------------------------------
void LoadRom(unsigned char bank)
{

    unsigned char ch;
    int ctr = 0;
    char *p_rom;
    char *RomFile;

    if (bank == CurrentBank) return;

    // Make sure flash file is closed
    if (h_RomFile) {
        fclose(h_RomFile);
        h_RomFile = NULL;
    }

    if (BankDirty) {
        RomFile = FlashFile[CurrentBank];
        _DLOG("LoadRom switching out dirty bank %d %s\n",CurrentBank,RomFile);
        h_RomFile = fopen(RomFile,"wb");
        if (h_RomFile == NULL) {
            _DLOG("LoadRom failed to open bank file%d\n",bank);
        } else {
            ctr = 0;
            p_rom = PakRom;
            while (ctr++ < 0x4000) fputc(*p_rom++, h_RomFile);
            fclose(h_RomFile);
            h_RomFile = NULL;
        }
        BankDirty = 0;
    }

    _DLOG("LoadRom load flash bank %d\n",bank);
    RomFile = FlashFile[bank];
    CurrentBank = bank;

    // If bank is empty and is the StartupBank load SDC-DOS
    if (*FlashFile[CurrentBank] == '\0') {
        _DLOG("LoadRom bank %d is empty\n",CurrentBank);
        if (CurrentBank == StartupBank) {
            _DLOG("LoadRom loading default SDC-DOS\n");
            strncpy(RomFile,"SDC-DOS.ROM",MAX_PATH);
        }
    }

    // Open romfile for read or write if not startup bank
    h_RomFile = fopen(RomFile,"rb");
    if (h_RomFile == NULL) {
        if (CurrentBank != StartupBank) h_RomFile = fopen(RomFile,"wb");
    }
    if (h_RomFile == NULL) {
        _DLOG("LoadRom '%s' failed %s \n",RomFile,LastErrorTxt());
        return;
    }

    // Load rom from flash
    memset(PakRom, 0xFF, 0x4000);
    ctr = 0;
    p_rom = PakRom;
    while (ctr++ < 0x4000) {
        if ((ch = fgetc(h_RomFile)) < 0) break;
        *p_rom++ = (char) ch;
    }

    fclose(h_RomFile);
    return;
}

//----------------------------------------------------------------------
// Parse the startup.cfg file
//----------------------------------------------------------------------
void ParseStartup(void)
{
    char buf[MAX_PATH+10];
    if (!IsDirectory(SDCard)) {
        _DLOG("ParseStartup SDCard path invalid\n");
        return;
    }

    strncpy(buf,SDCard,MAX_PATH);
    AppendPathChar(buf,'/');
    strncat(buf,"startup.cfg",MAX_PATH);

    FILE *su = fopen(buf,"r");
    if (su == NULL) {
        _DLOG("ParseStartup file not found,%s\n",buf);
        return;
    }

    // Strict single char followed by '=' then path
    while (fgets(buf,sizeof(buf),su) > 0) {
        //Chomp line ending
        buf[strcspn(buf,"\r\n")] = 0;
        // Skip line if less than 3 chars;
        if (strlen(buf) < 3) continue;
        // Skip line if second char is not '='
        if (buf[1] != '=') continue;
        // Grab drive num char
        char drv = buf[0];
        // Attempt to mount drive
        switch (drv) {
        case '0':
            MountDisk(0,&buf[2],0);
            break;
        case '1':
            MountDisk(1,&buf[2],0);
            break;
        case 'D':
            SetCurDir(&buf[2]);
            break;
        }
    }
    fclose(su);
}

//----------------------------------------------------------------------
// Write port.  If a command needs a data block to complete it
// will put a count (256 or 512) in IF.bufcnt.
//----------------------------------------------------------------------
void SDCWrite(unsigned char data,unsigned char port)
{
    if ((!IF.sdclatch) && (port > 0x43)) return;

    switch (port) {
    // Control Latch
    case 0x40:
        if (IF.sdclatch) memset(&IF,0,sizeof(IF));
        if (data == 0x43) IF.sdclatch = true;
        break;
    // Flash Data
    case 0x42:
        //_DLOG("IF data: %02X\n",data);
        BankData = data;
        break;
    // Flash Control
    case 0x43:
        FlashControl(data);
        break;
    // Command registor
    case 0x48:
        IF.cmdcode = data;
        SDCCommand();
        break;
    // Command param #1
    case 0x49:
        IF.param1 = data;
        break;
    // Command param #2 or block data receive
    case 0x4A:
        if (IF.bufcnt > 0)
            BlockReceive(data);
        else
            IF.param2 = data;;
        break;
    // Command param #3 or block data receive
    case 0x4B:
        if (IF.bufcnt > 0)
            BlockReceive(data);
        else
            IF.param3 = data;;
        break;
    }
    return;
}

//----------------------------------------------------------------------
// Can't reply with words, only bytes.  But the SDC interface design
// has most replies in words and the order the word bytes are read can
// vary so we play games to send the right ones
//----------------------------------------------------------------------
unsigned char PickReplyByte(unsigned char port)
{
    unsigned char rpy = 0;

    // Byte mode bytes come on port 0x4B
    if (IF.reply_mode == 1) {
        if (IF.bufcnt > 0) {
            rpy = *IF.bufptr++;
            IF.bufcnt--;
        }
    // Word mode bytes come on port 0x4A and 0x4B
    } else {
        if (port == 0x4A) {
            rpy = IF.bufptr[0];
        } else {
            rpy = IF.bufptr[1];
        }
        if (IF.half_sent) {
            IF.bufcnt -= 2;
            IF.bufptr += 2;
            IF.half_sent = 0;
        } else {
            IF.half_sent = 1;
        }
    }

    // Keep stream going until stopped
    if ((IF.bufcnt < 1) && (streaming)) StreamImage();

    return rpy;
}

//----------------------------------------------------------------------
// Read port. If there are bytes in the reply buffer return them.
//----------------------------------------------------------------------
unsigned char SDCRead(unsigned char port)
{

    if ((!IF.sdclatch) && (port > 0x43)) return 0;

    unsigned char rpy;
    switch (port) {
    // Flash control read is used by SDCDOS to detect the SDC
    case 0x43:
        rpy = CurrentBank | (BankData & 0xF8);
        break;
    // Interface status
    case 0x48:
        if (IF.bufcnt > 0) {
            rpy = (IF.reply_status != 0) ? IF.reply_status : STA_BUSY | STA_READY;
        } else {
            rpy = IF.status;
        }
        break;
    // Reply data 1
    case 0x49:
        rpy = IF.reply1;
        break;
    // Reply data 2 or block reply
    case 0x4A:
        if (IF.bufcnt > 0) {
            rpy = PickReplyByte(port);
        } else {
            rpy = IF.reply2;
        }
        break;
    // Reply data 3 or block reply
    case 0x4B:
        if (IF.bufcnt > 0) {
            rpy = PickReplyByte(port);
        } else {
            rpy = IF.reply3;
        }
        break;
    default:
        rpy = 0;
        break;
    }
    return rpy;
}

//----------------------------------------------------------------------
//  Dispatch SDC commands
//----------------------------------------------------------------------
void SDCCommand(void)
{

    switch (IF.cmdcode & 0xF0) {
    // Read sector
    case 0x80:
        ReadSector();
        break;
    // Stream 512 byte sectors
    case 0x90:
        StreamImage();
        break;
    // Get drive info
    case 0xC0:
        GetDriveInfo();
        break;
     // Control SDC
    case 0xD0:
        SDCControl();
        break;
    // Next two are block receive commands
    case 0xA0:
    case 0xE0:
        IF.status = STA_READY | STA_BUSY;
        IF.bufptr = IF.blkbuf;
        IF.bufcnt = 256;
        IF.half_sent = 0;
        break;
    }
    return;
}

//----------------------------------------------------------------------
// Receive block data
//----------------------------------------------------------------------
void BlockReceive(unsigned char byte)
{
    if (IF.bufcnt > 0) {
        IF.bufcnt--;
        *IF.bufptr++ = byte;
    }

    // Done receiving block
    if (IF.bufcnt < 1) {
        switch (IF.cmdcode & 0xF0) {
        case 0xA0:
            WriteSector();
            break;
        case 0xE0:
            UpdateSD();
            break;
        default:
            _DLOG("BlockReceive invalid cmd %d\n",IF.cmdcode);
            IF.status = STA_FAIL;
            break;
        }
    }
}

//----------------------------------------------------------------------
// Get drive information
//----------------------------------------------------------------------
void GetDriveInfo(void)
{
    int drive = IF.cmdcode & 1;
    switch (IF.param1) {
    case 0x49:
        // 'I' - return drive information in block
        GetMountedImageRec();
        break;
    case 0x43:
        // 'C' Return current directory leaf in block
        GetDirectoryLeaf();
        break;
    case 0x51:
        // 'Q' Return the size of disk image in p1,p2,p3
        GetSectorCount();
        break;
    case 0x3E:
        // '>' Get directory page
        LoadDirPage();
        IF.reply_mode=0;
        LoadReply(DirPage,256);
        break;
    case 0x2B:
        // '+' Mount next next disk in set.
        MountNext(drive);
        break;
    case 0x56:
        // 'V' Get BCD firmware version number in p2, p3.
        IF.reply2 = 0x00;
        IF.reply3 = 0x01;
        break;
    }
}

//----------------------------------------------------------------------
// Get directory leaf.  This is the leaf name of the current directory,
// not it's full path.  SDCEXP uses this with the set directory '..'
// command to learn the full path when restore last session is active.
// The full path is saved in SDCX.CFG for the next session.
//----------------------------------------------------------------------
void GetDirectoryLeaf(void)
{
    _DLOG("GetDirectoryLeaf CurDir '%s'\n",CurDir);

    char leaf[32];
    memset(leaf,0,32);

    // Strip trailing '/' from current directory. There should not
    // be one there but the slash is so bothersome best to check.
    int n = strlen(CurDir);
    if (n > 0) {
        n -= 1;
        if (CurDir[n] == '/') CurDir[n] = '\0';
    }

    // If at least one leaf find the last one
    if (n > 0) {
        char *p = strrchr(CurDir,'/');
        if (p == NULL) {
            p = CurDir;
        } else {
            p += 1;
        }
        // Build reply
        memset(leaf,32,12);
        strncpy(leaf,p,12);
        n = strlen(p);
        // SDC filenames are fixed 8 chars so blank any terminator
        if (n < 12) leaf[n] = ' ';
    // If current directory is SDCard root reply with zeros and status
    } else {
        IF.reply_status = STA_FAIL | STA_NOTFOUND;
    }

    IF.reply_mode=0;
    LoadReply(leaf,32);
}

//----------------------------------------------------------------------
//  Update SD Commands.
//----------------------------------------------------------------------
void UpdateSD(void)
{
    _DLOG("UpdateSD %d %02X '%s' %d %d %d\n",
            IF.cmdcode&1,IF.blkbuf[0],&IF.blkbuf[2],
            IF.param1, IF.param2, IF.param3);

    switch (IF.blkbuf[0]) {
    // Mount disk
    case 0x4D: //M
        MountDisk(IF.cmdcode&1,&IF.blkbuf[2],0);
        break;
    case 0x6D: //m
        MountDisk(IF.cmdcode&1,&IF.blkbuf[2],1);
        break;
    // Create Disk
    case 0x4E: //N
        MountNewDisk(IF.cmdcode&1,&IF.blkbuf[2],0);
        break;
    case 0x6E: //n
        MountNewDisk(IF.cmdcode&1,&IF.blkbuf[2],1);
        break;
    // Set directory
    case 0x44: //D
        SetCurDir(&IF.blkbuf[2]);
        break;
    // Initiate directory list
    case 0x4C: //L
        InitiateDir(&IF.blkbuf[2]);
        break;
    // Create directory
    case 0x4B: //K
        _DLOG("UpdateSD create new directory not Supported\n");
        IF.status = STA_FAIL;
        break;
    // Rename file or directory
    case 0x52: //R
        RenameFile(&IF.blkbuf[2]);
        break;
    // Delete file or directory
    case 0x5B: //X
        _DLOG("UpdateSD delete file or directory not Supported\n");
        IF.status = STA_FAIL;
        break;
    default:
        _DLOG("UpdateSD %02x not Supported\n",IF.blkbuf[0]);
        IF.status = STA_FAIL;
        break;
    }
}

//----------------------------------------------------------------------
// Flash control
//----------------------------------------------------------------------
void FlashControl(unsigned char data)
{
    unsigned char bank = data & 7;
    EnableBankWrite = data & 0x80;
    if (EnableBankWrite) {
        BankWriteNum = bank;
    } else if (CurrentBank != bank) {
        LoadRom(bank);
    }
}

//----------------------------------------------------------------------
// Write or kill flash bank
//
// To write a byte a sequence of four writes is used. The first three
// writes prepare the SST39S flash. The last writes the byte.
//
//    state 0 write bank 1 adr $1555 val $AA
//    state 1 write bank 0 adr $2AAA val $55
//    state 2 write bank 1 adr $1555 val $A0
//    state 3 write bank # adr [adr] val write val to adr in bank #
//
// Kill bank sector is a sequence of six writes. All six are required
// to kill the bank sector.
//
//    state 0 write bank 1 adr 1555 val AA
//    state 1 write bank 0 adr 2AAA val 55
//    state 2 write bank 1 adr 1555 val 80
//    state 4 write bank 1 adr 1555 val AA
//    state 5 write bank 0 adr 2AAA val 55
//    state 6 write bank # adr sect val 30 kill bank # sect (adr & 0x3000)
//
//----------------------------------------------------------------------
unsigned char WriteFlashBank(unsigned short adr)
{
    _DLOG("WriteFlashBank %d %d %04X %02X\n",
            BankWriteState,BankWriteNum,adr,BankData);

    // BankWriteState controls the write or kill
    switch (BankWriteState) {
    case 0:
        if ((BankWriteNum == 1) && (adr == 0x1555))
            if (BankData == 0xAA) BankWriteState = 1;
        break;
    case 1:
        if ((BankWriteNum == 0) && (adr == 0x2AAA))
            if (BankData == 0x55) BankWriteState = 2;
        break;
    case 2:
        if ((BankWriteNum == 1) && (adr == 0x1555)) {
            if (BankData == 0xA0) BankWriteState = 3;
            else if (BankData == 0x80) BankWriteState = 4;
        }
        break;
    // State three writes data
    case 3:
        if (BankWriteNum != CurrentBank) LoadRom(BankWriteNum);
        PakRom[adr] = BankData;
        BankDirty = 1;
        break;
    // State four continues kill sequence
    case 4:
        if ((BankWriteNum == 1) && (adr == 0x1555))
            if (BankData == 0xAA) BankWriteState = 5;
        break;
    case 5:
        if ((BankWriteNum == 0) && (adr == 0x2AAA))
            if (BankData == 0x55) BankWriteState = 6;
        break;
    // State six kills (fills with 0xFF) bank sector
    case 6:
        if (BankData == 0x30) {
            if (BankWriteNum != CurrentBank) LoadRom(BankWriteNum);
            memset(PakRom + (adr & 0x3000), 0xFF, 0x1000);
            BankDirty = 1;
        }
        break;
    }
    EnableBankWrite = 0;
    return BankData;
}

//----------------------------------------------------------------------
// Seek sector in drive image
//  cmdcode:
//    b0 drive number
//    b1 single sided flag
//    b2 eight bit transfer flag
//----------------------------------------------------------------------
bool SeekSector(unsigned char cmdcode, unsigned int lsn)
{
    // Adjust for read of one side of a doublesided disk.
    int drive = cmdcode & 1;
    if ((Disk[drive].doublesided) && (cmdcode & 2)) {
        int trk = lsn / Disk[drive].tracksectors;
        int sec = lsn % Disk[drive].tracksectors;
        lsn = 2 * Disk[drive].tracksectors * trk + sec;
    }

    // Allow seek to expand a writable file to a resonable limit
    if (lsn > MAX_DSK_SECTORS) {
        _DLOG("SeekSector exceed max image %d %d\n",drive,lsn);
        return false;
    }

    // Seek to logical sector on drive.
    LARGE_INTEGER pos;
    pos.QuadPart = lsn * Disk[drive].sectorsize + Disk[drive].headersize;
    if (!SetFilePointerEx(Disk[drive].hFile,pos,NULL,FILE_BEGIN)) {
        _DLOG("SeekSector error %s\n",LastErrorTxt());
        return false;
    }
    return true;
}

//----------------------------------------------------------------------
// Read a sector from drive image and load reply
//----------------------------------------------------------------------
bool ReadDrive(unsigned char cmdcode, unsigned int lsn)
{
    char buf[520];
    DWORD cnt = 0;

    int drive = cmdcode & 1;
    if (Disk[drive].hFile == NULL) {
        _DLOG("ReadDrive %d not open\n");
        return false;
    }

    if (!SeekSector(cmdcode,lsn)) {
        return false;
    }

    if (!ReadFile(Disk[drive].hFile,buf,Disk[drive].sectorsize,&cnt,NULL)) {
        _DLOG("ReadDrive %d %s\n",drive,LastErrorTxt());
        return false;
    }

    if (cnt != Disk[drive].sectorsize) {
        _DLOG("ReadDrive %d short read\n",drive);
        return false;
    }

    snprintf(Status,16,"SDC:%d Rd %d,%d",CurrentBank,drive,lsn);
    LoadReply(buf,cnt);
    return true;
}

//----------------------------------------------------------------------
// Read logical sector
// cmdcode:
//    b0 drive number
//    b1 single sided flag
//    b2 eight bit transfer flag
//----------------------------------------------------------------------
void ReadSector(void)
{
    unsigned int lsn = (IF.param1 << 16) + (IF.param2 << 8) + IF.param3;
    IF.reply_mode = ((IF.cmdcode & 4) == 0) ? 0 : 1; // words : bytes
    if (!ReadDrive(IF.cmdcode,lsn)) IF.status = STA_FAIL;
}

//----------------------------------------------------------------------
// Stream image data
//----------------------------------------------------------------------
void StreamImage(void)
{
    // If already streaming continue
    if (streaming) {
        stream_lsn++;
    // Else start streaming
    } else {
        stream_cmdcode = IF.cmdcode;
        IF.reply_mode = ((IF.cmdcode & 4) == 0) ? 0 : 1;
        stream_lsn = (IF.param1 << 16) + (IF.param2 << 8) + IF.param3;
        _DLOG("StreamImage lsn %d\n",stream_lsn);
    }

    // For now can only stream 512 byte sectors
    int drive = stream_cmdcode & 1;
    Disk[drive].sectorsize = 512;
    Disk[drive].tracksectors = 9;

    if (stream_lsn > (Disk[drive].size/Disk[drive].sectorsize)) {
        _DLOG("StreamImage done\n");
        streaming = 0;
        return;
    }

    if (!ReadDrive(stream_cmdcode,stream_lsn)) {
        _DLOG("StreamImage read error %s\n",LastErrorTxt());
        IF.status = STA_FAIL;
        streaming = 0;
        return;
    }
    streaming = 1;
}

//----------------------------------------------------------------------
// Write logical sector
//----------------------------------------------------------------------
void WriteSector(void)
{
    DWORD cnt = 0;
    int drive = IF.cmdcode & 1;
    unsigned int lsn = (IF.param1 << 16) + (IF.param2 << 8) + IF.param3;
    snprintf(Status,16,"SDC:%d Wr %d,%d",CurrentBank,drive,lsn);

    if (Disk[drive].hFile == NULL) {
        IF.status = STA_FAIL;
        return;
    }

    if (!SeekSector(drive,lsn)) {
        IF.status = STA_FAIL;
        return;
    }
    if (!WriteFile(Disk[drive].hFile,IF.blkbuf,
                   Disk[drive].sectorsize,&cnt,NULL)) {
        _DLOG("WriteSector %d %s\n",drive,LastErrorTxt());
        IF.status = STA_FAIL;
        return;
    }
    if (cnt != Disk[drive].sectorsize) {
        _DLOG("WriteSector %d short write\n",drive);
        IF.status = STA_FAIL;
        return;
    }
    IF.status = 0;
    return;
}

//----------------------------------------------------------------------
// Get most recent windows error text
//----------------------------------------------------------------------
char * LastErrorTxt(void) {
    static char msg[200];
    DWORD error_code = GetLastError();
    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error_code,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   msg, 200, nullptr );
    return msg;
}

//----------------------------------------------------------------------
// Return sector count for mounted disk image
//----------------------------------------------------------------------
void  GetSectorCount() {

    int drive = IF.cmdcode & 1;
    unsigned int numsec = Disk[drive].size/Disk[drive].sectorsize;
    IF.reply3 = numsec & 0xFF;
    numsec = numsec >> 8;
    IF.reply2 = numsec & 0xFF;
    numsec = numsec >> 8;
    IF.reply1 = numsec & 0xFF;
    IF.status = STA_READY;
}

//----------------------------------------------------------------------
// Return file record for mounted disk image
//----------------------------------------------------------------------
void GetMountedImageRec()
{
    int drive = IF.cmdcode & 1;
    //_DLOG("GetMountedImageRec %d %s\n",drive,Disk[drive].name);
    if (strlen(Disk[drive].name) == 0) {
        IF.status = STA_FAIL;
    } else {
        IF.reply_mode = 0;
        LoadReply(&Disk[drive].filerec,sizeof(FileRecord));
    }
}

//----------------------------------------------------------------------
// $DO Abort stream and mount disk in a set of disks.
// IF.param1  0: Next disk 1-9: specific disk.
// IF.param2 b0: Blink Enable
//----------------------------------------------------------------------
void SDCControl(void)
{
    // If streaming is in progress abort it.
    if (streaming) {
        _DLOG ("Streaming abort");
        streaming = 0;
        IF.status = STA_READY;
        IF.bufcnt = 0;
    } else {
        // TODO: Mount in set
        _DLOG("SDCControl Mount in set unsupported %d %d %d \n",
                  IF.cmdcode,IF.param1,IF.param2);
        IF.status = STA_FAIL | STA_NOTFOUND;
    }
}

//----------------------------------------------------------------------
// Load reply. Count is bytes, 512 max.
//----------------------------------------------------------------------
void LoadReply(void *data, int count)
{
    if ((count < 2) | (count > 512)) {
        _DLOG("LoadReply bad count %d\n",count);
        return;
    }

    memcpy(IF.blkbuf,data,count);

    IF.bufptr = IF.blkbuf;
    IF.bufcnt = count;
    IF.half_sent = 0;

    // If port reads exceed the count zeros will be returned
    IF.reply2 = 0;
    IF.reply3 = 0;

    return;
}

//----------------------------------------------------------------------
// The name portion of SDC path may be in SDC format which does
// not use a dot to seperate the extension examples:
//    "FOO     DSK" = FOO.DSK
//    "ALONGFOODSK" = ALONGFOO.DSK
//----------------------------------------------------------------------
void FixSDCPath(char *path, const char *fpath8)
{
    const char *pname8 = strrchr(fpath8,'/');
    // Copy Directory portion
    if (pname8 != NULL) {
        pname8++;
        memcpy(path,fpath8,pname8-fpath8);
    } else {
        pname8 = fpath8;
    }
    path[pname8-fpath8]='\0'; // terminate directory portion

    // Copy Name portion
    char c;
    char name[16];
    int  namlen=0;
    while(c = *pname8++) {
        if ((c == '.')||(c == ' ')) break;
        name[namlen++] = c;
        if (namlen > 7) break;
    }

    // Copy extension if any thing is left
    if (c) {
        name[namlen++] = '.';
        int extlen=0;
        while(c = *pname8++) {
        if ((c == '.')||(c == ' ')) continue;
            name[namlen++] = c;
            extlen++;
            if (extlen > 2) break;
        }
    }
    name[namlen] = '\0';           // terminate name
    strncat(path,name,MAX_PATH);   // append it to directory
}

//----------------------------------------------------------------------
// Load a file record with the file found by Find File
//----------------------------------------------------------------------
bool LoadFindRecord(struct FileRecord * rec)
{
    memset(rec,0,sizeof(rec));
    memset(rec->name,' ',8);
    memset(rec->type,' ',3);

    // File type
    char * pdot = strrchr(dFound.cFileName,'.');
    if (pdot) {
        char * ptyp = pdot + 1;
        for (int cnt = 0; cnt<3; cnt++) {
           if (*ptyp == '\0') break;
           rec->type[cnt] = *ptyp++;
        }
    }

    // File name
    char * pnam = dFound.cFileName;
    for (int cnt = 0; cnt < 8; cnt++) {
        if (*pnam == '\0') break;
        if (pdot && (pnam == pdot)) break;
        rec->name[cnt] = *pnam++;
    }

    // Attributes
    if (dFound.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
        rec->attrib |= ATTR_RDONLY;
    }
    if (dFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        rec->attrib |= ATTR_DIR;
    }

    // Filesize, sssume < 4G (dFound.nFileSizeHigh == 0)
    rec->lolo_size = (dFound.nFileSizeLow) & 0xFF;
    rec->hilo_size = (dFound.nFileSizeLow >> 8) & 0xFF;
    rec->lohi_size = (dFound.nFileSizeLow >> 16) & 0xFF;
    rec->hihi_size = (dFound.nFileSizeLow >> 24) & 0xFF;

    return true;
}

//----------------------------------------------------------------------
// Create and mount a new disk image
//  IF.param1 == 0 create 161280 byte JVC file
//  IF.param1 SDF image number of cylinders
//  IF.param2 SDC image number of sides
//----------------------------------------------------------------------
void MountNewDisk (int drive, const char * path, int raw)
{
    //_DLOG("MountNewDisk %d %s %d\n",drive,path,raw);

    // limit drive to 0 or 1
    drive &= 1;

    // Close and clear previous entry
    CloseDrive(drive);
    memset((void *) &Disk[drive],0,sizeof(_Disk));

    // Convert from SDC format
    char file[MAX_PATH];
    FixSDCPath(file,path);

    // Look for pre-existing file
    if (SearchFile(file)) {
        OpenFound(drive,raw);
        return;
    }

    OpenNew(drive,path,raw);
    return;
}

//----------------------------------------------------------------------
// Mount Disk. If image path starts with '/' load drive relative
// to SDRoot, else load drive relative to the current directory.
// If there is no '.' in the path first appending '.DSK' will be
// tried then wildcard. If wildcarded the set will be available for
// the 'Next Disk' function.
// TODO: Sets of type SOMEAPPn.DSK
//----------------------------------------------------------------------
void MountDisk (int drive, const char * path, int raw)
{
    _DLOG("MountDisk %d %s %d\n",drive,path,raw);

    drive &= 1;

    // Close and clear previous entry
    CloseDrive(drive);
    memset((void *) &Disk[drive],0,sizeof(_Disk));

    // Check for UNLOAD.  Path will be an empty string.
    if (*path == '\0') {
        _DLOG("MountDisk unload %d %s\n",drive,path);
        IF.status = STA_NORMAL;
        return;
    }

    char file[MAX_PATH];
    char tmp[MAX_PATH];

    // Convert from SDC format
    FixSDCPath(file,path);

    // Look for the file
    bool found = SearchFile(file);

    // if no '.' in the name try appending .DSK  or wildcard
    if (!found && (strchr(file,'.') == NULL)) {
        strncpy(tmp,file,MAX_PATH);
        strncat(tmp,".DSK",MAX_PATH);
        found = SearchFile(tmp);
        if(!found) {
            strncpy(tmp,file,MAX_PATH);
            strncat(tmp,"*.*",MAX_PATH);
            found = SearchFile(tmp);
        }
    }

    // Give up
    if (!found) {
        IF.status = STA_FAIL | STA_NOTFOUND;
        return;
    }

    // Mount first image found
    OpenFound(drive,raw);
    return;
}

//----------------------------------------------------------------------
// Mount Next Disk from found set
//----------------------------------------------------------------------
bool MountNext (int drive)
{
    if (FindNextFile(hFind,&dFound) == 0) {
        _DLOG("MountNext no more\n");
        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
        return false;
    }

    // Open next image found on the drive
    OpenFound(drive,0);
    return true;
}

//----------------------------------------------------------------------
// Open new disk image
//
//  IF.Param1: $FF49 B   number of cylinders for SDF
//  IF.Param2: $FF4A X.H number of sides for SDF image
//
//  OpenNew 0 'A.DSK' 0 40 0 NEW
//  OpenNew 0 'B.DSK' 0 40 1 NEW++ one side
//  OpenNew 0 'C.DSK' 0 40 2 NEW++ two sides
//
//  Currently new JVC files are 35 cylinders, one sided
//  Possibly future num cylinders could specify 40 or more
//  cylinders with num cyl controlling num sides
//
//----------------------------------------------------------------------
void OpenNew( int drive, const char * path, int raw)
{
    _DLOG("OpenNew %d '%s' %d %d %d\n",
          drive,path,raw,IF.param1,IF.param2);

    // Number of sides controls file type
    switch (IF.param2) {
    case 0:    //NEW
        // create JVC DSK file
        break;
    case 1:    //NEW+
    case 2:    //NEW++
        _DLOG("OpenNew SDF file not supported\n");
        IF.status = STA_FAIL | STA_INVALID;
        return;
    }

    // Contruct fully qualified file name
    char fqn[MAX_PATH]={};
    strncpy(fqn,SDCard,MAX_PATH);
    AppendPathChar(fqn,'/');
    strncat(fqn,CurDir,MAX_PATH);
    AppendPathChar(fqn,'/');
    strncat(fqn,path,MAX_PATH);

    // Verify not trying to mount a directory
    if (IsDirectory(fqn)) {
        _DLOG("OpenNew %s is a directory\n",fqn);
        IF.status = STA_FAIL | STA_INVALID;
        return;
    }

    // Try to create file
    CloseDrive(drive);
    strncpy(Disk[drive].name,fqn,MAX_PATH);

    // Open file for write
    Disk[drive].hFile = CreateFile(
        Disk[drive].name, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
        NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);

    if (Disk[drive].hFile == INVALID_HANDLE_VALUE) {
        _DLOG("OpenNew fail %d file %s\n",drive,Disk[drive].name);
        _DLOG("... %s\n",LastErrorTxt());
        IF.status = STA_FAIL | STA_WIN_ERROR;
        return;
    }

    // Sectorsize and sectors per track
    Disk[drive].sectorsize = 256;
    Disk[drive].tracksectors = 18;

    IF.status = STA_FAIL;
    if (raw) {
        // New raw file is empty - can be any format
        IF.status = STA_NORMAL;
        Disk[drive].doublesided = 0;
        Disk[drive].headersize = 0;
        Disk[drive].size = 0;
    } else {
        // TODO: handle SDF
        // Create headerless 35 track JVC file
        Disk[drive].doublesided = 0;
        Disk[drive].headersize = 0;
        Disk[drive].size = 35 
                         * Disk[drive].sectorsize
                         * Disk[drive].tracksectors
                         + Disk[drive].headersize;
        // Extend file to size
        LARGE_INTEGER l_siz;
        l_siz.QuadPart = Disk[drive].size;
        if (SetFilePointerEx(Disk[drive].hFile,l_siz,NULL,FILE_BEGIN)) {
            if (SetEndOfFile(Disk[drive].hFile)) {
                IF.status = STA_NORMAL;
            } else {
                IF.status = STA_FAIL | STA_WIN_ERROR;
            }
        }
    }
    return;
 }

//----------------------------------------------------------------------
// Open disk image found.  Raw flag skips file type checks
//----------------------------------------------------------------------
void OpenFound (int drive,int raw)
{
    drive &= 1;
    int writeprotect = 0;

    _DLOG("OpenFound %d %s %d\n",
        drive, Disk[drive].name, Disk[drive].hFile);

    CloseDrive(drive);

    // Verify not trying to mount a directory
    if (dFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        _DLOG("OpenFound %s is a directory\n",dFound.cFileName);
        IF.status = STA_FAIL | STA_INVALID;
        return;
    }

    // Fully qualify name of found file and try to open it
    char fqn[MAX_PATH]={};
    strncpy(fqn,SeaDir,MAX_PATH);
    strncat(fqn,dFound.cFileName,MAX_PATH);
    strncpy(Disk[drive].name,fqn,MAX_PATH);

    // Open file for read
    Disk[drive].hFile = CreateFile(
        Disk[drive].name, GENERIC_READ, FILE_SHARE_READ,
        NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (Disk[drive].hFile == INVALID_HANDLE_VALUE) {
        _DLOG("OpenFound fail %d file %s\n",drive,Disk[drive].name);
        _DLOG("... %s\n",LastErrorTxt());
        int ecode = GetLastError();
        if (ecode == ERROR_SHARING_VIOLATION) {
            IF.status = STA_FAIL | STA_INUSE;
        } else {
            IF.status = STA_FAIL | STA_WIN_ERROR;
        }
        return;
    }

    // Default sectorsize and sectors per track
    Disk[drive].sectorsize = 256;
    Disk[drive].tracksectors = 18;

    // Grab filesize from found record
    Disk[drive].size = dFound.nFileSizeLow;

    if (raw) {
        writeprotect = 0;
        Disk[drive].headersize = 0;
        Disk[drive].doublesided = 0;
    } else {
        // Determine if it is a doublesided disk.
        // Header size also used to offset sector seeks
        Disk[drive].headersize = Disk[drive].size & 255;

        unsigned int numsec;
        unsigned char header[16];
        switch (Disk[drive].headersize) {
        // JVC optional header bytes
        case 4: // First Sector     = header[3]   1 assumed
        case 3: // Sector Size code = header[2] 256 assumed {128,256,512,1024}
        case 2: // Number of sides  = header[1]   1 or 2
        case 1: // Sectors per trk  = header[0]  18 assumed
            ReadFile(Disk[drive].hFile,header,4,NULL,NULL);
            Disk[drive].doublesided = (header[1] == 2);
            break;
        // No header JVC or OS9 disk if no header, side count per file size
        case 0:
            numsec = Disk[drive].size >> 8;
            Disk[drive].doublesided = ((numsec > 720) && (numsec <= 2880));
            break;
        // VDK
        case 12:
            ReadFile(Disk[drive].hFile,header,12,NULL,NULL);
            Disk[drive].doublesided = (header[8] == 2);
            writeprotect = header[9] & 1;
            break;
        // Unknown or unsupported
        default: // More than 4 byte header is not supported
            _DLOG("OpenFound unsuported image type %d %d\n",
                drive, Disk[drive].headersize);
            IF.status = STA_FAIL | STA_INVALID;
            return;
            break;
        }
    }

    // Fill in image info.
    LoadFindRecord(&Disk[drive].filerec);

    // Set readonly attrib per find status or file header
    if ((Disk[drive].filerec.attrib & ATTR_RDONLY) != 0) {
        writeprotect = 1;
    } else if (writeprotect) {
        Disk[drive].filerec.attrib |= ATTR_RDONLY;
    }

    // If file is writeable reopen read/write
    if (!writeprotect) {
        CloseHandle(Disk[drive].hFile);
        Disk[drive].hFile = CreateFile(
            Disk[drive].name, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
            NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        if (Disk[drive].hFile == INVALID_HANDLE_VALUE) {
            _DLOG("OpenFound reopen fail %d\n",drive);
            _DLOG("... %s\n",LastErrorTxt());
            IF.status = STA_FAIL | STA_WIN_ERROR;
            return;
        }
    }

    IF.status = STA_NORMAL;
    return;
}

//----------------------------------------------------------------------
// Rename disk or directory
//   names contains two consecutive null terminated name strings
//   first is file or directory to rename, second is target name
//----------------------------------------------------------------------
void RenameFile(const char *names)
{
    char from[MAX_PATH];
    char target[MAX_PATH];
    char fixed[MAX_PATH];

    // convert from name from SDC format and prepend current dir.
    FixSDCPath(fixed,names);
    strncpy(from,SDCard,MAX_PATH);
    AppendPathChar(from,'/');
    strncat(from,CurDir,MAX_PATH);
    AppendPathChar(from,'/');
    strncat(from,fixed,MAX_PATH);

    // convert to name from SDC format and prepend current dir.
    FixSDCPath(fixed,1+strchr(names,'\0'));
    strncpy(target,SDCard,MAX_PATH);
    AppendPathChar(target,'/');
    strncat(target,CurDir,MAX_PATH);
    AppendPathChar(target,'/');
    strncat(target,fixed,MAX_PATH);

    _DLOG("UpdateSD rename %s %s\n",from,target);

    if (std::rename(from,target)) {
        _DLOG("%s\n", strerror(errno));
        IF.status = STA_FAIL | STA_WIN_ERROR;
    } else {
        IF.status = STA_NORMAL;
    }
    return;
}
//----------------------------------------------------------------------
// Close virtual disk
//----------------------------------------------------------------------
void CloseDrive (int drive)
{
    drive &= 1;
    if (Disk[drive].hFile != NULL) {
        CloseHandle(Disk[drive].hFile);
        Disk[drive].hFile = INVALID_HANDLE_VALUE;
    }
}

//----------------------------------------------------------------------
// Append char to path if not there
//----------------------------------------------------------------------
void AppendPathChar(char * path, char c)
{
    int l = strlen(path);
    if ((l > 0) && (path[l-1] == c)) return;
    if (l > (MAX_PATH-2)) return;
    path[l] = c;
    path[l+1] = '\0';
}

//----------------------------------------------------------------------
// Determine if path is a direcory
//----------------------------------------------------------------------
bool IsDirectory(const char * path)
{
    struct _stat file_stat;
    int result = _stat(path,&file_stat);
    if (result != 0) return false;
    return ((file_stat.st_mode & _S_IFDIR) != 0);
}

//----------------------------------------------------------------------
// Set the Current Directory relative to previous current or to SDRoot
//----------------------------------------------------------------------
void SetCurDir(char * path)
{
    //_DLOG("SetCurdir enter '%s' '%s'\n",path,CurDir);

    // If path is "/" set CurDir to root
    if (strcmp(path,"/") == 0) {
        *CurDir = '\0';
        IF.status = STA_NORMAL;
        return;
    }

    // If path is ".." go back a directory
    if (strcmp(path,"..") == 0) {
        int n = strlen(CurDir);
        if (n > 0) {
            if (CurDir[n-1] == '/') CurDir[n-1] = '\0';
            char *p = strrchr(CurDir,'/');
            if (p != NULL) {
                *p = '\0';
                IF.status = STA_NORMAL;
                return;
            }
        }
        *CurDir = '\0';
        IF.status = STA_NORMAL;
        return;
    }

    char cleanpath[MAX_PATH];

    // Find trailing blanks or trailing '/'
    strncpy(cleanpath,path,MAX_PATH);
    int n = strlen(cleanpath)-1;
    while (n >= 0) {
       if (cleanpath[n] != ' ') break;
       n--;
    }

    // if path empty nothing changes
    if (n < 0) {
        _DLOG("SetCurdir '%s'\n",CurDir);
        IF.status = STA_NORMAL;
        return;
    }

    // Get rid of trailing spaces and trailing '/'
    if (cleanpath[n] == '/')
        cleanpath[n] = '\0';
    else
        cleanpath[n+1] = '\0';

    char testpath[MAX_PATH];
    if (*cleanpath == '/') {
        strncpy(testpath,&cleanpath[1],MAX_PATH);
    } else if (*CurDir == '\0') {
        strncpy(testpath,cleanpath,MAX_PATH);
    } else {
        strncpy(testpath,CurDir,MAX_PATH);
        AppendPathChar(testpath,'/');
        strncat(testpath,cleanpath,MAX_PATH);
    }

    // Test new directory
    char fullpath[MAX_PATH];
    strncpy(fullpath,SDCard,MAX_PATH);
    AppendPathChar(fullpath,'/');
    strncat(fullpath,testpath,MAX_PATH);
    if (!IsDirectory(fullpath)) {
        _DLOG("SetCurDir '%s' invalid %s\n",path);
        IF.status = STA_FAIL;
        return;
    }

    // Set new directory
    strncpy(CurDir,testpath,256);
    _DLOG("SetCurdir '%s'\n",CurDir);
    IF.status = STA_NORMAL;
    return;
}

//----------------------------------------------------------------------
//  Start File search.  Searches start from the root of the SDCard.
//----------------------------------------------------------------------
bool SearchFile(const char * pattern)
{
    // Path always starts with SDCard
    char path[MAX_PATH];
    strncpy(path,SDCard,MAX_PATH);
    AppendPathChar(path,'/');

    if (*pattern == '/') {
        strncat(path,&pattern[1],MAX_PATH);
    } else {
        strncat(path,CurDir,MAX_PATH);
        AppendPathChar(path,'/');
        strncat(path,pattern,MAX_PATH);
    }

    //_DLOG("SearchFile %s\n",path);

    // Close previous search
    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }

    // Search
    hFind = FindFirstFile(path, &dFound);

    if (hFind == INVALID_HANDLE_VALUE) {
        *SeaDir = '\0';
        return false;
    } else {
        // Save directory portion for prepending to results
        char * pnam = strrchr(path,'/');
        if (pnam != NULL) pnam[1] = '\0';
        strncpy(SeaDir,path,MAX_PATH);
        return true;
    }

    return (hFind != INVALID_HANDLE_VALUE);
}

//----------------------------------------------------------------------
// InitiateDir command.
//----------------------------------------------------------------------
void InitiateDir(const char * path)
{
    bool rc;
    // Append "*.*" if last char in path was '/';
    int l = strlen(path);
    if (path[l-1] == '/') {
        char tmp[MAX_PATH];
        strncpy(tmp,path,MAX_PATH);
        strncat(tmp,"*.*",MAX_PATH);
        rc = SearchFile(tmp);
    } else {
        rc = SearchFile(path);
    }
    if (rc)
        IF.status = STA_NORMAL;
    else
        IF.status = STA_FAIL;
    return;
}

//----------------------------------------------------------------------
// Load directory page containing up to 16 file records that match
// the pattern used in SearchFile. Can be called multiple times until
// there are no more matches.
//----------------------------------------------------------------------
void LoadDirPage(void)
{
    memset(DirPage,0,sizeof(DirPage));

    if (hFind == INVALID_HANDLE_VALUE) {
         _DLOG("LoadDirPage Search fail\n");
        return;
    }

    int cnt = 0;
    while (cnt < 16) {
        if (dFound.cFileName[0] == '.') {
            // Special cases "." or ".." directory or hidden file
            if (LoadDotRecord(&DirPage[cnt])) cnt++;
        } else {
            if (LoadFindRecord(&DirPage[cnt])) cnt++;
        }
        // Get next match
        if (FindNextFile(hFind,&dFound) == 0) {
            FindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
            break;
        }
    }
}

//----------------------------------------------------------------------
// Found a file that starts with a dot, load it if it is "." or ".."
// Other files that start with '.' are skipped (hidden).
//----------------------------------------------------------------------
bool LoadDotRecord(struct FileRecord * rec)
{
    memset(rec->name,' ',8);
    memset(rec->type,' ',3);
    rec->name[0] = '.';
    rec->attrib = ATTR_DIR;
    if (dFound.cFileName[1] == '\0') return true;
    if (dFound.cFileName[1] == '.' ) rec->name[1] = '.';
    if (dFound.cFileName[2] == '\0') return true;
    // A hidden file - ignore it
    return false;
}

