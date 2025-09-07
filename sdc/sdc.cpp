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
//  $FF40 ; controller latch (write)
//  $FF42 ; flash data register
//  $FF43 ; flash control register
//  $FF48 ; command register (write)
//  $FF48 ; status register (read)
//  $FF49 ; param register 1
//  $FF4A ; param register 2
//  $FF4B ; param register 3
//
//  The FD502 interface uses following ports;
//
//  $FF40 ; Control register (write)
//      Coco uses this for drive select, density, precomp, enable, and motor on
//  $FF48 ; Command register (write)
//      high order nibble; low order nibble type; command
//      0x0 ;   I ; Restore
//      0x1 ;   I ; Seek
//      0x2 ;   I ; Step
//      0x4 ;   I ; Step in
//      0x5 ;   I ; Step out
//      0x8 ;  II ; Read sector
//      0x9 ;  II ; Read sector multiple
//      0xA ;  II ; write sector
//      0xB ;  II ; write sector multiple
//      0xC ; III ; read address
//      0xD ; III ; force interrupt
//      0xE ; III ; read track
//      0xF ;  IV ; write track
//      low order nibble
//      type ; meanings
//        I ; b3 head loaded, b2 track verify , b1-b0 step rate
//       II ; b3 side compare enable, b2 delay, b1 side, b0 0
//      III ; b2 delay others 0
//       IV ; interrupt control b3 immediate, b2 index pulse, b1 notready, b0 ready
//  $FF48 ; Status register (read)
//  $FF49 ; Track register (read/write)
//  $FF4A ; Sector register (read/write)
//  $FF4B ; Data register (read/write)
//
//  Port conflicts are resolved in the MPI by using the SCS (select
//  cart signal) to direct the floppy ports ($FF40-$FF5F) to the
//  selected cartridge slot, either SDC or FD502.
//
//  Sometime in the past the VCC cart select was disabled in mmi.cpp. This
//  had to be be re-enabled for for sdc to co-exist with FD502. Additionally
//  the becker port (drivewire) uses port $FF41 for status and port $FF42
//  for data so these must be always alowed for becker.dll to work. This
//  means sdc.dll requires the new version of mmi.dll to work properly.
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
#pragma warning(push)
#pragma warning(disable:4091)
#include <shlobj.h>
#pragma warning(pop)
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include "../DialogOps.h"
#include "sdc.h"

// from ../pakinterface.h
#define	HEAD 0
#define SLAVE 1
#define STANDALONE 2

//======================================================================
// Functions
//======================================================================

typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
typedef unsigned char (*MEMREAD8)(unsigned short);
typedef void (*MEMWRITE8)(unsigned char,unsigned short);
static void (*DynamicMenuCallback)( char *,int, int)=nullptr;
typedef void (*ASSERTINTERUPT)(unsigned char, unsigned char);
void (*AssertInt)(unsigned char, unsigned char);
void SDCInit(void);
void LoadRom(unsigned char);
void (*MemWrite8)(unsigned char,unsigned short)=nullptr;
void SDCWrite(unsigned char,unsigned char);
void MemWrite(unsigned char,unsigned short);
unsigned char (*MemRead8)(unsigned short)=nullptr;
unsigned char SDCRead(unsigned char port);
unsigned char MemRead(unsigned short);
LRESULT CALLBACK SDC_Control(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SDC_Configure(HWND, UINT, WPARAM, LPARAM);

void LoadConfig(void);
bool SaveConfig(HWND);
void BuildDynaMenu(void);
void CenterDialog(HWND);
void SelectCardBox(void);
void update_disk0_box(void);
void UpdateFlashItem(int);
void ModifyFlashItem(int);
void InitCardBox(void);
void InitEditBoxes(void);
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
bool LoadFoundFile(struct FileRecord *);
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
void SetCurDir(const char *);
bool SearchFile(const char *);
void InitiateDir(const char *);
void GetFullPath(char *,const char *);
void RenameFile(const char *);
void KillFile(const char *);
void MakeDirectory(const char *);
bool IsDirectory(const char *);
void GetMountedImageRec(void);
void GetSectorCount(void);
void GetDirectoryLeaf(void);
void CommandDone(void);
unsigned char PickReplyByte(unsigned char);
unsigned char WriteFlashBank(unsigned short);

void FloppyCommand(unsigned char,unsigned char);
void FloppyRestore(unsigned char,unsigned char);
void FloppySeek(unsigned char,unsigned char);
void FloppyReadDisk(unsigned char,unsigned char);
void FloppyWriteDisk(unsigned char,unsigned char);
void FloppyTrack(unsigned char,unsigned char);
void FloppySector(unsigned char,unsigned char);
void FloppyWriteData(unsigned char,unsigned char);
unsigned char FloppyStatus(unsigned char);
unsigned char FloppyReadData(unsigned char);

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
static Interface IF;

// Cart ROM
char PakRom[0x4000];

// Host paths for SDC
static char IniFile[MAX_PATH] = {};  // Vcc ini file name
static char SDCard[MAX_PATH]  = {};  // SD card root directory
static char CurDir[256]       = {};  // SDC current directory
static char SeaDir[MAX_PATH]  = {};  // Last directory searched

// Packed file records for interface
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
static struct FileRecord DirPage[16];

// Mounted image data
struct _Disk {
    HANDLE hFile;
    unsigned int size;
    unsigned int headersize;
    DWORD sectorsize;
    DWORD tracksectors;
    char doublesided;
    char name[MAX_PATH];
    char fullpath[MAX_PATH];
    struct FileRecord filerec;
} Disk[2];

// Flash banks
static char FlashFile[8][MAX_PATH];
static FILE *h_RomFile = nullptr;
static unsigned char StartupBank = 0;
static unsigned char CurrentBank = 0xff;
static unsigned char EnableBankWrite = 0;
static unsigned char BankWriteNum = 0;
static unsigned char BankWriteState = 0;
static unsigned char BankDirty = 0;
static unsigned char BankData = 0;

// Dll handle
static HINSTANCE hinstDLL;

// Clock enable IDC_CLOCK
static int ClockEnable;

// Windows file lookup handle and data
static HANDLE hFind = INVALID_HANDLE_VALUE;
static WIN32_FIND_DATAA dFound;

// config control handles
static HWND hControlDlg = nullptr;
static HWND hConfigureDlg = nullptr;
//static HWND hFlashBox = nullptr;
static HWND hSDCardBox = nullptr;
static HWND hStartupBank = nullptr;

// Streaming control
static int streaming;
static unsigned char stream_cmdcode;
static unsigned int stream_lsn;

static char Status[16] = {};

// Floppy I/O
static char FlopDrive = 0;
static char FlopTrack = 0;
static char FlopSector = 0;
static char FlopStatus = 0;
static DWORD FlopWrCnt = 0;
static DWORD FlopRdCnt = 0;
static char FlopWrBuf[256];
static char FlopRdBuf[256];

static int EDBOXES[8] = {ID_TEXT0,ID_TEXT1,ID_TEXT2,ID_TEXT3,
                         ID_TEXT4,ID_TEXT5,ID_TEXT6,ID_TEXT7};
static int UPDBTNS[8] = {ID_UPDATE0,ID_UPDATE1,ID_UPDATE2,ID_UPDATE3,
                         ID_UPDATE4,ID_UPDATE5,ID_UPDATE6,ID_UPDATE7};

static char MPIPath[MAX_PATH];

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
        if (DynamicMenuCallback != nullptr) BuildDynaMenu();
        return;
    }

    // Write to port
    __declspec(dllexport) void PackPortWrite(unsigned char Port,unsigned char Data)
    {
        SDCWrite(Data,Port);
        return;
    }

    // Read from port
    __declspec(dllexport) unsigned char PackPortRead(unsigned char Port)
    {
        if (ClockEnable && ((Port==0x78) | (Port==0x79) | (Port==0x7C))) {
            return ReadTime(Port);
        } else if ((Port > 0x3F) & (Port < 0x60)) {
            return SDCRead(Port);
        } else {
            return 0;
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
        case 10:
            if (hConfigureDlg == nullptr)  // Only create dialog once
                hConfigureDlg = CreateDialog(hinstDLL, (LPCTSTR) IDD_CONFIG,
                         GetActiveWindow(), (DLGPROC) SDC_Configure);
            ShowWindow(hConfigureDlg,1);
            break;
        case 11:
            if (hControlDlg == nullptr)
                hControlDlg = CreateDialog(hinstDLL, (LPCTSTR) IDD_CONTROL,
                         GetActiveWindow(), (DLGPROC) SDC_Control);
            ShowWindow(hControlDlg,1);
            break;
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

    // Supply transfer point for interrupt
    __declspec(dllexport) void AssertInterupt(ASSERTINTERUPT Dummy)
    {
        AssertInt=Dummy;
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

    } else if (reason == DLL_PROCESS_DETACH) {
        CloseCartDialog(hControlDlg);
        CloseCartDialog(hConfigureDlg);
        hControlDlg = nullptr;
        hConfigureDlg = nullptr;
        CloseDrive(0);
        CloseDrive(1);
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
    DynamicMenuCallback("SDC Config",5010,STANDALONE);
    DynamicMenuCallback("SDC Control",5011,STANDALONE);
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
    SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

//------------------------------------------------------------
// Control SDC multi floppy
//------------------------------------------------------------
LRESULT CALLBACK
SDC_Control(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CLOSE:
        DestroyWindow(hDlg);
        hControlDlg=nullptr;
        return TRUE;
        break;
    case WM_INITDIALOG:
        CenterDialog(hDlg);
        update_disk0_box();
        SetFocus(GetDlgItem(hDlg,ID_NEXT));
        return TRUE;
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_NEXT:
            MountNext (0);
            SetFocus(GetParent(hDlg));
            return TRUE;
            break;
        }
    }
    return FALSE;
}

//------------------------------------------------------------
// Configure the SDC
//------------------------------------------------------------
LRESULT CALLBACK
SDC_Configure(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CLOSE:
        DestroyWindow(hDlg);
        hConfigureDlg=nullptr;
        return TRUE;
        break;
    case WM_INITDIALOG:
        hConfigureDlg=hDlg;  // needed for LoadConfig() and Init..()
        CenterDialog(hDlg);
        LoadConfig();
        InitEditBoxes();
        InitCardBox();
        SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnable,0);
        hStartupBank = GetDlgItem(hDlg,ID_STARTUP_BANK);
        char tmp[4];
        snprintf(tmp,4,"%d",(StartupBank & 7));
        SetWindowText(hStartupBank,tmp);
        break;
    case WM_COMMAND:
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
            return TRUE;
        case ID_UPDATE0:
            UpdateFlashItem(0);
            return TRUE;
        case ID_UPDATE1:
            UpdateFlashItem(1);
            return TRUE;
        case ID_UPDATE2:
            UpdateFlashItem(2);
            return TRUE;
        case ID_UPDATE3:
            UpdateFlashItem(3);
            return TRUE;
        case ID_UPDATE4:
            UpdateFlashItem(4);
            return TRUE;
        case ID_UPDATE5:
            UpdateFlashItem(5);
            return TRUE;
        case ID_UPDATE6:
            UpdateFlashItem(6);
            return TRUE;
        case ID_UPDATE7:
            UpdateFlashItem(7);
            return TRUE;
        case ID_TEXT0:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(0);
            return TRUE;
        case ID_TEXT1:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(1);
            return FALSE;
        case ID_TEXT2:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(2);
            return FALSE;
        case ID_TEXT3:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(3);
            return FALSE;
        case ID_TEXT4:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(4);
            return FALSE;
        case ID_TEXT5:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(5);
            return FALSE;
        case ID_TEXT6:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(6);
            return FALSE;
        case ID_TEXT7:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(7);
            return FALSE;
        case ID_STARTUP_BANK:
            if (HIWORD(wParam) == EN_CHANGE) {
                char tmp[4];
                GetWindowText(hStartupBank,tmp,4);
                StartupBank = atoi(tmp);// & 7;
                if (StartupBank > 7) {
                    StartupBank &= 7;
                    char tmp[4];
                    snprintf(tmp,4,"%d",StartupBank);
                    SetWindowText(hStartupBank,tmp);
                }
            }
            break;
        case IDOK:
            SaveConfig(hDlg);
            DestroyWindow(hDlg);
            hConfigureDlg=nullptr;
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
        ("DefaultPaths", "MPIPath", "", MPIPath, MAX_PATH, IniFile);
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

    // Init the hFind handle (otherwise could crash on dll load)
    hFind = INVALID_HANDLE_VALUE;

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
void InitEditBoxes(void)
{
    for (int index=0; index<8; index++) {
        HWND h;
        h = GetDlgItem(hConfigureDlg,EDBOXES[index]);
        SetWindowText(h,FlashFile[index]);
        h = GetDlgItem(hConfigureDlg,UPDBTNS[index]);
        if (*FlashFile[index] == '\0') {
            SetWindowText(h,">");
        } else {
            SetWindowText(h,"X");
        }
    }
}

//----------------------------------------------------------------------
// Put disk 0 name to control dialog
//----------------------------------------------------------------------
void update_disk0_box()
{ 
    if (hControlDlg != nullptr) {
        HWND h = GetDlgItem(hControlDlg,ID_DISK0);
        SendMessage(h, WM_SETTEXT, 0, (LPARAM) Disk[0].name );
    }
}

//------------------------------------------------------------
// Init SD card box
//------------------------------------------------------------

void InitCardBox(void)
{
    hSDCardBox = GetDlgItem(hConfigureDlg,ID_SD_BOX);
    SendMessage(hSDCardBox, WM_SETTEXT, 0, (LPARAM)SDCard);
}

//------------------------------------------------------------
// Modify and or Update flash box item
//------------------------------------------------------------
void ModifyFlashItem(int index)
{
    if ((index < 0) | (index > 7)) return;
    HWND h = GetDlgItem(hConfigureDlg,EDBOXES[index]);
    GetWindowText(h, FlashFile[index], MAX_PATH);
//    UpdateFlashItem(index);
}

void UpdateFlashItem(int index)
{
    char filename[MAX_PATH]={};

    if ((index < 0) | (index > 7)) return;

    if (*FlashFile[index] != '\0') {
        *FlashFile[index] = '\0';
    } else {
        char title[64];
        snprintf(title,64,"Load Flash Bank %d",index);
        FileDialog dlg;
        dlg.setDefExt("rom");
        dlg.setFilter("Rom File\0*.rom\0All Files\0*.*\0\0");
        dlg.setTitle(title);
        dlg.setInitialDir(MPIPath);   // FIXME someday
        if (dlg.show(0,hConfigureDlg)) {
            dlg.getupath(filename,MAX_PATH); // cvt to unix style
            strncpy(FlashFile[index],filename,MAX_PATH);
        }
    }
    InitEditBoxes();
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
        (nullptr,CSIDL_PROFILE,(LPITEMIDLIST *) &bi.pidlRoot);

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
        h_RomFile = nullptr;
    }

    if (BankDirty) {
        RomFile = FlashFile[CurrentBank];
        _DLOG("LoadRom switching out dirty bank %d %s\n",CurrentBank,RomFile);
        h_RomFile = fopen(RomFile,"wb");
        if (h_RomFile == nullptr) {
            _DLOG("LoadRom failed to open bank file%d\n",bank);
        } else {
            ctr = 0;
            p_rom = PakRom;
            while (ctr++ < 0x4000) fputc(*p_rom++, h_RomFile);
            fclose(h_RomFile);
            h_RomFile = nullptr;
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
    if (h_RomFile == nullptr) {
        if (CurrentBank != StartupBank) h_RomFile = fopen(RomFile,"wb");
    }
    if (h_RomFile == nullptr) {
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
    if (su == nullptr) {
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
//  Command done interrupt;
//----------------------------------------------------------------------
void CommandDone(void)
{
    _DLOG("*");
	AssertInt(INT_NMI,IS_NMI);
}

//----------------------------------------------------------------------
// Write port.  If a command needs a data block to complete it
// will put a count (256 or 512) in IF.bufcnt.
//----------------------------------------------------------------------
void SDCWrite(unsigned char data,unsigned char port)
{
    if (port < 0x40 || port > 0x4F) return;

    if (IF.sdclatch) {
        switch (port) {
        // Control Latch
        case 0x40:
            if (IF.sdclatch) memset(&IF,0,sizeof(IF));
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
        // Unhandled
        default:
            _DLOG("SDCWrite L %02x %02x\n",port,data);
            break;
        }
    } else {
        switch (port) {
        // Command latch and floppy drive select
        case 0x40:
            if (data == 0x43) {
                IF.sdclatch = true;
            } else {
                int tmp = data & 0x47;
                if (tmp == 1) {
                    FlopDrive = 0;
                } else if (tmp == 2) {
                    FlopDrive = 1;
                }
            }
            break;
         // Flash Data
        case 0x42:
            BankData = data;
            break;
        // Flash Control
        case 0x43:
            FlashControl(data);
            break;
        // floppy command
        case 0x48:
            FloppyCommand(port,data);
            break;
        // floppy set track
        case 0x49:
            FloppyTrack(port,data);
            break;
        // floppy set sector
        case 0x4A:
            FloppySector(port,data);
            break;
        // floppy write data
        case 0x4B:
            FloppyWriteData(port,data);
            break;
        // Unhandled
        default:
            _DLOG("SDCWrite U %02x %02x\n",port,data);
            break;
        }
    }
    return;
}

//----------------------------------------------------------------------
// Read port. If there are bytes in the reply buffer return them.
//----------------------------------------------------------------------
unsigned char SDCRead(unsigned char port)
{
    unsigned char rpy = 0;

    if (IF.sdclatch) {
        switch (port) {
        case 0x48:
            if (IF.bufcnt > 0) {
                rpy = (IF.reply_status != 0) ? IF.reply_status:STA_BUSY|STA_READY;
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
            _DLOG("SDCRead L %02x\n",port);
            rpy = 0;
            break;
        }
    } else {
        switch (port) {
        // Flash control read is used by SDCDOS to detect the SDC
        case 0x43:
            rpy = CurrentBank | (BankData & 0xF8);
            break;
        // Floppy read status
        case 0x48:
            rpy = FloppyStatus(port);
            break;
        // Floppy read data
        case 0x4B:
            rpy = FloppyReadData(port);
            break;
        default:
            _DLOG("SDCRead U %02x\n",port);
            rpy = 0;
            break;
        }
    }
    return rpy;
}

//----------------------------------------------------------------------
// Floppy I/O
//----------------------------------------------------------------------
void FloppyCommand(unsigned char port,unsigned char data)
{
    switch (data >> 4) {
    // floppy restore
    case 0x0:
        FloppyRestore(port,data);
        break;
    case 0x1:
        FloppySeek(port,data);
        break;
    // floppy read sector
    case 0x8:
        FloppyReadDisk(port,data);
        break;
    // floppy write sector
    case 0xA:
        FloppyWriteDisk(port,data);
        break;
    }
}

// floppy restore
void FloppyRestore(unsigned char port,unsigned char data)
{
    _DLOG("FloppyRestore %02x %02x\n",port,data);
    FlopTrack = 0;
    FlopSector = 0;
    FlopStatus = FLP_NORMAL;
    FlopWrCnt = 0;
    FlopRdCnt = 0;
    CommandDone();
}

// floppy seek
void FloppySeek(unsigned char port,unsigned char data)
{
    _DLOG("FloppySeek %02x %02x\n",port,data);
}

// floppy read sector
void FloppyReadDisk(unsigned char port,unsigned char data)
{
    int lsn = FlopTrack * 18 + FlopSector - 1;
    snprintf(Status,16,"SDC:%d Rd %d,%d",CurrentBank,FlopDrive,lsn);
    if (SeekSector(FlopDrive,lsn)) {
        if (ReadFile(Disk[FlopDrive].hFile,FlopRdBuf,256,&FlopRdCnt,nullptr)) {
            _DLOG("FloppyReadDisk %d %d\n",FlopDrive,lsn);
            FlopStatus = FLP_DATAREQ;
        } else {
            _DLOG("FloppyReadDisk read error %d %d\n",FlopDrive,lsn);
            FlopStatus = FLP_READERR;
        }
    } else {
        _DLOG("FloppyReadDisk seek error %d %d\n",FlopDrive,lsn);
        FlopStatus = FLP_SEEKERR;
    }
}

// floppy write sector
void FloppyWriteDisk(unsigned char port,unsigned char data)
{
    int lsn = FlopTrack * 18 + FlopSector - 1;
    // write not implemented yet
    _DLOG("FloppyWriteDisk %d %d not implmented\n",FlopDrive,lsn);
    FlopStatus = FLP_READONLY;
}

// floppy set track
void FloppyTrack(unsigned char port,unsigned char data)
{
    _DLOG("FloppyTrack %d\n",data);
    FlopTrack = data;
}

// floppy set sector
void FloppySector(unsigned char port,unsigned char data)
{
    FlopSector = data;  // (1-18)

    int lsn = FlopTrack * 18 + FlopSector - 1;
    _DLOG("FloppySector %d lsn %d\n",FlopSector,lsn);
    FlopStatus = FLP_NORMAL;
}

// floppy write data
void FloppyWriteData(unsigned char port,unsigned char data)
{
    _DLOG("FloppyWriteData %02x %02x\n",port,data);
    if (FlopWrCnt<256)  {
        FlopWrCnt++;
        FlopWrBuf[FlopWrCnt] = data;
        FlopStatus |= FLP_DATAREQ;
    } else {
        FlopStatus = FLP_NORMAL;
    }
}

// floppy get status
unsigned char FloppyStatus(unsigned char port)
{
    _DLOG("FloppyStatus %02x\n",FlopStatus);
    return FlopStatus;
}

// floppy read data
unsigned char FloppyReadData(unsigned char port)
{
    unsigned char rpy;
    if (FlopRdCnt>0)  {
        rpy = FlopRdBuf[256-FlopRdCnt];
        FlopRdCnt--;
        FlopStatus |= FLP_DATAREQ;
    } else {
        FlopStatus = FLP_NORMAL;
        CommandDone();
        rpy = 0;
    }
    _DLOG("FloppyReadData %03d %02x\n",256-FlopRdCnt,rpy);
    return rpy;
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
        if (p == nullptr) {
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
    switch (IF.blkbuf[0]) {
    case 0x4D: //M
        MountDisk(IF.cmdcode&1,&IF.blkbuf[2],0);
        break;
    case 0x6D: //m
        MountDisk(IF.cmdcode&1,&IF.blkbuf[2],1);
        break;
    case 0x4E: //N
        MountNewDisk(IF.cmdcode&1,&IF.blkbuf[2],0);
        break;
    case 0x6E: //n
        MountNewDisk(IF.cmdcode&1,&IF.blkbuf[2],1);
        break;
    case 0x44: //D
        SetCurDir(&IF.blkbuf[2]);
        break;
    case 0x4C: //L
        InitiateDir(&IF.blkbuf[2]);
        break;
    case 0x4B: //K
        MakeDirectory(&IF.blkbuf[2]);
        break;
    case 0x52: //R
        RenameFile(&IF.blkbuf[2]);
        break;
    case 0x58: //X
        KillFile(&IF.blkbuf[2]);
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
    if (!SetFilePointerEx(Disk[drive].hFile,pos,nullptr,FILE_BEGIN)) {
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
    if (Disk[drive].hFile == nullptr) {
        _DLOG("ReadDrive %d not open\n");
        return false;
    }

    if (!SeekSector(cmdcode,lsn)) {
        return false;
    }

    if (!ReadFile(Disk[drive].hFile,buf,Disk[drive].sectorsize,&cnt,nullptr)) {
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

    _DLOG("R%d\n",lsn);

    IF.reply_mode = ((IF.cmdcode & 4) == 0) ? 0 : 1; // words : bytes
    if (!ReadDrive(IF.cmdcode,lsn))
        IF.status = STA_FAIL | STA_READERROR;
    else
        IF.status = STA_NORMAL;
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

    if (Disk[drive].hFile == nullptr) {
        IF.status = STA_FAIL;
        return;
    }

    if (!SeekSector(drive,lsn)) {
        IF.status = STA_FAIL;
        return;
    }
    if (!WriteFile(Disk[drive].hFile,IF.blkbuf,
                   Disk[drive].sectorsize,&cnt,nullptr)) {
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
    //_DLOG("GetMountedImageRec %d %s\n",drive,Disk[drive].fullpath);
    if (strlen(Disk[drive].fullpath) == 0) {
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
    if (pname8 != nullptr) {
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
bool LoadFoundFile(struct FileRecord * rec)
{
    memset(rec,0,sizeof(rec));
    memset(rec->name,' ',8);
    memset(rec->type,' ',3);

    // Special case filename starts with a dot
    if (dFound.cFileName[0] == '.' ) {
        // Don't load if current directory is SD root,
        // is only one dot, or if more than two chars
        if ((*CurDir=='\0') |
            (dFound.cFileName[1] != '.' ) |
            (dFound.cFileName[2] != '\0'))
            return false;
        rec->name[0]='.';
        rec->name[1]='.';
        rec->attrib = ATTR_DIR;
        return true;
    }

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
    if (!found && (strchr(file,'.') == nullptr)) {
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
        _DLOG("MountDisk not found '%s'\n",file);
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

    // Trying to mount a directory. Find .DSK files in the ending
    // with a digit and mount the first one.
    if (IsDirectory(fqn)) {
        _DLOG("OpenNew %s is a directory\n",fqn);
        IF.status = STA_FAIL | STA_INVALID;
        return;
    }

    // Try to create file
    CloseDrive(drive);
    strncpy(Disk[drive].fullpath,fqn,MAX_PATH);

    // Open file for write
    Disk[drive].hFile = CreateFile(
        Disk[drive].fullpath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
		nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);

    if (Disk[drive].hFile == INVALID_HANDLE_VALUE) {
        _DLOG("OpenNew fail %d file %s\n",drive,Disk[drive].fullpath);
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
        if (SetFilePointerEx(Disk[drive].hFile,l_siz,nullptr,FILE_BEGIN)) {
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
        drive, dFound.cFileName, Disk[drive].hFile);

    CloseDrive(drive);
    *Disk[drive].name = '\0';

    // Open a directory of containing DSK files
    if (dFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        _DLOG("OpenFound %s is a directory\n",dFound.cFileName);
        char path[MAX_PATH];
        strncpy(path,dFound.cFileName,MAX_PATH);
        AppendPathChar(path,'/');
        strncat(path,"*.DSK",MAX_PATH);
        InitiateDir(path);
        OpenFound(drive,0);
        return;
    }

    // Fully qualify name of found file and try to open it
    char fqn[MAX_PATH]={};
    strncpy(fqn,SeaDir,MAX_PATH);
    strncat(fqn,dFound.cFileName,MAX_PATH);

    // Open file for read
    Disk[drive].hFile = CreateFile(
        fqn, GENERIC_READ, FILE_SHARE_READ,
		nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (Disk[drive].hFile == INVALID_HANDLE_VALUE) {
        _DLOG("OpenFound fail %d file %s\n",drive,fqn);
        _DLOG("... %s\n",LastErrorTxt());
        int ecode = GetLastError();
        if (ecode == ERROR_SHARING_VIOLATION) {
            IF.status = STA_FAIL | STA_INUSE;
        } else {
            IF.status = STA_FAIL | STA_WIN_ERROR;
        }
        return;
    }

    strncpy(Disk[drive].fullpath,fqn,MAX_PATH);
    strncpy(Disk[drive].name,dFound.cFileName,MAX_PATH);

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

        // Read a few bytes of the file to determine it's type
        unsigned char header[16];
        if (ReadFile(Disk[drive].hFile,header,12,nullptr,nullptr) == 0) {
            _DLOG("OpenFound header read error\n");
            IF.status = STA_FAIL | STA_INVALID;
            return;
        }

        // Check for SDF file. SDF file has a 512 byte header and each
        // track record is 6250 bytes. Assume at least 35 tracks so
        // minimum size of a SDF file is 219262 bytes. The first four
        // bytes of the header contains "SDF1"
        if ((Disk[drive].size >= 219262) &&
            (strncmp("SDF1",(const char *) header,4) == 0)) {
            _DLOG("OpenFound SDF file unsupported\n");
            IF.status = STA_FAIL | STA_INVALID;
            return;
        }

        unsigned int numsec;
        Disk[drive].headersize = Disk[drive].size & 255;
        switch (Disk[drive].headersize) {
        // JVC optional header bytes
        case 4: // First Sector     = header[3]   1 assumed
        case 3: // Sector Size code = header[2] 256 assumed {128,256,512,1024}
        case 2: // Number of sides  = header[1]   1 or 2
        case 1: // Sectors per trk  = header[0]  18 assumed
            Disk[drive].doublesided = (header[1] == 2);
            break;
        // No apparant header
        // JVC or OS9 disk if no header, side count per file size
        case 0:
            numsec = Disk[drive].size >> 8;
            Disk[drive].doublesided = ((numsec > 720) && (numsec <= 2880));
            break;
        // VDK
        case 12:
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
    LoadFoundFile(&Disk[drive].filerec);

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
            Disk[drive].fullpath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
			nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if (Disk[drive].hFile == INVALID_HANDLE_VALUE) {
            _DLOG("OpenFound reopen fail %d\n",drive);
            _DLOG("... %s\n",LastErrorTxt());
            IF.status = STA_FAIL | STA_WIN_ERROR;
            return;
        }
    }

    if (drive == 0) update_disk0_box();

    IF.status = STA_NORMAL;
    return;
}

//----------------------------------------------------------------------
// Convert file name from SDC format and prepend current dir.
//----------------------------------------------------------------------
void GetFullPath(char * path, const char * file) {
    char tmp[MAX_PATH];
    strncpy(path,SDCard,MAX_PATH);
    AppendPathChar(path,'/');
    strncat(path,CurDir,MAX_PATH);
    AppendPathChar(path,'/');
    FixSDCPath(tmp,file);
    strncat(path,tmp,MAX_PATH);
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

    GetFullPath(from,names);
    GetFullPath(target,1+strchr(names,'\0'));

    _DLOG("UpdateSD rename %s %s\n",from,target);

    if (std::rename(from,target)) {
        _DLOG("RenameFile %s\n", strerror(errno));
        IF.status = STA_FAIL | STA_WIN_ERROR;
    } else {
        IF.status = STA_NORMAL;
    }
    return;
}

//----------------------------------------------------------------------
// Delete disk or directory
//----------------------------------------------------------------------
void KillFile(const char *file)
{
    char path[MAX_PATH];
    GetFullPath(path,file);
    _DLOG("KillFile delete %s\n",path);

    if (IsDirectory(path)) {
        if (PathIsDirectoryEmpty(path)) {
            if (RemoveDirectory(path)) {
                IF.status = STA_NORMAL;
            } else {
                _DLOG("Deletefile %s\n", strerror(errno));
                IF.status = STA_FAIL | STA_NOTFOUND;
            }
        } else {
            IF.status = STA_FAIL | STA_NOTEMPTY;
        }
    } else {
        if (DeleteFile(path))
            IF.status = STA_NORMAL;
        else
            _DLOG("Deletefile %s\n", strerror(errno));
            IF.status = STA_FAIL | STA_NOTFOUND;
    }
    return;
}

//----------------------------------------------------------------------
// Create directory
//----------------------------------------------------------------------
void MakeDirectory(const char *name)
{
    char path[MAX_PATH];
    GetFullPath(path,name);
    _DLOG("MakeDirectory %s\n",path);

    // Make sure directory is not in use
    struct _stat file_stat;
    int result = _stat(path,&file_stat);
    if (result == 0) {
        IF.status = STA_FAIL | STA_INVALID;
        return;
    }

    if (CreateDirectory(path,nullptr)) {
        IF.status = STA_NORMAL;
    } else {
        _DLOG("MakeDirectory %s\n", strerror(errno));
        IF.status = STA_FAIL | STA_WIN_ERROR;
    }
    return;
}

//----------------------------------------------------------------------
// Close virtual disk
//----------------------------------------------------------------------
void CloseDrive (int drive)
{
    drive &= 1;
    if (Disk[drive].hFile != nullptr) {
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
// This is complicated by the many ways a user can change the directory
//----------------------------------------------------------------------
void SetCurDir(const char * branch)
{
    _DLOG("SetCurdir '%s'\n",branch);

    // If branch is "." or "" do nothing
    if ((*branch == '\0') || (strcmp(branch,".") == 0)) {
        _DLOG("SetCurdir no change\n");
        IF.status = STA_NORMAL;
        return;
    }

    // If branch is "/" set CurDir to root
    if (strcmp(branch,"/") == 0) {
        _DLOG("SetCurdir to root\n");
        *CurDir = '\0';
        IF.status = STA_NORMAL;
        return;
    }

    // If branch is ".." go back a directory
    if (strcmp(branch,"..") == 0) {
        char *p = strrchr(CurDir,'/');
        if (p != nullptr) {
            *p = '\0';
        } else {
            *CurDir = '\0';
        }
        _DLOG("SetCurdir back %s\n",CurDir);
        IF.status = STA_NORMAL;
        return;
    }

    // Disallow branch start with "//"
    if (strncmp(branch,"//",2) == 0) {
        _DLOG("SetCurdir // invalid\n");
        IF.status = STA_FAIL | STA_INVALID;
        return;
    }

    // Test for CurDir relative branch
    int relative = (*branch != '/');

    // Test the full directory path
    char test[MAX_PATH];
    strncpy(test,SDCard,MAX_PATH);
    AppendPathChar(test,'/');
    if (relative) {
        strncat(test,CurDir,MAX_PATH);
        AppendPathChar(test,'/');
        strncat(test,branch,MAX_PATH);
    } else {
        strncat(test,branch+1,MAX_PATH);
    }
    if (!IsDirectory(test)) {
        _DLOG("SetCurdir not a directory %s\n",test);
        IF.status = STA_FAIL | STA_NOTFOUND;
        return;
    }

    // Set current directory
    if (relative) {
        if (*CurDir != '\0') AppendPathChar(CurDir,'/');
        strncat(CurDir,branch,MAX_PATH);
    } else {
        strncpy(CurDir,branch+1,MAX_PATH);
    }

    // Trim trailing '/'
    int l = strlen(CurDir);
    while (l > 0 && CurDir[l-1] == '/') l--;
    CurDir[l] = '\0';

    _DLOG("SetCurdir set to '%s'\n",CurDir);

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

    _DLOG("SearchFile %s\n",path);

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
        if (pnam != nullptr) pnam[1] = '\0';
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
        IF.status = STA_FAIL | STA_INVALID;
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
        if (LoadFoundFile(&DirPage[cnt])) cnt++;
        if (FindNextFile(hFind,&dFound) == 0) {
            FindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
            break;
        }
    }
}
