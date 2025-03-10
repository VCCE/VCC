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
//  selected cartridge slot. Sometime the past the VCC cart select
//  was disabled in mmi.cpp. This had to be be re-enabled for by
//  modifying mmi.dll.  This means sdc.dll requires the new version
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
// Function templates
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

void SetMode(int);
void ParseStartup();
void SDCCommand(int);
void CalcSectorNum(int);
void ReadSector(int);
void StreamImage(int);
void WriteSector(int);
void UnusedCommand(int);
void GetDriveInfo(int);
void SDCControl(int);
void UpdateSD(int);
void AppendPathChar(char *,char c);
int FileAttrib(const char *,char *);
bool LoadFileRecord(char *,struct FileRecord *);
bool MountDisk (int,const char *);
void CloseDrive(int);
void OpenDrive(int);
void LoadReply(void *, int);
unsigned char PutByte();
void GetByte(unsigned char);
char * LastErrorTxt();
void BlockRecvComplete();
void BlockRecvStart(int);
void BankSelect(int);
bool LoadDirPage();
bool SetCurDir(char * path);
bool InitiateDir(const char *);
bool IsDirectory(const char *path);
void GetMountedImageRec(int);
void CvtName83(char *,char *,char *);
void GetSectorCount(int);

//======================================================================
// Globals
//======================================================================

// SDC Latch flag
bool SDC_Mode=false;

// Current command code and sector for Disk I/O
unsigned char BlockRecvCode = 0;
//int CurSectorNum = -1;

// Command status and parameters
unsigned char CmdSta = 0;
unsigned char CmdRpy1 = 0;
unsigned char CmdRpy2 = 0;
unsigned char CmdRpy3 = 0;
unsigned char CmdPrm1 = 0;
unsigned char CmdPrm2 = 0;
unsigned char CmdPrm3 = 0;
unsigned char FlshDat = 0;

// Cartridge ROM
unsigned char PakRom[0x4000];

// Rom bank currently selected
unsigned char CurrentBank = 0;

// Host paths for SDC
char SDCard[MAX_PATH] = {}; // SD card root directory
char CurDir[MAX_PATH] = {}; // SDC current directory
char SeaDir[MAX_PATH] = {}; // Last directory searched

// Command send buffer. Block data from Coco lands here.
// Buffer is sized to hold at least 256 bytes. (for sector writes)
char RecvBuf[300];
int RecvCount = 0;
char *RecvPtr = RecvBuf;

// Command reply buffer. Block data to Coco goes here.
// Buffer is sized to hold at least 512 bytes (for streaming)
unsigned char ReplyBuf[600];
int ReplyCount = 0;
unsigned char *ReplyPtr = ReplyBuf;

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
    int sector;
} DiskImage[2];

// Flash banks
char FlashFile[8][MAX_PATH];

// Dll handle
static HINSTANCE hinstDLL;

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
        return ;
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
        return(SDCRead(Port));
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
    for (int i=0;i<8;i++) {
        char txt[32];
        sprintf(txt,"FlashFile_%d",i);
        WritePrivateProfileString("SDC",txt,FlashFile[i],IniFile);
    }

    return;
}

//----------------------------------------------------------------------
// Init the controller. This gets called by ModuleReset
//----------------------------------------------------------------------
void SDCInit()
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
    SetMode(0);
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

    unsigned char *p = PakRom;
    while (ctr++ < 0x4000) {
        if ((ch = fgetc(h)) < 0) break;
        *p++ = (unsigned char) ch;
    }

    fclose(h);
    return true;
}

//----------------------------------------------------------------------
// Write SDC port.  If a command needs a data block to complete it
// will put a count (256 or 512) in RecvCount.  WRITE PORT
//----------------------------------------------------------------------
void SDCWrite(unsigned char data,unsigned char port)
{

    if (!SDC_Mode && port > 0x43) {
        //_DLOG("SDCWrite %02X %02X wrong mode\n",port,data);
        CmdSta = STA_FAIL;
        return;
    }

    switch (port) {
    case 0x40:
        SetMode(data);
        break;
    case 0x42:
        FlshDat = data;
        break;
    case 0x43:
        BankSelect(data);
        break;
    case 0x48:
        SDCCommand(data);
        break;
    case 0x49:
        CmdPrm1 = data;
        break;
    case 0x4A:
        if (RecvCount > 0) GetByte(data); else CmdPrm2 = data;
        break;
    case 0x4B:
        if (RecvCount > 0) GetByte(data); else CmdPrm3 = data;
        break;
    default:
        _DLOG("SDCWrite %02X %02X?\n",port,data);
        break;
    }
    return;
}

//----------------------------------------------------------------------
// Read SDC port. If there are bytes in the reply buffer return them.
//----------------------------------------------------------------------
unsigned char SDCRead(unsigned char port)
{

    if (!SDC_Mode && port != 0x43) {
        _DLOG("SDCRead port %02X wrong mode\n",port);
        CmdSta = STA_FAIL;
        return 0;
    }

    unsigned char rpy;
    switch (port) {
    case 0x42:
        rpy = FlshDat;
        _DLOG("SDCRead flash data %02x\n",rpy);
        break;
    case 0x43:
        rpy = CurrentBank | (FlshDat & 0xF8);
        break;
    case 0x48:
        if ((ReplyCount > 0) || (RecvCount > 0)) {
            rpy = STA_BUSY|STA_READY;
        } else {
            rpy = CmdSta;
        }
        break;
    case 0x49:
        rpy = CmdRpy1;
        break;
    case 0x4A:
        rpy = (ReplyCount > 0) ? PutByte() : CmdRpy2;
        break;
    case 0x4B:
        rpy = (ReplyCount > 0) ? PutByte() : CmdRpy3;
        break;
    default:
        rpy = 0;
        break;
    }
    return rpy;
}

//----------------------------------------------------------------------
// Parse the startup.cfg file
//----------------------------------------------------------------------
void ParseStartup()
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
// Buffer data received
//----------------------------------------------------------------------
void GetByte(unsigned char byte)
{
    if (RecvCount > 0) {
        RecvCount--;
        *RecvPtr++ = byte;
    }
    // Done receiving block
    if (RecvCount < 1) {
        BlockRecvComplete();
    }
}

//----------------------------------------------------------------------
// Send reply from buffer
//----------------------------------------------------------------------
unsigned char PutByte()
{
    if (ReplyCount > 0) {
        ReplyCount--;
        return *ReplyPtr++;
    } else {
        return 0;
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
// Set SDC controller mode
//----------------------------------------------------------------------
void SetMode(int data)
{
    if (data == 0x43) {
        SDC_Mode = true;
    } else if (data == 0) {
        SDC_Mode = false;
        CmdRpy1 = 0;
        CmdRpy2 = 0;
        CmdRpy3 = 0;
        CmdPrm1 = 0;
        CmdPrm2 = 0;
        CmdPrm3 = 0;
        RecvCount = 0;
        ReplyCount = 0;
    }
    CmdSta  = 0;
    return;
}

//----------------------------------------------------------------------
//  Dispatch SDC commands
//----------------------------------------------------------------------
void SDCCommand(int code)
{

    if (!SDC_Mode) {
        _DLOG("SDCCommand %02X %02X %02x %02x wrong mode\n",
            code,CmdPrm1,CmdPrm2,CmdPrm3);
        return;
    }

    // If busy or tranfer in progress abort whatever
    if ((RecvCount > 0) || (ReplyCount > 0)) {
        _DLOG("SDCCommand ABORTING\n");
        SetMode(0);
        CmdSta = STA_FAIL;
        return;
    }

    // Invalidate command code for block recieves
    BlockRecvCode = 0;

    // The SDC uses the low nibble of the command code as
    // added argument so this gets passed to the command.
    int loNib = code & 0xF;
    int hiNib = code & 0xF0;

    switch (hiNib) {
    case 0x80:
        CalcSectorNum(code);
        ReadSector(loNib);
        break;
    case 0x90:
        CalcSectorNum(code);
        StreamImage(loNib);
        break;
    case 0xA0:
        CalcSectorNum(code);
        BlockRecvStart(code);
        break;
    case 0xC0:
        GetDriveInfo(loNib);
        break;
    case 0xD0:
        SDCControl(loNib);
        break;
    case 0xE0:
        BlockRecvStart(code);
        break;
    default:
        _DLOG("SDCCommand %02X not supported\n",code);
        CmdSta = STA_FAIL;  // Fail
        break;
    }
    return;
}

//----------------------------------------------------------------------
// Calculate sector.  numsides == 2 if double sided
//----------------------------------------------------------------------
void CalcSectorNum(int code)
{
    int drive = code & 1;
    //if (!(code & 2)) {_DLOG("CalcSectorNum Not double sided\n");}
    DiskImage[drive].sector = (CmdPrm1 << 16) + (CmdPrm2 << 8) + CmdPrm3;
}

//----------------------------------------------------------------------
// Command Ax or Ex block transfer start
//----------------------------------------------------------------------
void BlockRecvStart(int code)
{
    if (!SDC_Mode) {
        _DLOG("BlockRecvStart not SDC mode %02X\n",code);
        BlockRecvCode = 0;
        CmdSta = STA_FAIL;
        return;
    } else if ((code & 4) !=0 ) {
        _DLOG("BlockRecvStart 8bit block not supported %02X\n",code);
        BlockRecvCode = 0;
        CmdSta = STA_FAIL;
        return;
    }
    BlockRecvCode = code;

    // Set up the receive buffer
    CmdSta = STA_READY | STA_BUSY;
    RecvPtr = RecvBuf;
    RecvCount = 256;
}

//----------------------------------------------------------------------
// Command Ax or Ex block transfer complete
//----------------------------------------------------------------------
void BlockRecvComplete()
{
    int loNib = BlockRecvCode & 0xF;
    int hiNib = BlockRecvCode & 0xF0;
    switch (hiNib) {
    case 0xA0:
        WriteSector(loNib);
        break;
    case 0xE0:
        UpdateSD(loNib);
        break;
    default:
        _DLOG("BlockRecvComplete invalid cmd %d\n",BlockRecvCode);
        CmdSta = STA_FAIL;
        break;
    }
    return;
}

//----------------------------------------------------------------------
// Get most recent windows error text
//----------------------------------------------------------------------
char * LastErrorTxt() {
    static char msg[200];
    DWORD error_code = GetLastError();
    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error_code,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   msg, 200, nullptr);
    return msg;
}

//----------------------------------------------------------------------
// Read logical sector
//----------------------------------------------------------------------
void ReadSector(int loNib)
{
    int drive = loNib & 1;
    int lsn = DiskImage[drive].sector;

    if (!SDC_Mode) {
        _DLOG("ReadSector wrong mode ignored\n");
        CmdSta = STA_FAIL;
        return;
    } else if ((loNib & 4) !=0 ) {
        _DLOG("ReadSector 8bit transfer not supported\n");
        CmdSta = STA_FAIL;
        return;
    } else if (lsn < 0) {
        _DLOG("ReadSector invalid sector number\n");
        CmdSta = STA_FAIL;
        return;
    }
    //_DLOG("ReadSector %d\n",lsn);

    if (DiskImage[drive].hFile == NULL) OpenDrive(drive);
    if (DiskImage[drive].hFile == NULL) {
        CmdSta = STA_FAIL;
        return;
    }

    LARGE_INTEGER pos;
    pos.QuadPart = lsn * 256;
    if (!SetFilePointerEx(DiskImage[drive].hFile,pos,NULL,FILE_BEGIN)) {
        _DLOG("ReadSector seek error %s\n",LastErrorTxt());
        CmdSta = STA_FAIL;
    }

    DWORD cnt;
    DWORD num = 256;
    char buf[256];
    ReadFile(DiskImage[drive].hFile,buf,num,&cnt,NULL);
    if (cnt != num) {
        _DLOG("ReadSector %d drive %d hfile %d error %s\n",
                lsn,drive,DiskImage[drive].hFile,LastErrorTxt());
        CmdSta = STA_FAIL;
    } else {
        //_DLOG("ReadSector %d drive %d hfile %d\n",lsn,drive,DiskImage[drive].hFile);
        LoadReply(buf,256);
    }
}

//----------------------------------------------------------------------
// Stream image data
//----------------------------------------------------------------------
void StreamImage(int loNib)
{
    int drive = loNib & 1;
    int lsn = DiskImage[drive].sector;

    if (lsn < 0) {
        _DLOG("StreamImage invalid sector\n");
        CmdSta = STA_FAIL;
        return;
    }

    _DLOG("StreamImage not supported\n");
    CmdSta = STA_FAIL;  // Fail
}

//----------------------------------------------------------------------
// Write logical sector
//----------------------------------------------------------------------
void WriteSector(int loNib)
{
    int drive = loNib & 1;
    int lsn = DiskImage[drive].sector;

    if (lsn < 0) {
        _DLOG("WriteSector invalid sector number\n");
        CmdSta = STA_FAIL;
        return;
    }

    if (DiskImage[drive].hFile == NULL) OpenDrive(drive);
    if (DiskImage[drive].hFile == NULL) {
        CmdSta = STA_FAIL|STA_NOTFOUND;
        return;
    }

    _DLOG("WriteSector %d drive %d\n",lsn,drive);

    LARGE_INTEGER pos;
    pos.QuadPart = lsn * 256;
    if (!SetFilePointerEx(DiskImage[drive].hFile,pos,NULL,FILE_BEGIN)) {
        _DLOG("WriteSector seek error %s\n",LastErrorTxt());
        CmdSta = STA_FAIL|STA_NOTFOUND;
    }

    DWORD cnt;
    DWORD num = 256;
    WriteFile(DiskImage[drive].hFile, RecvBuf, num,&cnt,NULL);
    if (cnt != num) {
        _DLOG("WriteSector write error %s\n",LastErrorTxt());
        CmdSta = STA_FAIL;
        CmdSta = STA_FAIL|STA_NOTFOUND;
    } else {
        CmdSta = 0;
    }
}

//----------------------------------------------------------------------
// Get drive information
//----------------------------------------------------------------------
void GetDriveInfo(int loNib)
{
     if (!SDC_Mode) {
        _DLOG("GetDriveInfo not SDC mode\n");
        return;
     }

    int drive = loNib & 1;
    switch (CmdPrm1) {
    case 0x49:
        // 'I' - return drive information in block
        GetMountedImageRec(drive);
        break;
    case 0x43:
        // 'C' Return current directory in block
        //_DLOG("GetDriveInfo $%0x (C) not supported\n",CmdPrm1);
        //CmdSta = STA_FAIL;
        _DLOG("GetCurDir %s\n",CurDir);
        LoadReply(CurDir,strlen(CurDir)+1);
        break;
    case 0x51:
        // 'Q' Return the size of disk image in p1,p2,p3
        GetSectorCount(drive);
        break;
    case 0x3E:
        // '>' Get directory page
        LoadDirPage();
        break;
    case 0x2B:
        // '+' Mount next next disk in set.  Mounts disks with a
        // digit suffix, starting with '1'. May repeat
        _DLOG("GetDriveInfo $%0x (+) not supported\n",CmdPrm1);
        CmdSta = STA_FAIL;
        break;
    case 0x56:
        // 'V' Get BCD firmware version number in p2, p3.
        CmdRpy2 = 0x01;
        CmdRpy3 = 0x13;
        break;
    default:
        _DLOG("GetDriveInfo %d $%0x (%c) not supported\n",
                loNib, CmdPrm1,CmdPrm1);
        CmdSta = STA_FAIL;
        break;
    }
}

//----------------------------------------------------------------------
// Return sector count for mounted disk image
//----------------------------------------------------------------------
void  GetSectorCount(int drive) {

    // For now assuming supported image type (not SDF)
    unsigned int numsec = DiskImage[drive].size >> 8;
    _DLOG("GetSectorCount %d %d\n",drive,numsec);
    CmdRpy3 = numsec & 0xFF;
    numsec = numsec >> 8;
    CmdRpy2 = numsec & 0xFF;
    numsec = numsec >> 8;
    CmdRpy1 = numsec & 0xFF;
}

//----------------------------------------------------------------------
// Return file record for mounted disk image
//----------------------------------------------------------------------
void GetMountedImageRec(int drive)
{
    if (strlen(DiskImage[drive].name) == 0) {
        //_DLOG("GetMountedImage drive %d empty\n",drive);
        CmdSta = STA_FAIL;
    } else {
        struct FileRecord rec;
        LoadFileRecord(DiskImage[drive].name, &rec);
        //_DLOG("GetMountedImage drive %d\n",drive);
        LoadReply(&rec,sizeof(rec));
    }
}

//----------------------------------------------------------------------
// $DO Abort stream or mount disk in a set of disks.
// CmdPrm1  0: Next disk 1-9: specific disk.
// CmdPrm2 b0: Blink Enable
//----------------------------------------------------------------------
void SDCControl(int loNib)
{
    // If a transfer is in progress abort it.
    if ((ReplyCount > 0) || (RecvCount > 0)) {
        _DLOG("SDCControl Block Transfer Aborted\n");
        ReplyCount = 0;
        RecvCount = 0;
        CmdSta = 0;
    } else {
        _DLOG("SDCControl Mount in set unsupported %d %d %d \n",
                  loNib,CmdPrm1,CmdPrm2);
        CmdSta = STA_FAIL | STA_NOTFOUND;
    }
}

//----------------------------------------------------------------------
// Load reply.  Buffer bytes are swapped within words so they
// are read in the correct order.  Count is bytes, 512 max
//----------------------------------------------------------------------
void LoadReply(void *data, int count)
{

    if ((count < 2) | (count > 512)) {
        _DLOG("LoadReply bad count\n");
        CmdSta = STA_FAIL;
        return;
    }

    // Copy data to the reply buffer with bytes swapped in words
    unsigned char *dp = (unsigned char *) data;
    unsigned char *bp = ReplyBuf;
    unsigned char tmp;
    int ctr = count/2;
    while (ctr--) {
        tmp = *dp++;
        *bp++ = *dp++;
        *bp++ = tmp;
    }

    //CmdSta = STA_READY;
    ReplyPtr = ReplyBuf;
    ReplyCount = count;

    // If port reads exceed the count zeros will be returned
    CmdPrm2 = 0;
    CmdPrm3 = 0;

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

    //_DLOG("Cvt83Path '%s' converted to '%s'\n",fpath8,path);
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
    DiskImage[drive].sector = -1;

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
        CmdSta = STA_FAIL;
        return;
    }

    DiskImage[drive].hFile = CreateFile(
        DiskImage[drive].name, GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

    if (DiskImage[drive].hFile == NULL) {
        _DLOG("OpenDrive %s drive %d file %s\n",
            drive,DiskImage[drive],LastErrorTxt());
        CmdSta = STA_FAIL;
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
//  Update SD Commands.
//----------------------------------------------------------------------
void UpdateSD(int loNib)
{

    _DLOG("UpdateSD %02X %02X %02X %02X %02X '%s'\n",
         CmdPrm1,CmdPrm2,CmdPrm3,RecvBuf[0],RecvBuf[1],&RecvBuf[2]);

    switch (RecvBuf[0]) {
    case 0x4D: //M
    case 0x6D: //m
        CmdSta = (MountDisk(loNib,&RecvBuf[2])) ? 0 : STA_FAIL;
        break;
    case 0x4E: //N
    case 0x6E: //n
        //  $FF49 0 for DSK image, number of cylinders for SDF
        //  $FF4A hiNib 0 for DSK image, number of sides for SDF image
        //  $FF4B 0
        _DLOG("UpdateSD mount new image not Supported\n");
        CmdSta = STA_FAIL;
        break;
    case 0x44: //D
        CmdSta = (SetCurDir(&RecvBuf[2])) ? 0 : STA_FAIL;
        break;
    case 0x4C: //L
        CmdSta = (InitiateDir(&RecvBuf[2])) ? STA_READY : STA_FAIL;
        break;
    case 0x4B: //K
        _DLOG("UpdateSD create new directory not Supported\n");
        CmdSta = STA_FAIL;
        break;
    case 0x5B: //X
        _DLOG("UpdateSD delete file or directory not Supported\n");
        CmdSta = STA_FAIL;
        break;
    default:
        _DLOG("UpdateSD %02x not Supported\n",RecvBuf[0]);
        CmdSta = STA_FAIL;
        break;
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
        _DLOG("SetCurdir null");
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
        _DLOG("SetCurdir null\n");
        CmdSta = STA_READY;
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
        CmdSta = STA_FAIL;
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
bool LoadDirPage()
{
    struct FileRecord dpage[16];
    memset(dpage,0,sizeof(dpage));

    if ( hFind == INVALID_HANDLE_VALUE) {
        if (!IsDirectory(SDCard)) {
            _DLOG("LoadDirPage SDCard path invalid\n");
            CmdSta = STA_FAIL;
            return false;
        }
         _DLOG("LoadDirPage Search failed\n");
        LoadReply(dpage,16);
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
    LoadReply(dpage,256);
    return true;
}
