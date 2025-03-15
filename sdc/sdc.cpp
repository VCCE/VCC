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
//  to co-exist with FD502. This means sdc.dll requires the new version
//  of mmi.dll to work properly.
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
//  disk the SDC User Guide for detailed command descriptions. It is
//  likely that some commands mentioned there are not yet supported
//  by this interface.
//
//  Flash data
//  ----------
//  Data written to the flash data port (0x42) can be read back.
//  When the flash data port (0x43) is written the three low bits
//  select the ROM from the corresponding flash bank (0-7). When
//  read the flash control port returns the currently selected bank
//  in the three low bits and the five bits from the Flash Data port.
//
//  There are eight 16K banks of programable flash memory. In this
//  emulator SDC-DOS is typically in bank 0 and disk basic into bank 1.
//  However these ROMS require their respective .DLL's to be installed
//  to function, these are typically in MMI slot 4 and 3 respectively.
//  The SDC RUN@n command will load an image from one of the 16K banks
//  into the currently selected MMI slot then reset VCC, which will start
//  the rom.
//
//  File Status bits:
//  0   Busy
//  1   Ready
//  2   Invalid path name
//  3   hardware error
//  4   Path not found
//  5   File in use
//  6   not used
//  7   Command failed
//
//----------------------------------------------------------------------

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>

#include "../defines.h"
#include "resource.h"
#include "cloud9.h"

// Debug logging if _DEBUG_ is defined
#define _DEBUG_
#ifdef _DEBUG_
#include "../logger.h"
#define _DLOG(...) PrintLogC(__VA_ARGS__)
#else
#define _DLOG(...)
#endif

// Return values for status register
#define STA_BUSY     0x01
#define STA_READY    0x02
#define STA_HWERROR  0x04
#define STA_CRCERROR 0x08
#define STA_NOTFOUND 0x10
#define STA_DELETED  0x20
#define STA_WPROTECT 0x40
#define STA_FAIL     0x80

// Single byte file attributes for File info records
#define ATTR_NORM    0x00
#define ATTR_RDONLY  0x01
#define ATTR_HIDDEN  0x02
#define ATTR_SDF     0x04
#define ATTR_DIR     0x10
#define ATTR_INVALID -1  //not 0xFF because byte is signed

//======================================================================
// Functions
//======================================================================

bool LoadRom(char *);
void SDCWrite(unsigned char data,unsigned char port);
unsigned char SDCRead(unsigned char port);
void SDCInit(void);

void AssertInterupt(unsigned char,unsigned char);
void MemWrite(unsigned char,unsigned short);
unsigned char MemRead(unsigned short);

static char IniFile[MAX_PATH]={0};  // Ini file name from config

typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
typedef unsigned char (*MEMREAD8)(unsigned short);
typedef void (*MEMWRITE8)(unsigned char,unsigned short);

static void (*AssertInt)(unsigned char,unsigned char)=NULL;
static void (*DynamicMenuCallback)( char *,int, int)=NULL;
static unsigned char (*MemRead8)(unsigned short)=NULL;
static void (*MemWrite8)(unsigned char,unsigned short)=NULL;

LRESULT CALLBACK SDC_Config(HWND, UINT, WPARAM, LPARAM);

void LoadConfig(void);
void SaveConfig(void);
void BuildDynaMenu(void);
void UpdateListBox(HWND);
void UpdateCardBox(void);
void UpdateFlashItem(void);
void InitCardBox(void);
void InitFlashBox(void);
void ParseStartup(void);
void SDCCommand(void);
void ReadSector(void);
void StreamImage(void);
void WriteSector(void);
void GetDriveInfo(void);
void SDCControl(void);
void UpdateSD(void);
void AppendPathChar(char *,char c);
int FileAttrib(const char *,char *);
bool LoadFileRecord(char *,struct FileRecord *);
bool MountDisk (int,const char *);
void CloseDrive(int);
void OpenDrive(int);
void LoadReply(void *, int, int);
void BlockReceive(unsigned char);
char * LastErrorTxt(void);
void BankSelect(int);
bool LoadDirPage(void);
bool SetCurDir(char * path);
bool InitiateDir(const char *);
bool IsDirectory(const char *path);
void GetMountedImageRec(void);
void CvtName83(char *,char *,char *);
void GetSectorCount(void);

//======================================================================
// Globals
//======================================================================

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
    unsigned char flash;
    int bufcnt;
    char *bufptr;
    char blkbuf[600];
} Interface;
Interface IF;

// Cartridge ROM
char PakRom[0x4000];

// Rom bank currently selected
char CurrentBank = 0;

// Host paths for SDC
char SDCard[MAX_PATH] = {}; // SD card root directory
char CurDir[MAX_PATH] = {}; // SDC current directory
char SeaDir[MAX_PATH] = {}; // Last directory searched

// Packed file record for sending to CoCo
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

// Mounted image data
struct _DiskImage {
    char name[MAX_PATH];
    HANDLE hFile;
    char attrib;
    int size;
} DiskImage[2];

// Flash banks
char FlashFile[8][MAX_PATH];

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

    // Capture the Fuction transfer point for the CPU assert interupt
    __declspec(dllexport) void AssertInterupt(ASSERTINTERUPT Dummy)
    {
        AssertInt=Dummy;
        return;
    }

    // Return SDC status.
    __declspec(dllexport) void ModuleStatus(char *MyStatus)
    {
        strcpy(MyStatus,"SDC");
        return ;
    }

    // Set ini file path and Initialize SDC
    __declspec(dllexport) void SetIniPath (char *IniFilePath)
    {
        strcpy(IniFile,IniFilePath);
        return;
    }

    // Return a byte from the current PAK ROM
    __declspec(dllexport) unsigned char PakMemRead8(unsigned short adr)
    {
        return(PakRom[adr & 0x3FFFF]);
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
    } else if (reason == DLL_PROCESS_DETACH) {
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
        hConfDlg=hDlg;
        InitFlashBox();
        InitCardBox();
        SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnable,0);
        break;
    case WM_VKEYTOITEM:
        switch (LOWORD(wParam)) {
        case VK_RETURN:
        case VK_SPACE:
            UpdateListBox((HWND) lParam);
            return -2; //no further processing
        }
        return -1;
    case WM_COMMAND:
        switch (HIWORD(wParam)) {
        case LBN_KILLFOCUS:
            SendMessage((HWND)lParam, LB_SETCURSEL, -1, 0);
            return 0;
        case LBN_DBLCLK:
            UpdateListBox((HWND) lParam);
            return 0;
        }
        switch (LOWORD(wParam)) {
        case IDOK:
            ClockEnable = (unsigned char)
                SendDlgItemMessage(hDlg,IDC_CLOCK,BM_GETCHECK,0,0);
            SaveConfig();
            EndDialog(hDlg,LOWORD(wParam));
            break;
        case IDCANCEL:
            EndDialog(hDlg,LOWORD(wParam));
            break;
        }
    }
    return 0;
}

//------------------------------------------------------------
// Get configuration items from ini file
//------------------------------------------------------------
void LoadConfig(void)
{
    GetPrivateProfileString
        ("SDC", "SDCardPath", "", SDCard, MAX_PATH, IniFile);
    if (!IsDirectory(SDCard)) {
        _DLOG("LoadConfig invalid SDCard path %s\n",SDCard);
        return;
    }

    for (int i=0;i<8;i++) {
        char txt[32];
        sprintf(txt,"FlashFile_%d",i);
        GetPrivateProfileString
            ("SDC", txt, "", FlashFile[i], MAX_PATH, IniFile);
    }

    ClockEnable = GetPrivateProfileInt("SDC","ClockEnable",1,IniFile);
}

//------------------------------------------------------------
// Save config to ini file
//------------------------------------------------------------
void SaveConfig(void)
{
    WritePrivateProfileString("SDC", "SDCardPath",SDCard,IniFile);
    for (int i=0;i<8;i++) {
        char txt[32];
        sprintf(txt,"FlashFile_%d",i);
        WritePrivateProfileString("SDC",txt,FlashFile[i],IniFile);
    }
    if (ClockEnable)
        WritePrivateProfileString("SDC","ClockEnable","1",IniFile);
    else
        WritePrivateProfileString("SDC","ClockEnable","0",IniFile);
    return;
}

//----------------------------------------------------------------------
// Init the controller. This gets called by ModuleReset
//----------------------------------------------------------------------
void SDCInit(void)
{

#ifdef _DEBUG_
    _DLOG("SDCInit\n");
    MoveWindow(GetConsoleWindow(),0,0,300,800,TRUE);
#endif

    // Make sure drives are unloaded
    MountDisk (0,"");
    MountDisk (1,"");

    // Load SDC configuration from ini file
    LoadConfig();

    memset((void *) &DiskImage,0,sizeof(DiskImage));

    ParseStartup();
    SetCurDir("");
    CurrentBank = 0;

    // init the interface
    memset(&IF,0,sizeof(IF));

    //_DLOG("SDCInit loading SDC-DOS Rom\n");
    LoadRom("SDC-DOS.ROM");
    return;
}

//------------------------------------------------------------
// Init flash box
//------------------------------------------------------------
void InitFlashBox(void)
{
    hFlashBox = GetDlgItem(hConfDlg,ID_FLASH_BOX);

    // Set height of items in the listbox
    SendMessage(hFlashBox, LB_SETITEMHEIGHT, 0, 18);

    // Add items to the listbox
    char text[64];
    for (int index=0; index<8; index++) {
        sprintf(text,"%d  %s",index,FlashFile[index]);
        SendMessage(hFlashBox, LB_ADDSTRING, 0, (LPARAM)text);
    }
}

//------------------------------------------------------------
// Init SD card box
//------------------------------------------------------------
void InitCardBox(void)
{
    hSDCardBox = GetDlgItem(hConfDlg,ID_SD_BOX);
    SendMessage(hSDCardBox, LB_SETITEMHEIGHT, 0, 18);
    SendMessage(hSDCardBox, LB_ADDSTRING, 0, (LPARAM)SDCard);
}

//------------------------------------------------------------
// Update flash box item
//------------------------------------------------------------
void UpdateFlashItem(void)
{
    char filename[MAX_PATH];

    int index = SendMessage(hFlashBox, LB_GETCURSEL, 0, 0);
    if ((index < 0) | (index > 7)) {
        _DLOG("UpdateFlashItem invalid index %d\n",index);
        return;
    }

    OPENFILENAME ofn ;

    char TempFileName[MAX_PATH]="";
    char CapFilePath[MAX_PATH]="C:/users";
    sprintf(filename,"%d  -empty-",index);

    memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize       = sizeof (OPENFILENAME);
    ofn.hwndOwner         = hFlashBox;          // hConfigDlg?
    ofn.Flags             = OFN_HIDEREADONLY;
    ofn.lpstrDefExt       = ".rom";
    ofn.lpstrFilter       = "Rom File\0*.rom\0All Files\0*.*\0\0";
    ofn.nFilterIndex      = 0 ;
    ofn.lpstrFile         = &filename[3];
    ofn.nMaxFile          = MAX_PATH-3;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrInitialDir   = CapFilePath;
    ofn.lpstrTitle        = "Set Flash Rom file";

    GetOpenFileName(&ofn);

    for(unsigned int i=0; i<strlen(filename); i++) {
        if (filename[i] == '\\') filename[i] = '/';
    }

    strncpy(FlashFile[index],&filename[3],MAX_PATH);

    _DLOG("UdateFlashItem %d %s\n",index,FlashFile[index]);

    // Delete prev string then add new string in it's place
    SendMessage(hFlashBox, LB_DELETESTRING, index, 0);
    SendMessage(hFlashBox, LB_INSERTSTRING, index, (LPARAM)filename);
    SendMessage(hFlashBox, LB_SETCURSEL, index, 0);
}

//------------------------------------------------------------
// Update SD card
//------------------------------------------------------------
void UpdateCardBox(void)
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
    for(unsigned int i=0; i<strlen(SDCard); i++) {
        if (SDCard[i] == '\\') SDCard[i] = '/';
    }
    AppendPathChar(SDCard,'/');

    // Make sure it  is a directory
    if (!IsDirectory(SDCard)) {
        _DLOG("UpdateCardBox invalid path %s\n",SDCard);
        return;
    }
    // Display selection in listbox
    SendMessage(hSDCardBox, LB_DELETESTRING, 0, 0);
    SendMessage(hSDCardBox, LB_INSERTSTRING, 0, (LPARAM)SDCard);
    SendMessage(hSDCardBox, LB_SETCURSEL, 0, 0);
}

//------------------------------------------------------------
// Update a list box
//------------------------------------------------------------
void UpdateListBox(HWND hBox)
{
    if (hBox == hFlashBox)
        UpdateFlashItem();
    else if (hBox == hSDCardBox)
        UpdateCardBox();
}

//-------------------------------------------------------------
// Load SDC rom
//-------------------------------------------------------------

bool LoadRom(char *RomName) //Returns true if loaded
{
    int ch;
    int ctr = 0;
    FILE *h = fopen(RomName, "rb");
    memset(PakRom, 0xFF, 0x4000);
    if (h == NULL) return false;

    char *p = PakRom;
    while (ctr++ < 0x4000) {
        if ((ch = fgetc(h)) < 0) break;
        *p++ = (char) ch;
    }

    fclose(h);
    return true;
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
    strncat(buf,"startup.cfg",MAX_PATH);

    FILE *su = fopen(buf,"r");
    if (su == NULL) {
        _DLOG("ParseStartup file not found,%s\n",buf);
        return;
    }

    //TODO implement D=[Current directory]
    // Strict single digit followed by '=' then path
    while (fgets(buf,sizeof(buf),su) > 0) {
        //Chomp line ending
        buf[strcspn(buf,"\r\n")] = 0;
        // Skip line if less than 3 chars;
        if (strlen(buf) < 3) continue;
        // Skip line if second char is not '='
        if (buf[1] != '=') continue;
        // Grab drive num digit
        char drv = buf[0];
        // Attempt to mount drive
        switch (drv) {
        case '0':
            MountDisk(0,&buf[2]);
            break;
        case '1':
            MountDisk(1,&buf[2]);
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
        if (data == 0x43) {
            IF.sdclatch = true;
            IF.status = 0;
        } else {
            // init the interface
            memset(&IF,0,sizeof(IF));
        }
        break;
    // Flash Data
    case 0x42:
        IF.flash = data;
        break;
    // Flash Control
    case 0x43:
        BankSelect(data);
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
// Read port. If there are bytes in the reply buffer return them.
//----------------------------------------------------------------------
unsigned char SDCRead(unsigned char port)
{

    if ((!IF.sdclatch) && (port > 0x43)) return 0;

    unsigned char rpy;
    switch (port) {
    // Flash data
    case 0x42:
        rpy = IF.flash;
        break;
    // Flash control
    case 0x43:
        rpy = CurrentBank | (IF.flash & 0xF8);
        break;
    // Interface status
    case 0x48:
        if (IF.bufcnt > 0) {
            rpy = STA_BUSY|STA_READY;
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
            rpy = *IF.bufptr++;
            IF.bufcnt--;
        } else {
            rpy = IF.reply2;
        }
        break;
    // Reply data 3 or block reply
    case 0x4B:
        if (IF.bufcnt > 0) {
            rpy = *IF.bufptr++;
            IF.bufcnt--;
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

    // If transfer in progress abort whatever
    if ((IF.bufcnt > 0) || (IF.bufcnt > 0)) {
        _DLOG("SDCCommand transfer in progress ABORTING\n");
        memset(&IF,0,sizeof(IF));
        IF.status = STA_FAIL;
        return;
    }

    switch (IF.cmdcode & 0xF0) {
    // Read sector
    case 0x80:
        ReadSector();
        break;
    // Stream sectors
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
        // 'C' Return current directory in block
        _DLOG("GetCurDir %s\n",CurDir);
        LoadReply(CurDir,strlen(CurDir)+1,0);
        break;
    case 0x51:
        // 'Q' Return the size of disk image in p1,p2,p3
        GetSectorCount();
        break;
    case 0x3E:
        // '>' Get directory page
        LoadDirPage();
        break;
    case 0x2B:
        // '+' Mount next next disk in set.  Mounts disks with a
        // digit suffix, starting with '1'. May repeat
        _DLOG("GetDriveInfo $%0x (+) not enabled\n",IF.param1);
        IF.status = STA_FAIL;
        break;
    case 0x56:
        // 'V' Get BCD firmware version number in p2, p3.
        IF.reply2 = 0x00;
        IF.reply3 = 0x01;
        break;
    }
}

//----------------------------------------------------------------------
//  Update SD Commands.
//----------------------------------------------------------------------
void UpdateSD(void)
{
//    _DLOG("UpdateSD %02X %02X %02X %02X %02X '%s'\n",
//         IF.param1, IF.param2, IF.param3,
//         IF.blkbuf[0], IF.recvbuf[1], &IF.recvbuf[2]);

    switch (IF.blkbuf[0]) {
    // Mount dsk
    case 0x4D: //M
    case 0x6D: //m
        if (MountDisk(IF.cmdcode&1,&IF.blkbuf[2]))
            IF.status =  0;
        else
            IF.status = STA_FAIL;
        break;
    // Create Dsk
    case 0x4E: //N
    case 0x6E: //n
        //  $FF49 0 for DSK image, number of cylinders for SDF
        //  $FF4A 0 for DSK image, number of sides for SDF image
        //  $FF4B 0
        _DLOG("UpdateSD mount new image not Supported\n");
        IF.status = STA_FAIL;
        break;
    // Set directory
    case 0x44: //D
        if (SetCurDir(&IF.blkbuf[2]))
            IF.status =  0;
        else
            IF.status = STA_FAIL;
        break;
    // Initiate directory list
    case 0x4C: //L
        if (InitiateDir(&IF.blkbuf[2]))
            IF.status =  0;
        else
            IF.status = STA_FAIL;
        break;
    // Create directory
    case 0x4B: //K
        _DLOG("UpdateSD create new directory not Supported\n");
        IF.status = STA_FAIL;
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
// Load ROM from bank.
//----------------------------------------------------------------------
void BankSelect(int data)
{
    _DLOG("BankSelect %02x\n",data);
    CurrentBank = data & 7;
    //TODO: Load bank into rom;
}

//----------------------------------------------------------------------
// Read logical sector
//----------------------------------------------------------------------
void ReadSector(void)
{
    int drive = IF.cmdcode & 1;
    int lsn = (IF.param1 << 16) + (IF.param2 << 8) + IF.param3;
    int dsksiz = DiskImage[drive].size >> 8;

    if (dsksiz == 0) {
        _DLOG("ReadSector empty drive %d\n",drive);
        IF.status = STA_FAIL;
        return;
    } else if ( lsn > (DiskImage[drive].size >> 8)) {
        _DLOG("ReadSector overrun %d %d\n",lsn,DiskImage[drive].size>>8);
        IF.status = STA_FAIL;
        return;
    }

    if (DiskImage[drive].hFile == NULL) OpenDrive(drive);
    if (DiskImage[drive].hFile == NULL) {
        IF.status = STA_FAIL;
        return;
    }

    LARGE_INTEGER pos;
    pos.LowPart = lsn * 256;
    pos.HighPart = 0;

    if (!SetFilePointerEx(DiskImage[drive].hFile,pos,NULL,FILE_BEGIN)) {
        _DLOG("ReadSector seek error %s\n",LastErrorTxt());
        IF.status = STA_FAIL;
    }

    DWORD cnt = 0;
    DWORD num = 256;
    char buf[260];
    BOOL result = ReadFile(DiskImage[drive].hFile,buf,num,&cnt,NULL);

    //_DLOG("ReadSector %d drive %d hfile %d got %d bytes\n",
    //        lsn,drive,DiskImage[drive].hFile, cnt);

    if (!result) {
        _DLOG("ReadSector %d drive %d hfile %d error %s\n",
                lsn,drive,DiskImage[drive].hFile,LastErrorTxt());
        IF.status = STA_FAIL;

    } else if (cnt != num) {
        _DLOG("ReadSector %d drive %d hfile %d short read %s\n",
                lsn,drive,DiskImage[drive].hFile,LastErrorTxt());
        IF.status = STA_FAIL;
    } else {
        int mode = ((IF.cmdcode & 4) == 0) ? 0 : 1;
        LoadReply(buf,256,mode);
    }
}

//----------------------------------------------------------------------
// Stream image data
//----------------------------------------------------------------------
void StreamImage(void)
{
    //512 byte sectors
    int drive = IF.cmdcode & 1;
    int lsn = (IF.param1 << 16) + (IF.param2 << 8) + IF.param3;

    if (lsn < 0) {
        _DLOG("StreamImage invalid sector\n");
        IF.status = STA_FAIL;
        return;
    }

    _DLOG("StreamImage not supported\n");
    IF.status = STA_FAIL;  // Fail
}

//----------------------------------------------------------------------
// Write logical sector
//----------------------------------------------------------------------
void WriteSector(void)
{
    int drive = IF.cmdcode & 1;

    if (DiskImage[drive].hFile == NULL) OpenDrive(drive);
    if (DiskImage[drive].hFile == NULL) {
        IF.status = STA_FAIL|STA_NOTFOUND;
        return;
    }

    //_DLOG("WriteSector %d drive %d\n",lsn,drive);

    int lsn = (IF.param1 << 16) + (IF.param2 << 8) + IF.param3;
    LARGE_INTEGER pos;
    pos.QuadPart = lsn * 256;
    if (!SetFilePointerEx(DiskImage[drive].hFile,pos,NULL,FILE_BEGIN)) {
        _DLOG("WriteSector seek error %s\n",LastErrorTxt());
        IF.status = STA_FAIL|STA_NOTFOUND;
    }

    DWORD cnt;
    DWORD num = 256;
    WriteFile(DiskImage[drive].hFile, IF.blkbuf, num,&cnt,NULL);
    if (cnt != num) {
        _DLOG("WriteSector write error %s\n",LastErrorTxt());
        IF.status = STA_FAIL;
        IF.status = STA_FAIL|STA_NOTFOUND;
    } else {
        IF.status = 0;
    }
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

    // For now assuming a supported image type (not SDF)
    if ((DiskImage[drive].size & 0xFF) != 0) {
        _DLOG("GetSectorCount file size not mutiple of 256\n");
        IF.status = STA_FAIL;
    }

    unsigned int numsec = DiskImage[drive].size >> 8;
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
    if (strlen(DiskImage[drive].name) == 0) {
        //_DLOG("GetMountedImage drive %d empty\n",drive);
        IF.status = STA_FAIL;
    } else {
        struct FileRecord rec;
        LoadFileRecord(DiskImage[drive].name, &rec);
        //_DLOG("GetMountedImage drive %d\n",drive);
        LoadReply(&rec,sizeof(rec),0);
    }
}

//----------------------------------------------------------------------
// $DO Abort stream or mount disk in a set of disks.
// IF.param1  0: Next disk 1-9: specific disk.
// IF.param2 b0: Blink Enable
//----------------------------------------------------------------------
void SDCControl(void)
{
    // If a transfer is in progress abort it.
    if (IF.bufcnt > 0) {
        _DLOG("SDCControl Block Transfer Aborted\n");
        memset(&IF,0,sizeof(IF));
    } else {
        _DLOG("SDCControl Mount in set unsupported %d %d %d \n",
                  IF.cmdcode,IF.param1,IF.param2);
        IF.status = STA_FAIL | STA_NOTFOUND;
    }
}

//----------------------------------------------------------------------
// Load reply. Count is bytes, 512 max. Reply Mode is 0 for words,
// 1 for bytes.  If reply mode is words buffer bytes are swapped within
// words so they are read in the correct order.
//----------------------------------------------------------------------
void LoadReply(void *data, int count, int mode)
{
    if ((count < 2) | (count > 512)) {
        _DLOG("LoadReply bad count\n");
        IF.status = STA_FAIL;
        return;
    }

    char *dp = (char *) data;
    char *bp = IF.blkbuf;
    char tmp;

    if (mode == 1) {
        int ctr = count;
        while (ctr--) {
            *bp++ = *dp++;
        }
    } else {
        // Copy data to the reply buffer with bytes swapped in words
        int ctr = count/2;
        while (ctr--) {
            tmp = *dp++;
            *bp++ = *dp++;
            *bp++ = tmp;
        }
    }

    IF.bufptr = IF.blkbuf;
    IF.bufcnt = count;

    // If port reads exceed the count zeros will be returned
    IF.reply2 = 0;
    IF.reply3 = 0;

    //_DLOG("LoadReply %d\n",count);

    return;
}

//----------------------------------------------------------------------
// Convert standard name from path into 8.3 format
//----------------------------------------------------------------------
void CvtName83(char *fname8, char *ftype3, char *path) {

    int i;
    char c;
    char *p;

    // start with blanks
    memset(fname8,' ',8);
    memset(ftype3,' ',3);

    i = 0;
    p = strrchr(path,'/');
    if (p == NULL) p = path; else p++;

    // Special case for '.' or '..' names
    if (p[0] == '.') {
        fname8[0] = '.';
        if (p[1] == '.')
            fname8[1] = '.';
        return;
    }

    // Copy eight chars after the last '/' to name field
    while (c = *p++) {
        if (c == '.') break;
        fname8[i++] = c;
        if (i > 7) break;
    }

    // Copy three chars after the last '.' to type field
    i = 0;
    p = strrchr(path,'.');
    if (p == NULL) return; else p++;
    while (c = *p++) {
        ftype3[i++] = c;
        if (i > 2) break;
    }
    return;
}

//----------------------------------------------------------------------
// Convert 8.3 format path into standard path for windows lookup
// Warning: path buffer must be at least one byte larger than fpath8
// string.
//----------------------------------------------------------------------
void Cvt83Path(char *path, const char *fpath8, unsigned int size)
{
    // Path buffer must be at least one byte larger than size
    if (strlen(fpath8) > (size - 1)) {
        _DLOG("Cvt83Path size violation\n");
        return;
    }

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
    name[namlen] = '\0';       // terminate name
    strncat(path,name,size);   // append it to directory
}

//----------------------------------------------------------------------
// Load a file record for SDC file commands.
//----------------------------------------------------------------------
bool LoadFileRecord(char * file, struct FileRecord * rec)
{

    memset(rec,0,sizeof(rec));
    int filesize = FileAttrib(file,&rec->attrib);
    if (rec->attrib == ATTR_INVALID) {
        _DLOG("LoadFileRecord attibutes unavailable\n");
        return false;
    }
    CvtName83(rec->name,rec->type,file);
    rec->lolo_size = (filesize) & 0xFF;
    rec->hilo_size = (filesize >> 8) & 0xFF;
    rec->lohi_size = (filesize >> 16) & 0xFF;
    rec->hihi_size = (filesize >> 24) & 0xFF;

    //_DLOG("LoadFileRecord %s %d %02x\n",file,filesize,rec->attrib);
    return true;
}

//----------------------------------------------------------------------
// Get attributes for a file. Fills attributes byte and returns filesize
//----------------------------------------------------------------------
int FileAttrib(const char * path, char * attr)
{
    struct _stat file_stat;
    int result = _stat(path,&file_stat);
    int filesize;
    if (result != 0) {
        *attr = ATTR_INVALID;
        _DLOG("FileAttrib %s %s\n",path,LastErrorTxt());
        return 0;
    } else {
        *attr = 0;
        if ((file_stat.st_mode & _S_IFDIR) != 0) {
            *attr |= ATTR_DIR;
            filesize = 0;
        } else {
            filesize = file_stat.st_size;
        }
        if ((file_stat.st_mode & _S_IWRITE) == 0)
            *attr |= ATTR_RDONLY;
        return filesize;
    }
}

//----------------------------------------------------------------------
// Mount Drive. If image path starts with '/' load drive relative
// to SDRoot, else load drive relative to the current directory
// path file portion is SDC 8.3 format (8 chars name, 3 chars ext)
// Extension is delimited from name by blank,dot,or size
// examples:
//    "FOO     DSK" = FOO.DSK
//    "FOO.DSK"     = FOO.DSK
//    "ALONGFOODSK" = ALONGFOO.DSK
//----------------------------------------------------------------------
bool MountDisk (int drive, const char * path)
{

    char cvtpath[64];
    Cvt83Path(cvtpath, path,64);

    char fqn[MAX_PATH]={};

    drive &= 1;

    // Check for UNLOAD.  Path will be an empty string.
    if (*path == '\0') {
        CloseDrive(drive);
        memset((void *) &DiskImage[drive],0,sizeof(_DiskImage));
        return true;
    }

    // Establish image filename
    strncpy(fqn,SDCard,MAX_PATH);
    if (*path == '/') {
        strncat(fqn,cvtpath+1,MAX_PATH);
    } else {
        strncat(fqn,CurDir,MAX_PATH);
        AppendPathChar(fqn,'/');
        strncat(fqn,cvtpath,MAX_PATH);
    }

    //TODO: Wildcard in file name.
    // Append .DSK if no extension (assumes 8.3 naming)
    //TODO: Don't append .DSK if file opens without it
    if (fqn[strlen(fqn)-4] != '.') strncat(fqn,".DSK",MAX_PATH);

    // Get file attributes.
    char attrib;
    int filesize = FileAttrib(fqn, &attrib);
    if (attrib < 0) {
        _DLOG("MountDisk %s does not exist\n",fqn);
        return false;
    }
    if ((attrib & ATTR_DIR) != 0) {
        _DLOG("MountDisk %s is a directory\n",fqn);
        return false;
    }

    // Make sure drive is not open before updating image info
    CloseDrive(drive);

    // Fill image info.
    DiskImage[drive].hFile = NULL;
    strncpy(DiskImage[drive].name,fqn,MAX_PATH);
    DiskImage[drive].attrib = attrib;
    DiskImage[drive].size = filesize;
    //DiskImage[drive].sector = -1;

    _DLOG("MountDisk %d %s\n",drive,fqn);
    return true;
}

//----------------------------------------------------------------------
// Open virtual disk
//----------------------------------------------------------------------
void OpenDrive (int drive)
{
    drive &= 1;

    if (DiskImage[drive].hFile != NULL) {
        _DLOG("OpenDrive already open %d\n",drive);
        return;
    }

    if (*DiskImage[drive].name == '\0') {
        _DLOG("OpenDrive not mounted %d\n",drive);
        IF.status = STA_FAIL;
        return;
    }

    DiskImage[drive].hFile = CreateFile(
        DiskImage[drive].name, GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

    if (DiskImage[drive].hFile == NULL) {
        _DLOG("OpenDrive %s drive %d file %s\n",
            drive,DiskImage[drive],LastErrorTxt());
        IF.status = STA_FAIL;
    }

    _DLOG("OpenDrive %d %s %d\n",
        drive, DiskImage[drive].name, DiskImage[drive].hFile);
}

//----------------------------------------------------------------------
// Close virtual disk
//----------------------------------------------------------------------
void CloseDrive (int drive)
{
    drive &= 1;
    if (DiskImage[drive].hFile != NULL) {
        _DLOG("ClosingDrive %d\n",drive);
        CloseHandle(DiskImage[drive].hFile);
        DiskImage[drive].hFile = NULL;
    }
}

//----------------------------------------------------------------------
// Append char to path if not there
//----------------------------------------------------------------------
void AppendPathChar(char * path, char c)
{
    int l = strlen(path);
    if (l < MAX_PATH - 1 ) {
        if (path[l-1] != c) {
            path[l] = c;
            path[l+1] = '\0';
        }
    }
}

//----------------------------------------------------------------------
// Determine if path is a direcory
//----------------------------------------------------------------------
bool IsDirectory(const char * path)
{
    char attr;
    FileAttrib(path,&attr);
    if (attr == ATTR_INVALID) return false;
    if ((attr & ATTR_DIR) == 0) return false;
    return true;
}

//----------------------------------------------------------------------
// Set the Current Directory
//----------------------------------------------------------------------
bool SetCurDir(char * path)
{
    //TODO: Wildcards

    char fqp[MAX_PATH];
    char tmp[MAX_PATH] = {};

    // If blank is passed clear the current dirctory
    if (*path == '\0') {
        *CurDir = '\0';
        _DLOG("SetCurdir null\n");
        return true;
    }

    // If path is ".." go back a directory
    if (strcmp(path,"..") == 0) {
        _DLOG("SetCurdir back a directory\n");
        char *p = strrchr(CurDir,'/');
        if (p != NULL) {
            *p = '\0';
        } else {
            *CurDir = '\0';
        }
        _DLOG("SetCurdir back to root\n");
        IF.status = STA_READY;
        return true;
    }

    // Set new directory into a temp string
    strncpy(tmp,CurDir,MAX_PATH);
    AppendPathChar(tmp,'/');
    strncat(tmp,path,MAX_PATH);

    // Check if a valid directory on SD
    strncpy(fqp,SDCard,MAX_PATH);
    AppendPathChar(fqp,'/');
    strncat(fqp,tmp,MAX_PATH);
    if (!IsDirectory(fqp)) {
        _DLOG("SetCurDir invalid %s\n",fqp);
        IF.status = STA_FAIL;
        return false;
    }

    // Set new directory
    strncpy(CurDir,tmp,MAX_PATH);
    _DLOG("SetCurdir %s\n",CurDir);
    return true;
}

//----------------------------------------------------------------------
// Initialize file search. Append "*.*" if pattern is a directory.
// Save the directory portion of the pattern for prepending to results
//----------------------------------------------------------------------
bool InitiateDir(const char * pattern)
{
    // Prepend current directory
    char path[MAX_PATH];
    strncpy(path,SDCard,MAX_PATH);
    AppendPathChar(path,'/');
    strncat(path,CurDir,MAX_PATH);
    AppendPathChar(path,'/');
    strncat(path,pattern,MAX_PATH);

    // Add *.* to search pattern if last char is '/'
    int l = strlen(path);
    if (path[l-1] == '/') strncat(path,"*.*",MAX_PATH);

    hFind = FindFirstFile(path, &dFound);
    _DLOG("IntiateDir find %s\n",path);

    // Save directory portion of search path
    char *p = strrchr(path,'/'); p++; *p='\0';
    strcpy(SeaDir,path);

    return (hFind != INVALID_HANDLE_VALUE);
}

//----------------------------------------------------------------------
// Load directory page containing up to 16 file records:
//----------------------------------------------------------------------
bool LoadDirPage(void)
{
    struct FileRecord dpage[16];
    memset(dpage,0,sizeof(dpage));

    if ( hFind == INVALID_HANDLE_VALUE) {
        if (!IsDirectory(SDCard)) {
            _DLOG("LoadDirPage SDCard path invalid\n");
            IF.status = STA_FAIL;
            return false;
        }
         _DLOG("LoadDirPage Search failed\n");
        LoadReply(dpage,16,0);
        return false;
    }

    char fqn[MAX_PATH];
    int cnt = 0;
    while (cnt < 16) {
        strncpy(fqn,SeaDir,MAX_PATH);
        strncat(fqn,dFound.cFileName,MAX_PATH);
        LoadFileRecord(fqn,&dpage[cnt]);
        cnt++;
        if (FindNextFile(hFind,&dFound) == 0) {
            FindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
            break;
        }
    }

    //_DLOG("LoadDirPage LoadReply\n");
    LoadReply(dpage,256,0);
    return true;
}
