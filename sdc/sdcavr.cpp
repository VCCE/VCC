//----------------------------------------------------------------------
//
// This file is part of VCC (Virtual Color Computer).
// Vcc is Copyright 2015 by Joseph Forgione
//
// VCC (Virtual Color Computer) is free software, you can redistribute
// it and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
// the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//
// This is the Vcc SDC interface by E J Jaquay
//
//----------------------------------------------------------------------
//
//  CTRLATCH  $FF40 ; controller latch (write)
//  FLSHDAT   $FF42 ; flash data register
//  FLSHCTRL  $FF43 ; flash control register
//  CMDREG    $FF48 ; command register (write)
//  STATREG   $FF48 ; status register (read)
//  PREG1     $FF49 ; param register 1
//  PREG2     $FF4A ; param register 2
//  PREG3     $FF4B ; param register 3
//  DATREGA   PREG2 ; first data register
//  DATREGB   PREG3 ; second data register
//
//  Note this interface shares ports with the FD502 floppy controller.
//  A special value (0x43) hopefully not used by the floppy controller
//  is placed in the Latch Registor to enable SDC commands. SDC commands
//  are in the range 0x80 to 0xFF while FD502 commands do not use this
//  range. FD502 also uses status bit 0 to indicate controller busy.
//
//  FD502 uses FF40 for Disk enable, drive select, density, and motor
//  control. FF48 for status/command, FF49 for trk#, FF4A for Sec#,
//  and FF4B for data transfer.  Writing zero to FF40 will turn off the
//  drive motor.
//
//  Data written to the flash data port (0x42) can be read back.
//  When the flash data port (0x43) is written the three low bits
//  select the ROM from the corresponding flash bank (0-7). When
//  read the flash control port returns the currently selected bank
//  in the three low bits and the five bits from the Flash Data port.
//
//  Status bits:
//  0   Busy
//  1   Ready
//  2   Invalid path name
//  3   hardware error
//  4   Path not found
//  5   File in use
//  6   not used
//  7   Command failed
//
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
//----------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include "../defines.h"
#include "../fileops.h"
#include "../logger.h"
#include "sdcavr.h"

void SetMode(int);
void SDCCommand(int);
int  CalcSectorNum(int);
void ReadSector(int);
void StreamImage(int);
void WriteSector(int);
void UnusedCommand(int);
void GetDriveInfo(int);
void SDCControl(int);
void UpdateSD(int);
void LoadDrive (int,char *);
void UnLoadDrive (int);
void CloseDrive (int);
void OpenDrive (int);
void LoadReply(void *, int);
unsigned char PutByte();
void GetByte(unsigned char);
char * LastErrorTxt();
void CommandComplete();
void StartBlockRecv(int);
void BankSelect(int);
void LoadDirPage();

// Status Register values
#define STA_BUSY     0X01
#define STA_READY    0x02
#define STA_HWERROR  0x04
#define STA_CRCERROR 0x08
#define STA_NOTFOUND 0X10
#define STA_DELETED  0X20
#define STA_WPROTECT 0X40
#define STA_FAIL     0x80
// File Attribute values
#define ATTR_DIR 0x10
#define ATTR_SDF 0x04
#define ATTR_HIDDEN 0x02
#define ATTR_LOCKED 0x01

// SDC Latch flag
bool SDC_Mode=false;

// Current and previous command code
unsigned char CurCmdCode = 0;
unsigned char PrvCmdCode = 0;

// Command status and parameters
unsigned char CmdSta = 0;
unsigned char CmdRpy1 = 0;
unsigned char CmdRpy2 = 0;
unsigned char CmdRpy3 = 0;
unsigned char CmdPrm1 = 0;
unsigned char CmdPrm2 = 0;
unsigned char CmdPrm3 = 0;
unsigned char FlshDat = 0;

// Last drive lsn read or write. -1 means none
int LastDrive    = -1;
int LastReadLSN  = -1;
int LastWriteLSN = -1;

// Last block read from disk
char LastReadBlock[512];

// Rom bank currently selected
unsigned char CurrentBank = 0;

// Command send buffer. Block data from Coco lands here.
// Buffer is sized to hold at least 256 bytes. (for sector writes)
char RecvBuf[300];
int RecvCount = 0;
char *RecvPtr = RecvBuf;

// Command reply buffer. Block data to Coco goes here.
// Buffer is sized to hold at least 512 bytes (for streaming)
char ReplyBuf[600];
int ReplyCount = 0;
char *ReplyPtr = ReplyBuf;

// Drive filenames and handles
char FileName0[MAX_PATH] = {};
char FileName1[MAX_PATH] = {};
HANDLE hDrive[2] = {NULL,NULL};

// SD card root and the directories within.
char SDCard[MAX_PATH] = {};

// Drive info buffers for SDC-ROM
#pragma pack(1)
struct Drive_Info {
    char name[8];
    char type[3];
    char attrib;
    DWORD lo_size;
    DWORD hi_size;
    char bytes[240];
};
#pragma pack()
struct Drive_Info DriveInfo[2];

//======================================================================
// Init the controller
//----------------------------------------------------------------------
void SDCInit()
{
#ifdef _DEBUG_
    _DLOG("\n");
    HANDLE hLog = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT r; r.Left=0; r.Top=0; r.Right=40; r.Bottom=36;
    SetConsoleWindowInfo(hLog,true,&r);
#endif

// TODO look for startup.cfg, which typically contains
//   0=/SDCEXP.DSK

SetCDRoot("C:\\Users\\ed\\vcc\\sets\\SDC\\SDRoot");
LoadDrive(0,"SDCEXP.DSK");
LoadDrive(1,"PROG\\SIGMON6.DSK");

    CurrentBank = 0;
    SetMode(0);
    return;
}

//----------------------------------------------------------------------
// Reset the controller
//----------------------------------------------------------------------
void SDCReset()
{
    _DLOG("SDCREset Unloading Drives\n");
    UnLoadDrive (0);
    UnLoadDrive (1);
    SetMode(0);
    return;
}

//----------------------------------------------------------------------
// Write SDC port.  If a command needs a data block to complete it
// will put a count (256 or 512) in RecvCount.
//----------------------------------------------------------------------
void SDCWrite(unsigned char data,unsigned char port)
{
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
    }
    return;
}

//----------------------------------------------------------------------
// Get byte for receive buffer.
//----------------------------------------------------------------------
void GetByte(unsigned char byte)
{
    RecvCount--;
    if (RecvCount < 1) {
        CommandComplete();
        CmdSta = 0;
    } else {
        CmdSta |= STA_BUSY;
    }
    *RecvPtr++ = byte;
}

//----------------------------------------------------------------------
// Read SDC port. If there are bytes in the reply buffer return them.
//----------------------------------------------------------------------
unsigned char SDCRead(unsigned char port)
{
    unsigned char rpy;
    switch (port) {
    case 0x42:
        rpy = FlshDat;
        _DLOG("FDataR %02x\n",rpy);
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
// Put byte from reply into buffer.
//----------------------------------------------------------------------
unsigned char PutByte()
{
    ReplyCount--;
    if (ReplyCount < 1) {
        CmdSta = 0;
    } else {
        CmdSta |= STA_BUSY;
    }
    return *ReplyPtr++;
}

//----------------------------------------------------------------------
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
    if (data == 0) {
        SDC_Mode = false;
    } else if (data == 0x43) {
        SDC_Mode = true;
    }
    CmdSta  = 0;
    CmdRpy1 = 0;
    CmdRpy2 = 0;
    CmdRpy3 = 0;
    CmdPrm1 = 0;
    CmdPrm2 = 0;
    CmdPrm3 = 0;
    return;
}

//----------------------------------------------------------------------
//  Dispatch SDC commands
//----------------------------------------------------------------------
void SDCCommand(int code)
{
    // Save code for CommandComplete
    PrvCmdCode = CurCmdCode;
    CurCmdCode = code;

    // expect block data for Ax,Bx,Ex

    // The SDC uses the low nibble of the command code as an additional
    // argument so this gets passed to the command processor.
    int loNib = code & 0xF;
    int hiNib = code & 0xF0;

    switch (hiNib) {
    case 0x80:
        ReadSector(loNib);
        break;
    case 0x90:
        StreamImage(loNib);
        break;
    case 0xA0:
        StartBlockRecv(code);
        break;
    case 0xC0:
        GetDriveInfo(loNib);
        break;
    case 0xD0:
        SDCControl(loNib);
        break;
    case 0xE0:
        StartBlockRecv(code);
        break;
    default:
        _DLOG("SDC command %02X not supported\n",code);
        CmdSta = STA_FAIL;  // Fail
        break;
    }
    return;
}

//----------------------------------------------------------------------
// Command Ax or Ex block transfer start
//----------------------------------------------------------------------
void StartBlockRecv(int code)
{
    if (!SDC_Mode) {
        _DLOG("Not SDC mode block receive ignored %02X\n",code);
        CmdSta = STA_FAIL;
        return;
    } else if ((code & 4) !=0 ) {
        _DLOG("8bit block receive not supported %02X\n",code);
        CmdSta = STA_FAIL;
        return;
    }

    // Set up the receive buffer
    CmdSta = STA_READY;
    RecvPtr = RecvBuf;
    RecvCount = 256;
}

//----------------------------------------------------------------------
// Command Ax or Ex block transfer complete
//----------------------------------------------------------------------
void CommandComplete()
{
    int loNib = CurCmdCode & 0xF;
    int hiNib = CurCmdCode & 0xF0;
    switch (hiNib) {
    case 0xA0:
        WriteSector(loNib);
        break;
    case 0xE0:
        UpdateSD(loNib);
        break;
    default:
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
// Calculate sector.  numsides == 2 if double sided
//----------------------------------------------------------------------
int CalcSectorNum(int numsides)
{
    if (numsides != 2) {_DLOG("Not double sided\n");}
    return (CmdPrm1 << 16) + (CmdPrm2 << 8) + CmdPrm3;
}


//----------------------------------------------------------------------
// Read logical sector
//----------------------------------------------------------------------
void ReadSector(int loNib)
{
    if (!SDC_Mode) {
        _DLOG("Not SDC mode Read ignored\n");
        CmdSta = STA_FAIL;
        return;
    } else if ((loNib & 4) !=0 ) {
        _DLOG("Read 8bit transfer not supported\n");
        CmdSta = STA_FAIL;
        return;
    }

    int drive = loNib & 1;

    if (hDrive[drive] == NULL) OpenDrive(drive);
    if (hDrive[drive] == NULL) {
        CmdSta = STA_FAIL;
        return;
    }

    int lsn = CalcSectorNum(loNib&2);
    _DLOG("Read drive %d lsn %d\n",drive,lsn);

    LARGE_INTEGER pos;
    pos.QuadPart = lsn * 256;
    if (!SetFilePointerEx(hDrive[drive],pos,NULL,FILE_BEGIN)) {
        _DLOG("Seek error %s\n",LastErrorTxt());
        CmdSta = STA_FAIL;
    }

    DWORD cnt;
    DWORD num = 256;
    ReadFile(hDrive[drive],LastReadBlock,num,&cnt,NULL);
    if (cnt != num) {
        _DLOG("Read error %s\n",LastErrorTxt());
        CmdSta = STA_FAIL;
    } else {
        LastDrive = drive;
        LastReadLSN = lsn;
        LoadReply(LastReadBlock,256);
    }
}

//----------------------------------------------------------------------
// Stream image data
//----------------------------------------------------------------------
void StreamImage(int loNib)
{
    _DLOG("\nStream not supported\n");
    CmdSta = STA_FAIL;  // Fail
}

//----------------------------------------------------------------------
// Write logical sector
//----------------------------------------------------------------------
void WriteSector(int loNib)
{
    if (!SDC_Mode) {
        _DLOG("Not SDC mode Write ignored\n");
        CmdSta = STA_FAIL;
        return;
    } else if ((loNib & 4) !=0 ) {
        _DLOG("Read 8bit transfer not supported\n");
        CmdSta = STA_FAIL;
        return;
    }

    int drive = loNib & 1;

    if (hDrive[drive] == NULL) OpenDrive(drive);
    if (hDrive[drive] == NULL) {
        CmdSta = STA_FAIL|STA_NOTFOUND;
        return;
    }

    int lsn = CalcSectorNum(loNib&2);
    _DLOG("Write drive %d lsn %d\n",drive,lsn);

    LARGE_INTEGER pos;
    pos.QuadPart = lsn * 256;
    if (!SetFilePointerEx(hDrive[drive],pos,NULL,FILE_BEGIN)) {
        _DLOG("Seek error %s\n",LastErrorTxt());
        CmdSta = STA_FAIL|STA_NOTFOUND;
    }

    DWORD cnt;
    DWORD num = 256;
    WriteFile(hDrive[drive], RecvBuf, num,&cnt,NULL);
    if (cnt != num) {
        _DLOG("Write error %s\n",LastErrorTxt());
        CmdSta = STA_FAIL;
        CmdSta = STA_FAIL|STA_NOTFOUND;
    } else {
        LastDrive = drive;
        LastWriteLSN = lsn;
        CmdSta = 0;
    }
}

//----------------------------------------------------------------------
// Get drive information
//----------------------------------------------------------------------
void GetDriveInfo(int loNib)
{
     if (!SDC_Mode) {
        _DLOG("Not SDC mode GetInfo ignored\n");
        return;
     }

    int drive = loNib & 1;
    switch (CmdPrm1) {
    case 0x49:
        // I - return drive information in block
        LoadReply( (void *) &DriveInfo[drive],64);
        break;
    case 0x43:
        // C Return current directory in block
        _DLOG("GetInfo $%0X not supported\n",CmdPrm1);
        CmdSta = STA_FAIL;
        break;
    case 0x51:
        // Q Return the size of disk image in p1,p2,p3
        _DLOG("GetInfo $%0X not supported\n",CmdPrm1);
        CmdSta = STA_FAIL;
        break;
    case 0x3E:
        // > Return directory page in block. Each block contains
        // 16 directory entries. Command may be repeated until
        // all directory entries are returned
        LoadDirPage();
        break;
    case 0x2B:
        // + Mount next next disk in set.  Mounts disks with a
        // digit suffix, starting with '1'. May repeat
        _DLOG("GetInfo $%0X not supported\n",CmdPrm1);
        CmdSta = STA_FAIL;
        break;
    case 0x56:
        // V Get BCD firmware version number in p2, p3.
        CmdRpy2 = 0x01;
        CmdRpy3 = 0x13;
        break;
    case 0x65:
        // e Want something that is undocumented
        _DLOG("GetInfo $%0X (%c) unknown\n",CmdPrm1,CmdPrm1);
        CmdRpy1 = 0;
        CmdRpy2 = 0;
        CmdRpy3 = 0;
        break;
    default:
        _DLOG("GetInfo $%0X (%c) not supported\n",CmdPrm1,CmdPrm1);
        CmdSta = STA_FAIL;
        break;
    }
}

//----------------------------------------------------------------------
// Abort stream or mount disk in a set of disks.
// CmdPrm1  0: Next disk 1-9: specific disk.
// CmdPrm2 b0: Blink Enable
//----------------------------------------------------------------------
void SDCControl(int loNib)
{
    // If a transfer is in progress abort it.
    if ((ReplyCount > 0) || (RecvCount > 0)) {
        _DLOG("**Block Transfer Aborted**\n");
        ReplyCount = 0;
        RecvCount = 0;
        CmdSta = 0;
    } else {
        _DLOG("Mount disk in set not supported %d %d %d \n",
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
    if (CmdSta != 0) {
        _DLOG("Load reply busy\n");
        CmdSta = STA_FAIL;
        return;
    } else if ((count < 2) | (count > 512)) {
        _DLOG("Load reply bad count\n");
        CmdSta = STA_FAIL;
        return;
    }

    // Copy data to the reply buffer with bytes swapped in words
    char *dp = (char *) data;
    char *bp = ReplyBuf;
    char tmp;
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

    //_DLOG("Loaded %d reply bytes\n",count);
    return;
}

//----------------------------------------------------------------------
// Load Drive
//----------------------------------------------------------------------
void LoadDrive (int drive, char * name)
{
    int i;
    drive &= 1;
    char *pPath = (drive == 0) ? FileName0 : FileName1;
    strncpy(pPath, SDCard, MAX_PATH);
    strncat(pPath, "\\", MAX_PATH);
    strncat(pPath, name, MAX_PATH);

    memset(DriveInfo[drive].name,' ',8);
    memset(DriveInfo[drive].type,' ',8);

    char * p = strrchr(pPath,'\\') + 1;
    for (i=0;i<8;i++) {
        char c = *p++;
        if ((c == '.') || (c == 0)) break;
        DriveInfo[drive].name[i] = c;
    }
    for (i=0;i<3;i++) {
        char c = *p++;
        if (c == 0) break;
        DriveInfo[drive].type[i] = c;
    }
}

//----------------------------------------------------------------------
// Unload drive
//----------------------------------------------------------------------
void UnLoadDrive (int drive)
{
    CloseDrive(drive);
    memset((void *) &DriveInfo[drive], 0, sizeof(Drive_Info));
    if (drive == 0)
        *FileName0 = '\0';
    else
        *FileName1 = '\0';
}

//----------------------------------------------------------------------
// Check windows path
//----------------------------------------------------------------------
BOOL FileExists(LPCTSTR szPath)
{
  DWORD dwAttrib = GetFileAttributes(szPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
         !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

//----------------------------------------------------------------------
// Open virtual disk
//----------------------------------------------------------------------
void OpenDrive (int drive)
{
    drive &= 1;
    if (hDrive[drive]) CloseDrive(drive);

    char *file = (drive==0) ? FileName0 : FileName1;

    // Locate the file
    DWORD attr = GetFileAttributes(file);
    if ((attr == -1) || (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        _DLOG("\nFile not found %d\n",drive);
        CmdSta = STA_FAIL;
        return;
    }

    // TODO check for FILE_ATTRIBUTE_READONLY 0x1

    HANDLE hFile;
    hFile = CreateFile( file,
            GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == NULL) {
        _DLOG("Open %s drive %d file %s\n",
                   drive,file,LastErrorTxt());
        CmdSta = STA_FAIL;
    }
    hDrive[drive] = hFile;

    BY_HANDLE_FILE_INFORMATION fileinfo;
    GetFileInformationByHandle(hFile,&fileinfo);
    DriveInfo[drive].attrib = 0; // TODO set attribs
    DriveInfo[drive].lo_size = fileinfo.nFileSizeLow;
    DriveInfo[drive].hi_size = fileinfo.nFileSizeHigh;

//typedef struct _BY_HANDLE_FILE_INFORMATION {
//  DWORD    dwFileAttributes;
//  FILETIME ftCreationTime;
//  FILETIME ftLastAccessTime;
//  FILETIME ftLastWriteTime;
//  DWORD    dwVolumeSerialNumber;
//  DWORD    nFileSizeHigh;
//  DWORD    nFileSizeLow;
//  DWORD    nNumberOfLinks;
//  DWORD    nFileIndexHigh;
//  DWORD    nFileIndexLow;
//} *LPBY_HANDLE_FILE_INFORMATION;
}

//----------------------------------------------------------------------
// Close virtual disk
//----------------------------------------------------------------------
void CloseDrive (int drive)
{
    drive &= 1;
    if (hDrive[drive] != NULL) {
        _DLOG("\nClosing Drive %d\n",drive);
        CloseHandle(hDrive[drive]);
        hDrive[drive] = NULL;
    }
}

//----------------------------------------------------------------------
//  Update SD Commands
//----------------------------------------------------------------------
void UpdateSD(int loNib)
{
    _DLOG("UpdateSD %02X %02X %02X %02X %02X '%s'\n",
        CmdPrm1,CmdPrm2,CmdPrm3,RecvBuf[0],RecvBuf[1],&RecvBuf[2]);

    switch (RecvBuf[0]) {
    case 0x4d: //M
    case 0x6d:
        _DLOG("Mount image not Supported\n");
        CmdSta = STA_FAIL;
        break;
    case 0x4E: //N
    case 0x6E:
        //  $FF49 0 for DSK image, number of cylinders for SDF
        //  $FF4A hiNib 0 for DSK image, number of sides for SDF image
        //  $FF4B 0
        _DLOG("Mount new image not Supported\n");
        CmdSta = STA_FAIL;
        break;
    case 0x44: //D
        _DLOG("Set Current directory not Supported\n");
        CmdSta = STA_FAIL;
        break;
    case 0x4C: //L
        _DLOG("Initiate directory not Supported\n");
        CmdSta = STA_FAIL;
        break;
    case 0x4B: //K
        _DLOG("Create new directory not Supported\n");
        CmdSta = STA_FAIL;
        break;
    case 0x5B: //X
        _DLOG("Delete file or directory not Supported\n");
        CmdSta = STA_FAIL;
        break;
    default:
        _DLOG("Not Supported\n");
        CmdSta = STA_FAIL;
        break;
    }
}

//----------------------------------------------------------------------
// Set the CD root
//----------------------------------------------------------------------
void SetCDRoot(char * path)
{
    DWORD attr = GetFileAttributes(path);
    if ( (attr == INVALID_FILE_ATTRIBUTES) ||
         !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        _DLOG("\nInvalid CD root %X %s\n",attr,path);
        return;
    }
    strncpy(SDCard,path,MAX_PATH);
    return;
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//char CurrendDir[MAX_PATH];
//void ResetCurrentDir()
//{
//    strncpy(CurrentDir,SDCard,MAX_PATH);
//}

//----------------------------------------------------------------------
// Load directory page, return block containing an array of 16
// directory records:
//	0-7 File Name
//	8-10 Extension
//	11 Attribute Bits:
//		$10 Directory
//		$02 Hidden
//		$01 Locked
//		12-15 Size in bytes (MSB first)
//----------------------------------------------------------------------
void LoadDirPage()
{
_DLOG("\nLoad directory page\n");
    CmdSta = STA_FAIL;  // Fail
    return;
}
