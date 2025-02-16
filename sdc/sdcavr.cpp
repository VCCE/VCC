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
#include <sys/stat.h>
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
void LoadDrive (int,const char *);
void UnLoadDrive (int);
void CloseDrive (int);
void OpenDrive (int);
void LoadReply(void *, int);
unsigned char PutByte();
void ReceiveByte(unsigned char);
char * LastErrorTxt();
void CmdBlockRecvComplete();
void CmdBlockRecvStart(int);
void BankSelect(int);
void LoadDirPage();
bool InitiateDir(const char *);

// Status Register values
#define STA_BUSY     0X01
#define STA_READY    0x02
#define STA_HWERROR  0x04
#define STA_CRCERROR 0x08
#define STA_NOTFOUND 0X10
#define STA_DELETED  0X20
#define STA_WPROTECT 0X40
#define STA_FAIL     0x80

// SDC Latch flag
bool SDC_Mode=false;

// Current command code
unsigned char CurCmdCode = 0;

// Current sector for Disk I/O
int CurSectorNum = -1;

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

// SD card root
char SDCard[MAX_PATH] = {};

// File record
#define ATTR_NORM   0x00
#define ATTR_RDONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_DIR    0x10
#define ATTR_SDF    0x04
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

// Drive file info
struct FileRecord DriveFile[2];

// Windows file lookup handle and data
HANDLE hFind;
WIN32_FIND_DATAA dFound;

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

SetCDRoot("C:/Users/ed/vcc/sets/SDC/SDRoot");
LoadDrive(0,"SDCEXP.DSK");
LoadDrive(1,"PROG/SIGMON6.DSK");

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
        if (RecvCount > 0) ReceiveByte(data); else CmdPrm2 = data;
        break;
    case 0x4B:
        if (RecvCount > 0) ReceiveByte(data); else CmdPrm3 = data;
        break;
    }
    return;
}

//----------------------------------------------------------------------
// Get byte for receive buffer.
//----------------------------------------------------------------------
void ReceiveByte(unsigned char byte)
{
    RecvCount--;
    if (RecvCount < 1) {
        CmdBlockRecvComplete();
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
    RecvCount = 0;
    ReplyCount = 0;
    CurCmdCode = 0;
    return;
}

//----------------------------------------------------------------------
//  Dispatch SDC commands
//----------------------------------------------------------------------
void SDCCommand(int code)
{
    // If busy or tranfer in progress abort whatever
    if ((CmdSta & STA_BUSY) || (RecvCount > 0) || (ReplyCount > 0)) {
        _DLOG("*** ABORTING ***\n");
        SetMode(0);
        CmdSta = STA_FAIL;
        return;
    }

    //Invalidate sector number for disk I/O.
    //Set when disk I/O command is received.
    CurSectorNum = -1;
    // Save code for CmdBlockRecvComplete
    CurCmdCode = code;

    // The SDC uses the low nibble of the command code as an additional
    // argument so this gets passed to the command processor.
    int loNib = code & 0xF;
    int hiNib = code & 0xF0;

    switch (hiNib) {
    case 0x80:
        CurSectorNum = CalcSectorNum(loNib);
        ReadSector(loNib);
        break;
    case 0x90:
        CurSectorNum = CalcSectorNum(loNib);
        StreamImage(loNib);
        break;
    case 0xA0:
        CurSectorNum = CalcSectorNum(loNib);
        CmdBlockRecvStart(code);
        break;
    case 0xC0:
        GetDriveInfo(loNib);
        break;
    case 0xD0:
        SDCControl(loNib);
        break;
    case 0xE0:
        CmdBlockRecvStart(code);
        break;
    default:
        _DLOG("SDC command %02X not supported\n",code);
        CmdSta = STA_FAIL;  // Fail
        break;
    }
    return;
}

//----------------------------------------------------------------------
// Calculate sector.  numsides == 2 if double sided
//----------------------------------------------------------------------
int CalcSectorNum(int loNib)
{
    if (!(loNib & 2)) {_DLOG("Not double sided\n");}
    return (CmdPrm1 << 16) + (CmdPrm2 << 8) + CmdPrm3;
}

//----------------------------------------------------------------------
// Command Ax or Ex block transfer start
//----------------------------------------------------------------------
void CmdBlockRecvStart(int code)
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
    CmdSta = STA_READY | STA_BUSY;
    RecvPtr = RecvBuf;
    RecvCount = 256;
}

//----------------------------------------------------------------------
// Command Ax or Ex block transfer complete
//----------------------------------------------------------------------
void CmdBlockRecvComplete()
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
    } else if (CurSectorNum < 0) {
        _DLOG("Read invalid sector number\n");
        CmdSta = STA_FAIL;
        return;
    }

    int drive = loNib & 1;

    if (hDrive[drive] == NULL) OpenDrive(drive);
    if (hDrive[drive] == NULL) {
        CmdSta = STA_FAIL;
        return;
    }

    int lsn = CurSectorNum;

    //_DLOG("Read drive %d lsn %d\n",drive,lsn);

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
        LoadReply(LastReadBlock,256);
    }
}

//----------------------------------------------------------------------
// Stream image data
//----------------------------------------------------------------------
void StreamImage(int loNib)
{
    if (CurSectorNum < 0) {
        _DLOG("Stream invalid sector number\n");
        CmdSta = STA_FAIL;
        return;
    }

    _DLOG("Stream not supported\n");
    CmdSta = STA_FAIL;  // Fail
}

//----------------------------------------------------------------------
// Write logical sector
//----------------------------------------------------------------------
void WriteSector(int loNib)
{
    if (CurSectorNum < 0) {
        _DLOG("Write invalid sector number\n");
        CmdSta = STA_FAIL;
        return;
    }

    int drive = loNib & 1;
    if (hDrive[drive] == NULL) OpenDrive(drive);
    if (hDrive[drive] == NULL) {
        CmdSta = STA_FAIL|STA_NOTFOUND;
        return;
    }

    int lsn = CurSectorNum;
    _DLOG("Write drive %d sector %d\n",drive,lsn);

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
        LoadReply((void *) &DriveFile[drive],sizeof(FileRecord));
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

    //_DLOG("Load Reply %d\n",count);

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
// Load file record
//----------------------------------------------------------------------
bool LoadFileRecord(char * file, FileRecord * rec)
{
    int i;
    char c;
    memset(rec,0,sizeof(rec));

    char * p = strrchr(file,'/');
    if (p == NULL) {
        _DLOG("Load file record no backslash\n");
        return false;
    }
    p++;

    memset(rec->name,' ',8);
    for (i=0;i<8;i++) {
        c = *p++;
        if ((c == '.') || (c == 0)) break;
        rec->name[i] = c;
    }

    memset(rec->type,' ',3);
    if (c == '.') {
        for (i=0;i<3;i++) {
            c = *p++;
            if (c == 0) break;
            rec->type[i] = c;
        }
    }

    struct _stat fs;
    if (_stat(file,&fs) != 0) {
        _DLOG("File not found %s\n",file);
        return false;
    }

    //WIN32_FILE_ATTRIBUTE_DATA fdat;
    //GetFileAttributesEx(file, GetFileExInfoStandard, &fdat);
    //_DLOG("Load FR %s %d bytes\n",file,fdat.nFileSizeLow);

    rec->lolo_size = (fs.st_size) & 0xff;
    rec->hilo_size = (fs.st_size >> 8) & 0xff;
    rec->lohi_size = (fs.st_size >> 16) & 0xff;
    rec->hihi_size = (fs.st_size >> 24) & 0xff;

    rec->attrib = (fs.st_mode & _S_IFDIR) ? ATTR_DIR : ATTR_NORM;
    return true;
}

//----------------------------------------------------------------------
// Load Drive
//----------------------------------------------------------------------
void LoadDrive (int drive, const char * name)
{
    drive &= 1;
    char *pPath = (drive == 0) ? FileName0 : FileName1;
    strncpy(pPath, SDCard, MAX_PATH);
    strncat(pPath, "/", MAX_PATH);
    strncat(pPath, name, MAX_PATH);
    LoadFileRecord(pPath, &DriveFile[drive]);
}

//----------------------------------------------------------------------
// Unload drive
//----------------------------------------------------------------------
void UnLoadDrive (int drive)
{
    CloseDrive(drive);
    memset((void *) &DriveFile[drive], 0, sizeof(FileRecord));
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

    _DLOG("Opening %s\n",file);

    if (DriveFile[drive].attrib & ATTR_DIR) {
        _DLOG("File is a directory\n");
        CmdSta = STA_FAIL;
        return;
    }

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
        CmdSta = (InitiateDir(&RecvBuf[2])) ? 0 : STA_FAIL;
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
// Initialize Directory search
//----------------------------------------------------------------------

bool InitiateDir(const char * wildcard)
{
    char search[MAX_PATH];
    strncpy(search,SDCard,MAX_PATH);
    strncat(search,"/",MAX_PATH);
    strncat(search,wildcard,MAX_PATH);
    hFind = FindFirstFile(search, &dFound);
    return (hFind != INVALID_HANDLE_VALUE);
}

//----------------------------------------------------------------------
// Load directory page containing up to 16 file records:
//----------------------------------------------------------------------
void LoadDirPage()
{
    struct FileRecord dpage[16];
    memset(dpage,0,sizeof(dpage));
    char fqn[MAX_PATH];
    int cnt = 0;
    while (cnt < 16) {
        //_DLOG("%d %s\n",cnt,dFound.cFileName);
        if (FindNextFile(hFind,&dFound) == 0) break;
        if (*dFound.cFileName != '.') {
            strncpy(fqn,SDCard,MAX_PATH);
            strncat(fqn,"/",MAX_PATH);
            strncat(fqn,dFound.cFileName,MAX_PATH);
            LoadFileRecord(fqn,&dpage[cnt]);
            cnt++;
        }
        if (cnt == 16) break;
    }
    LoadReply(dpage,256);
    return;
}
