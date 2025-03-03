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

// For PathCompactPathEx
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

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
#define STA_BUSY     0X01
#define STA_READY    0x02
#define STA_HWERROR  0x04
#define STA_CRCERROR 0x08
#define STA_NOTFOUND 0X10
#define STA_DELETED  0X20
#define STA_WPROTECT 0X40
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
void SDCReset(void);
void SetSDRoot(const char *);

void AssertInterupt(unsigned char,unsigned char);
void MemWrite(unsigned char,unsigned short);
unsigned char MemRead(unsigned short);

static char IniFile[MAX_PATH]={0};  // Ini file name from config
static HINSTANCE hinstDLL;          // DLL handle
static HWND hConfDlg = NULL;        // Config dialog

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
void SDC_Load_Card(void);
unsigned char PakRom[0x4000];

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
void AppendPathChar(char *, char c);
char FileAttrByte(const char *);
bool MountDisk (int,const char *);
void CloseDrive(int);
void OpenDrive(int);
void LoadReply(void *, int);
unsigned char PutByte();
void ReceiveByte(unsigned char);
char * LastErrorTxt();
void BlockRecvComplete();
void BlockRecvStart(int);
void BankSelect(int);
bool LoadDirPage();
bool SetCurDir(char * path);
bool InitiateDir(const char *);
bool FileExists(const char * path);
bool IsDirectory(const char *path);
void GetMountedImageRec(int);

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

// Last block read from disk
char FileReadBuf[512];

// Rom bank currently selected
unsigned char CurrentBank = 0;

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


// Host paths for SDC
char SDCard[MAX_PATH] = {}; // SD card root directory
char CurDir[MAX_PATH] = {}; // SDC current directory
char SeaDir[MAX_PATH] = {}; // Last directory searched

// File record for sending to ROM utilitied
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
struct {
    char name[MAX_PATH];
    HANDLE hFile;
    int sector;
} DiskImage[2];

//DiskImmage[drive].sector

//char DiskImageName[2][MAX_PATH] = {"",""};
// Drive file info
struct FileRecord DriveFileRec[2] = {};

// Windows file lookup handle and data
HANDLE hFind;
WIN32_FIND_DATAA dFound;

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
    {        //SDCReset();
        return 0;
    }

    //  Dll export run config dialog
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

    // Set ini file path and load HD config settings
    __declspec(dllexport) void SetIniPath (char *IniFilePath)
    {
        strcpy(IniFile,IniFilePath);
        LoadConfig();
        SDCInit();
        return;
    }

    // Return a byte from the current PAK ROM
	__declspec(dllexport) unsigned char PakMemRead8(unsigned short adr)
	{
		return(PakRom[adr & 0x3ffff]);
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
        hinstDLL = NULL;
        SDCReset();
    }
    return TRUE;
}

//-------------------------------------------------------------
// Generate menu for configuring the SDC
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

//----------------------------------------------------------------------
// Set the CD root
//----------------------------------------------------------------------
void SetSDRoot(const char * path)
{
    // Make sure path is a directory
    if (!IsDirectory(path)) {
        _DLOG("SetSDRoot invalid path %s\n",path);
        return;
    }

    strncpy(SDCard,path,MAX_PATH);
    AppendPathChar(SDCard,'/');

    // Clear the current directory
    SetCurDir("");
    return;
}

//----------------------------------------------------------------------
// Init the controller.
//----------------------------------------------------------------------
void SDCInit()
{

#ifdef _DEBUG_
    _DLOG("SDCInit\n");
    MoveWindow(GetConsoleWindow(),0,0,300,800,TRUE);
#endif

    memset((void *) &DriveFileRec,0,sizeof(DriveFileRec));
//  memset((void *) &DiskImageName,0,sizeof(DiskImageName));
    memset((void *) &DiskImage,0,sizeof(DiskImage));

    ParseStartup();
    SetCurDir("");
    CurrentBank = 0;
    SetMode(0);
    //_DLOG("SDCInit loading SDC-DOS Rom\n");
    LoadRom("SDC-DOS.ROM");
    return;
}

//----------------------------------------------------------------------
// Reset the controller
//----------------------------------------------------------------------
void SDCReset()
{
    _DLOG("SDCReset Unloading Drives\n");
    MountDisk (0,"");
    MountDisk (1,"");
    SetMode(0);
    return;
}

//----------------------------------------------------------------------
// Write SDC port.  If a command needs a data block to complete it
// will put a count (256 or 512) in RecvCount.
//----------------------------------------------------------------------
void SDCWrite(unsigned char data,unsigned char port)
{

    if ((!SDC_Mode) && (port > 0x4A || port == 0x4B || port == 0x49)) {
        //_DLOG("SDCWrite %02X %02X wrong mode\n",port,data);
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
        if (RecvCount > 0) ReceiveByte(data); else CmdPrm2 = data;
        break;
    case 0x4B:
        if (RecvCount > 0) ReceiveByte(data); else CmdPrm3 = data;
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
        rpy = CmdSta;
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

    //TODO D=[Current directory]
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
// Buffer data received.
//----------------------------------------------------------------------
void ReceiveByte(unsigned char byte)
{
    RecvCount--;
    if (RecvCount < 1) {
        BlockRecvComplete();
        CmdSta = 0;
    } else {
        CmdSta |= STA_BUSY;
    }
    *RecvPtr++ = byte;
}

//----------------------------------------------------------------------
// Unload data for reply.
//----------------------------------------------------------------------
unsigned char PutByte()
{
    ReplyCount--;
    if (ReplyCount < 1) {
        CmdSta = 0;
    } else {
        CmdSta |= STA_BUSY;
    }
    //_DLOG("PutByte %03d %02X %X\n",ReplyCount,*ReplyPtr,ReplyPtr);
    return *ReplyPtr++;
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
    bool tmp;

    if (data == 0x43)   tmp = true;
    else if (data == 0) tmp = false;
    else return;

    if (SDC_Mode != tmp) {
        SDC_Mode = tmp;
        CmdSta  = 0;
        CmdRpy1 = 0;
        CmdRpy2 = 0;
        CmdRpy3 = 0;
        CmdPrm1 = 0;
        CmdPrm2 = 0;
        CmdPrm3 = 0;
        RecvCount = 0;
        ReplyCount = 0;
        BlockRecvCode = 0;
    }
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
    if ((CmdSta & STA_BUSY) || (RecvCount > 0) || (ReplyCount > 0)) {
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
    if (!(code & 2)) {_DLOG("CalcSectorNum Not double sided\n");}
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
    ReadFile(DiskImage[drive].hFile,FileReadBuf,num,&cnt,NULL);
    if (cnt != num) {
        _DLOG("ReadSector %d drive %d hfile %d error %s\n",
                lsn,drive,DiskImage[drive].hFile,LastErrorTxt());
        CmdSta = STA_FAIL;
    } else {
        //_DLOG("ReadSector %d drive %d hfile %d\n",lsn,drive,DiskImage[drive].hFile);
        LoadReply(FileReadBuf,256);
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
        _DLOG("GetDriveInfo $%0X (C) not supported\n",CmdPrm1);
        CmdSta = STA_FAIL;
        break;
    case 0x51:
        // 'Q' Return the size of disk image in p1,p2,p3
        _DLOG("GetDriveInfo $%0X (Q) not supported\n",CmdPrm1);
        CmdSta = STA_FAIL;
        break;
    case 0x3E:
        LoadDirPage();
        break;
    case 0x2B:
        // '+' Mount next next disk in set.  Mounts disks with a
        // digit suffix, starting with '1'. May repeat
        _DLOG("GetDriveInfo $%0X (+) not supported\n",CmdPrm1);
        CmdSta = STA_FAIL;
        break;
    case 0x56:
        // 'V' Get BCD firmware version number in p2, p3.
        CmdRpy2 = 0x01;
        CmdRpy3 = 0x13;
        break;
    default:
        _DLOG("GetDriveInfo $%0X (%c) not supported\n",CmdPrm1,CmdPrm1);
        CmdSta = STA_FAIL;
        break;
    }
}

//----------------------------------------------------------------------
// Return file record for mounted disk image
//----------------------------------------------------------------------
void GetMountedImageRec(int drive)
{
    //if (strlen(DiskImageName[drive]) == 0) {
    if (strlen(DiskImage[drive].name) == 0) {
        _DLOG("GetMountedImage drive %d empty\n",drive);
        CmdSta = STA_FAIL;
    } else {
        LoadReply((void *) &DriveFileRec[drive],sizeof(FileRecord));
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

    CmdSta = STA_READY;
    ReplyPtr = ReplyBuf;
    ReplyCount = count;

    // If port reads exceed the count zeros will be returned
    CmdPrm2 = 0;
    CmdPrm3 = 0;

    return;
}
//----------------------------------------------------------------------
// Return file attribute bits
//----------------------------------------------------------------------
char FileAttrByte(const char * file)
{
    DWORD Attr = GetFileAttributes(file);
    if (Attr == INVALID_FILE_ATTRIBUTES) return ATTR_INVALID;

    char retval = 0;
    if (Attr & FILE_ATTRIBUTE_READONLY)  retval |= ATTR_RDONLY;
    if (Attr & FILE_ATTRIBUTE_DIRECTORY) retval |= ATTR_DIR;
    if (Attr & FILE_ATTRIBUTE_HIDDEN)    retval |= ATTR_HIDDEN;
    return retval;
}

//----------------------------------------------------------------------
// Load file record
//----------------------------------------------------------------------
bool LoadFileRecord(char * file, FileRecord * rec)
{
    int i;
    char c;
    memset(rec,0,sizeof(rec));

    rec->attrib = FileAttrByte(file);
    if (rec->attrib == ATTR_INVALID) {
        _DLOG("LoadFileRecord attibutes unavailable\n");
        return false;
    }

    char * p = strrchr(file,'/');
    if (p == NULL) {
        _DLOG("LoadFileRecord no backslash\n");
        return false;
    }
    p++;

    memset(rec->name,' ',8);
    memset(rec->type,' ',3);

    // If a directory all we need is it's name
    if (rec->attrib & ATTR_DIR) {
        for (i=0;i<8;i++) {
            char c = *p++;
            if (c == '\0') break;
            rec->name[i] = c;
        }
        //_DLOG("LoadFileRecord %s\n", strrchr(file,'/'));
        return true;
    }

    i = 0;
    while (c = *p++) {
        if (c == '.') break;
        rec->name[i++] = c;
        if (i > 7) break;
    }

    // Advance to file type
    while ((c != '\0') && (c != '.')) c = *p++;

    i = 0;
    while (c = *p++) {
        rec->type[i++] = c;
        if (i > 2) break;
    }

    struct _stat fs;
    int r=_stat(file,&fs);
    if (r != 0) {
        _DLOG("LoadFileRecord stat returned $d file %s\n",r,file);
        return false;
    }

    //_DLOG("LoadFileRecord %s %d\n", strrchr(file,'/'), fs.st_size);

    rec->lolo_size = (fs.st_size) & 0xff;
    rec->hilo_size = (fs.st_size >> 8) & 0xff;
    rec->lohi_size = (fs.st_size >> 16) & 0xff;
    rec->hihi_size = (fs.st_size >> 24) & 0xff;

    return true;
}

//----------------------------------------------------------------------
// Mount Drive.  If image path starts with '/' load drive relative
// to SDRoot, else load drive relative to the current directory.
//----------------------------------------------------------------------
bool MountDisk (int drive, const char * path)
{
    char fqn[MAX_PATH]={};

    drive &= 1;

    // Check for UNLOAD.  Path will be an empty string.
    if (*path == '\0') {
        memset((void *) &DriveFileRec[drive], 0, sizeof(FileRecord));
        //*DiskImageName[drive] = '\0';
        *DiskImage[drive].name = '\0';
        CloseDrive(drive);
        return true;
    }

    strncpy(fqn,SDCard,MAX_PATH);
    if (*path == '/') {
        strncat(fqn,path+1,MAX_PATH);
    } else {
        strncat(fqn,CurDir,MAX_PATH);
        AppendPathChar(fqn,'/');
        strncat(fqn,path,MAX_PATH);
    }

    //TODO: Don't append .DSK if file opens without it
    //TODO: Wildcard in file name.
    // Append .DSK if no extension (assumes 8.3 naming)
    if (fqn[strlen(fqn)-4] != '.') strncat(fqn,".DSK",MAX_PATH);

    // Check if file exists
    if (!FileExists(fqn)) {
        if (!FileExists(fqn)) {
            _DLOG("MountDisk %s does not exist\n",fqn);
            return false;
        }
    }

    // Set the image name and load the file record;
    //strncpy(DiskImageName[drive],fqn,MAX_PATH);
    strncpy(DiskImage[drive].name,fqn,MAX_PATH);
    LoadFileRecord(fqn,&DriveFileRec[drive]);

    // Establish image type (only headerless JVC is supported)
    if (DriveFileRec[drive].lolo_size != 0) {
        _DLOG("MountDisk %d invalid type %s\n",drive,fqn);
        return false;
    }

    CloseDrive(drive);
    _DLOG("MountDisk %d %s\n",drive,fqn);

    return true;
}

//----------------------------------------------------------------------
// Check windows file exists and is not a directory
//----------------------------------------------------------------------
bool FileExists(const char * path)
{
  DWORD dwAttrib = GetFileAttributes(path);
  return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
         !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

//----------------------------------------------------------------------
// Open virtual disk
//----------------------------------------------------------------------
void OpenDrive (int drive)
{

    drive &= 1;
    if (DiskImage[drive].hFile) {
        _DLOG("OpenDrive closing old handle %d\n",drive);
        CloseDrive(drive);
    }

    //char *file = DiskImageName[drive];
    char *file = DiskImage[drive].name;

    if (*file == '\0') {
        _DLOG("OpenDrive no image mounted drive %d\n",drive);
        CmdSta = STA_FAIL;
        return;
    }

    _DLOG("Open %d %s\n",drive,file);

    if (!FileExists(file)) {
        _DLOG("OpenDrive loaded image file does not exist\n");
        CmdSta = STA_FAIL;
        return;
    }

    if (DriveFileRec[drive].attrib & ATTR_DIR) {
        _DLOG("OpenDrive file record says it is a directory\n");
        CmdSta = STA_FAIL;
        return;
    }

    HANDLE hFile;
    hFile = CreateFile( file,
            GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == NULL) {
        _DLOG("OpenDrive %s drive %d file %s\n",
                   drive,file,LastErrorTxt());
        CmdSta = STA_FAIL;
    }

    DiskImage[drive].hFile = hFile;
    _DLOG("OpenDrive %d hfile %d\n",drive,DiskImage[drive].hFile);

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

    //_DLOG("UpdateSD %02X %02X %02X %02X %02X '%s'\n",
    //  CmdPrm1,CmdPrm2,CmdPrm3,RecvBuf[0],RecvBuf[1],&RecvBuf[2]);

    switch (RecvBuf[0]) {
    case 0x4d: //M
    case 0x6d: //m
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
        CmdSta = (InitiateDir(&RecvBuf[2])) ? 0 : STA_FAIL;
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
    char attr = FileAttrByte(path);
    if (attr == ATTR_INVALID) return false;
    if ((attr & ATTR_DIR) == 0) return false;
    return true;
}

//----------------------------------------------------------------------
// Set the Current Directory
//----------------------------------------------------------------------
bool SetCurDir(char * path)
{
    //_DLOG("SetCurdir %s\n",path);
    //TODO: Wildcards

    char fqp[MAX_PATH];
    char tmp[MAX_PATH] = {};

    // If blank is passed clear the current dirctory
    if (*path == '\0') {
        *CurDir = '\0';
        return true;
    }

    // If first char passed is not '/' append to previous
    if (*path != '/') {
        strncpy(tmp,CurDir,MAX_PATH);
        AppendPathChar(tmp,'/');
    }
    strncat(tmp,path,MAX_PATH);

    // Check for valid directory on SD
    strncpy(fqp,SDCard,MAX_PATH);
    strncat(fqp,tmp,MAX_PATH);

    if (!IsDirectory(fqp)) {
        _DLOG("SetCurDir invalid %s\n",fqp);
        CmdSta = STA_FAIL;
        return false;
    }

    strncpy(CurDir,tmp,MAX_PATH);
    _DLOG("SetCurdir %s\n",CurDir);
    return true;
}

//----------------------------------------------------------------------
// Initialize file search. Append "*.*" if pattern is a directory.
// Save the directory portion of the pattern for prepending to results
//----------------------------------------------------------------------
bool InitiateDir(const char * wildcard)
{
    // Make sure SDCard is a valid directory so we
    // don't get caught in a search loop
    if (!IsDirectory(SDCard)) {
        hFind = INVALID_HANDLE_VALUE;
        _DLOG("InitiateDir SDCard path invalid\n");
        return false;
    }

    char path[MAX_PATH];
    strncpy(path,SDCard,MAX_PATH);
    AppendPathChar(path,'/');
    strncat(path,CurDir,MAX_PATH);
    AppendPathChar(path,'/');
    strncat(path,wildcard,MAX_PATH);

    //Append '/*.*' if a directory
    if (IsDirectory(path)) {
        AppendPathChar(path,'/');
        strncat(path,"*.*",MAX_PATH);
    }

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

    LoadReply(dpage,256);
    return true;
}
