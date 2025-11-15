////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//
//	VCC (Virtual Color Computer) is free software: you can redistribute itand/or
//	modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your
//	option) any later version.
//
//	VCC (Virtual Color Computer) is distributed in the hope that it will be
//	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
//	Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with
//	VCC (Virtual Color Computer). If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////
//
//----------------------------------------------------------------------
// SDC simulator E J Jaquay 2025
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
//  NOTE: Stock SDC-DOS does not support the becker ports, it expects
//  drivewire to use the bitbanger ports. RGBDOS does, however, and will
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
//  The SDC simulator tries to simulate the commands as documented in
//  Darren Atkinson's CoCo SDC user guide.
//
//  Psuedo floppy interface
//  -----------------------
//  Some programs do not use SDC-DOS's disk driver, instead they contain
//  their own disk drivers to access drives 0 and 1. The SDC simulator
//  tries to emulate a fd502 interface to allow those programs to see
//  the SDC drives as floppy drives.
//
//  Flash data
//  ----------
//  There are eight 16K banks of programable flash memory. In this
//  simulator the banks are associated with ROM files where the
//  bank is stored. When a bank is selected the file is read into ROM.
//  The SDC-DOS RUN@n command can be used to select a bank.
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
//  This software was created a command at a time and is somewhat messy.
//  It also contains bugs and does not completely simulate all of the
//  CoCo SDC functioality. It could use a through going over.
//
//----------------------------------------------------------------------
//#define USE_LOGGING

#include <Windows.h>
#include <filesystem>
#include "vcc/devices/rtc/ds1315.h"
#include "vcc/utils/logger.h"
#include "../CartridgeMenu.h"

#include "sdc_cartridge.h"
#include "sdc_configuration.h"

// Return values for status register from CoCo SDC User Guide
constexpr unsigned char STA_NORMAL    = 0x00;
constexpr unsigned char STA_BUSY      = 0x01;
constexpr unsigned char STA_READY     = 0x02;   // reply from command is ready
constexpr unsigned char STA_INVALID   = 0x04;   // file or dir path is invalid
constexpr unsigned char STA_WIN_ERROR = 0x08;   // Misc windows error
constexpr unsigned char STA_INITERROR = 0x08;   // Directory not initiated
constexpr unsigned char STA_READERROR = 0x08;   // Read error
constexpr unsigned char STA_BAD_LSN   = 0x10;   // LSN or no image error
constexpr unsigned char STA_NOTFOUND  = 0x10;   // file (directory) not found
constexpr unsigned char STA_DELETED   = 0x20;   // Sector deleted mark
constexpr unsigned char STA_INUSE     = 0x20;   // image already in use
constexpr unsigned char STA_NOTEMPTY  = 0x20;   // Delete directory error
constexpr unsigned char STA_WPROTECT  = 0x40;   // write protect error
constexpr unsigned char STA_FAIL      = 0x80;

// Floppy mode (unlatched) status
constexpr unsigned char FLP_NORMAL    = 0x00;
constexpr unsigned char FLP_BUSY      = 0x01;   // b0 Busy
constexpr unsigned char FLP_DATAREQ   = 0x02;   // b1 DRQ
constexpr unsigned char FLP_DATALOST  = 0x04;   // b2 Lost data
constexpr unsigned char FLP_READERR   = 0x08;   // b3 Read error (CRC)
constexpr unsigned char FLP_SEEKERR   = 0x10;   // b4 Seek error
constexpr unsigned char FLP_WRITEERR  = 0x20;   // b5 Write error (Fault)
constexpr unsigned char FLP_READONLY  = 0x40;   // b6 Write protected
constexpr unsigned char FLP_NOTREADY  = 0x80;   // b7 Not ready

// Single byte file attributes for File info records
constexpr unsigned char ATTR_NORM     = 0x00;
constexpr unsigned char ATTR_RDONLY   = 0x01;
constexpr unsigned char ATTR_HIDDEN   = 0x02;
constexpr unsigned char ATTR_SDF      = 0x04;
constexpr unsigned char ATTR_DIR      = 0x10;

// Self imposed limit on maximum dsk file size.
constexpr unsigned int MAX_DSK_SECTORS = 2097152;

// SDC CoCo Interface
struct Interface
{
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
};

// Packed file records for directory list
#pragma pack(push,1)
struct SdcFile {
    char name[8];
    char type[3];
};
struct FileRecord {
    SdcFile file;
    char attrib;
    char hihi_size;
    char lohi_size;
    char hilo_size;
    char lolo_size;
};
#pragma pack(pop)

// Mounted image data
struct DiskImage {
    HANDLE hFile;
    unsigned int size;
    unsigned int headersize;
    DWORD sectorsize;
    DWORD tracksectors;
    char doublesided;
    char name[MAX_PATH];
    char fullpath[MAX_PATH];
    struct FileRecord filerec;
};

// Private functions
void AppendPathChar(char *,char c);
void FixSDCPath(char *,const char *);
void OpenNew(int,const char *,int);
bool SearchFile(const char *);
void InitiateDir(const char *);
void set_filerecord_file_size(FileRecord& , uint32_t);
void set_sdcfile_from_filename(SdcFile&, const std::string& );

//======================================================================
// Public Data
//======================================================================

// Idle Status counter
int idle_ctr = 0;

// Cart ROM
char PakRom[0x4000];

// Host paths for SDC
char IniFile[MAX_PATH] = {}; // Vcc ini file name
char SDCard[MAX_PATH] = {};  // SD card root directory

unsigned char EnableBankWrite = 0;
unsigned char BankWriteState = 0;

unsigned char StartupBank = 0;
unsigned char CurrentBank = 0xff;

char SDC_Status[16] = {};

//======================================================================
// Private Data
//======================================================================

static Interface IF = {};

static char CurDir[256] = {};       // SDC current directory
static char SeaDir[MAX_PATH] = {};  // Last directory searched

static struct FileRecord DirPage[16] = {};
static struct DiskImage Disk[2] = {};

// Flash banks
static FILE *h_RomFile = nullptr;
static unsigned char BankWriteNum = 0;
static unsigned char BankDirty = 0;
static unsigned char BankData = 0;

// Windows file lookup handle and data
static HANDLE hFind = INVALID_HANDLE_VALUE;
static WIN32_FIND_DATAA dFound;

// Streaming control
static int streaming;
static unsigned char stream_cmdcode;
static unsigned int stream_lsn;

// Floppy I/O
static char FlopDrive = 0;
static char FlopTrack = 0;
static char FlopSector = 0;
static char FlopStatus = 0;
static DWORD FlopWrCnt = 0;
static DWORD FlopRdCnt = 0;
static char FlopWrBuf[256];
static char FlopRdBuf[256];

//----------------------------------------------------------------------
// Init the controller. This gets called by PakReset
//----------------------------------------------------------------------
void sdc_cartridge::SDCInit()
{

#ifdef USE_LOGGING
    DLOG_C("\nSDCInit\n");
    MoveWindow(GetConsoleWindow(),0,0,300,800,TRUE);
#endif

    // Init the hFind handle (otherwise could crash on dll load)
    hFind = INVALID_HANDLE_VALUE;

    // Make sure drives are unloaded
    MountDisk (0,"",0);
    MountDisk (1,"",0);

    // Load SDC settings
    LoadConfig();

    // Load the startup rom
    LoadRom(StartupBank);

    SetCurDir(""); // May be changed by ParseStartup()

    // Process the startup config file
    ParseStartup();

    // init the interface
    memset(&IF,0,sizeof(IF));

    return;
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
        DLOG_C("LoadRom switching out dirty bank %d %s\n",CurrentBank,RomFile);
        h_RomFile = fopen(RomFile,"wb");
        if (h_RomFile == nullptr) {
            DLOG_C("LoadRom failed to open bank file%d\n",bank);
        } else {
            ctr = 0;
            p_rom = PakRom;
            while (ctr++ < 0x4000) fputc(*p_rom++, h_RomFile);
            fclose(h_RomFile);
            h_RomFile = nullptr;
        }
        BankDirty = 0;
    }

    DLOG_C("LoadRom load flash bank %d\n",bank);
    RomFile = FlashFile[bank];
    CurrentBank = bank;

    // If bank is empty and is the StartupBank load SDC-DOS
    if (*FlashFile[CurrentBank] == '\0') {
        DLOG_C("LoadRom bank %d is empty\n",CurrentBank);
        if (CurrentBank == StartupBank) {
            DLOG_C("LoadRom loading default SDC-DOS\n");
            strncpy(RomFile,"SDC-DOS.ROM",MAX_PATH);
        }
    }

    // Open romfile for read or write if not startup bank
    h_RomFile = fopen(RomFile,"rb");
    if (h_RomFile == nullptr) {
        if (CurrentBank != StartupBank) h_RomFile = fopen(RomFile,"wb");
    }
    if (h_RomFile == nullptr) {
        DLOG_C("LoadRom '%s' failed %s \n",RomFile,LastErrorTxt());
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
// Determine if path is a directory
//----------------------------------------------------------------------
bool IsDirectory(const char * path)
{
    std::filesystem::file_status stat = std::filesystem::status(path);
    return std::filesystem::is_directory(stat);
}
//
//----------------------------------------------------------------------
// Parse the startup.cfg file
//----------------------------------------------------------------------
void sdc_cartridge::ParseStartup()
{
    char buf[MAX_PATH+10];
    if (!IsDirectory(SDCard)) {
        DLOG_C("ParseStartup SDCard path invalid\n");
        return;
    }

    strncpy(buf,SDCard,MAX_PATH);
    AppendPathChar(buf,'/');
    strncat(buf,"startup.cfg",MAX_PATH);

    FILE *su = fopen(buf,"r");
    if (su == nullptr) {
        DLOG_C("ParseStartup file not found,%s\n",buf);
        return;
    }

    // Strict single char followed by '=' then path
    while (fgets(buf,sizeof(buf),su) != nullptr) {
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
void sdc_cartridge::CommandDone()
{
    DLOG_C("*");
    context_->assert_interrupt(INT_NMI, IS_NMI);
}

//----------------------------------------------------------------------
// Write port.
//----------------------------------------------------------------------
void sdc_cartridge::SDCWrite(unsigned char data,unsigned char port)
{
    if (port < 0x40 || port > 0x4F) return;  // Not disk data

    // Latched writes
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
        // Command param #2, #3, or receive block data. If a previous
        // command expects data it will have put a count in IF.bufcnt
        case 0x4A:
            if (IF.bufcnt > 0)
                BlockReceive(data);
            else
                IF.param2 = data;
            break;
        case 0x4B:
            if (IF.bufcnt > 0)
                BlockReceive(data);
            else
                IF.param3 = data;
            break;
        // Unhandled
        default:
            DLOG_C("SDCWrite L %02x %02x\n",port,data);
            break;
        }

    // Unlatched writes
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
            FloppyCommand(data);
            break;
        // floppy set track
        case 0x49:
            FloppyTrack(data);
            break;
        // floppy set sector
        case 0x4A:
            FloppySector(data);
            break;
        // floppy write data
        case 0x4B:
            FloppyWriteData(data);
            break;
        // Unhandled
        default:
            DLOG_C("SDCWrite U %02x %02x\n",port,data);
            break;
        }
    }
    return;
}

//----------------------------------------------------------------------
// Read port. If there are bytes in the reply buffer return them.
//----------------------------------------------------------------------
unsigned char sdc_cartridge::SDCRead(unsigned char port)
{
    unsigned char rpy = 0;

    // Latched reads
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
            DLOG_C("SDCRead L %02x\n",port);
            rpy = 0;
            break;
        }

    // Unlatched reads
    } else {
        switch (port) {
        // Flash control read is used by SDCDOS to detect the SDC
        case 0x43:
            rpy = CurrentBank | (BankData & 0xF8);
            break;
        // Floppy read status
        case 0x48:
            rpy = FloppyStatus();
            break;
        // Floppy read data
        case 0x4B:
            rpy = FloppyReadData();
            break;
        default: //TODO: why constant reads of ports 0x49,0x4a
            //DLOG_C("SDCRead U %02x\n",port);
            rpy = 0;
            break;
        }
    }
    return rpy;
}

//----------------------------------------------------------------------
// Floppy I/O
//----------------------------------------------------------------------
void sdc_cartridge::FloppyCommand(unsigned char data)
{
    switch (data >> 4) {
    // floppy restore
    case 0x0:
        FloppyRestore(data);
        break;
    case 0x1:
        FloppySeek(data);
        break;
    // floppy read sector
    case 0x8:
        FloppyReadDisk();
        break;
    // floppy write sector
    case 0xA:
        FloppyWriteDisk();
        break;
    }
}

// floppy restore
void sdc_cartridge::FloppyRestore(unsigned char data)
{
    DLOG_C("FloppyRestore\n");
    FlopTrack = 0;
    FlopSector = 0;
    FlopStatus = FLP_NORMAL;
    FlopWrCnt = 0;
    FlopRdCnt = 0;
}

// floppy seek
void sdc_cartridge::FloppySeek(unsigned char data)
{
    // Seek not implemented
    DLOG_C("FloppySeek\n");
}

// floppy read sector
void sdc_cartridge::FloppyReadDisk()
{
    int lsn = FlopTrack * 18 + FlopSector - 1;
    snprintf(SDC_Status,16,"SDC:%d Rd %d,%d",CurrentBank,FlopDrive,lsn);
    if (SeekSector(FlopDrive,lsn)) {
        if (ReadFile(Disk[FlopDrive].hFile,FlopRdBuf,256,&FlopRdCnt,nullptr)) {
            DLOG_C("FloppyReadDisk %d %d\n",FlopDrive,lsn);
            FlopStatus = FLP_DATAREQ;
        } else {
            DLOG_C("FloppyReadDisk read error %d %d\n",FlopDrive,lsn);
            FlopStatus = FLP_READERR;
        }
    } else {
        DLOG_C("FloppyReadDisk seek error %d %d\n",FlopDrive,lsn);
        FlopStatus = FLP_SEEKERR;
    }
}

// floppy write sector
void sdc_cartridge::FloppyWriteDisk()
{
    // write not implemented
    int lsn = FlopTrack * 18 + FlopSector - 1;
    DLOG_C("FloppyWriteDisk %d %d not implmented\n",FlopDrive,lsn);
    FlopStatus = FLP_READONLY;
}

// floppy set track
void sdc_cartridge::FloppyTrack(unsigned char data)
{
    //DLOG_C("FloppyTrack %d\n",data);
    FlopTrack = data;
}

// floppy set sector
void sdc_cartridge::FloppySector(unsigned char data)
{
    FlopSector = data;  // (1-18)

    int lsn = FlopTrack * 18 + FlopSector - 1;
    //DLOG_C("FloppySector %d lsn %d\n",FlopSector,lsn);
    FlopStatus = FLP_NORMAL;
}

// floppy write data
void sdc_cartridge::FloppyWriteData(unsigned char data)
{
    if (FlopWrCnt<256)  {
        FlopWrCnt++;
        FlopWrBuf[FlopWrCnt] = data;
    } else {
        if ((FlopStatus &= FLP_DATAREQ) != 0) CommandDone();
        FlopStatus = FLP_NORMAL;
    }
}

// floppy get status
unsigned char sdc_cartridge::FloppyStatus()
{
    //DLOG_C("FloppyStatus %02x\n",FlopStatus);
    return FlopStatus;
}

// floppy read data
unsigned char sdc_cartridge::FloppyReadData()
{
    unsigned char rpy;
    if (FlopRdCnt>0)  {
        rpy = FlopRdBuf[256-FlopRdCnt];
        FlopRdCnt--;
    } else {
        if ((FlopStatus &= FLP_DATAREQ) != 0) CommandDone();
        FlopStatus = FLP_NORMAL;
        rpy = 0;
    }
    return rpy;
}

//----------------------------------------------------------------------
// Can't reply with words, only bytes.  But the SDC interface design
// has most replies in words and the order the word bytes are read can
// vary so we play games to send the right ones
//----------------------------------------------------------------------
unsigned char sdc_cartridge::PickReplyByte(unsigned char port)
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
    if ((IF.bufcnt < 1) && streaming) StreamImage();

    return rpy;
}

//----------------------------------------------------------------------
//  Dispatch SDC commands
//----------------------------------------------------------------------
void sdc_cartridge::SDCCommand()
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
void sdc_cartridge::BlockReceive(unsigned char byte)
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
            DLOG_C("BlockReceive invalid cmd %d\n",IF.cmdcode);
            IF.status = STA_FAIL;
            break;
        }
    }
}

//----------------------------------------------------------------------
// Get drive information
//----------------------------------------------------------------------
void sdc_cartridge::GetDriveInfo()
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
void sdc_cartridge::GetDirectoryLeaf()
{
    DLOG_C("GetDirectoryLeaf CurDir '%s'\n",CurDir);

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
		const char *p = strrchr(CurDir,'/');
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
void sdc_cartridge::UpdateSD()
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
        DLOG_C("UpdateSD %02x not Supported\n",IF.blkbuf[0]);
        IF.status = STA_FAIL;
        break;
    }
}

//----------------------------------------------------------------------
// Flash control
//----------------------------------------------------------------------
void sdc_cartridge::FlashControl(unsigned char data)
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
unsigned char sdc_cartridge::WriteFlashBank(unsigned short adr)
{
    DLOG_C("WriteFlashBank %d %d %04X %02X\n",
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
bool sdc_cartridge::SeekSector(unsigned char cmdcode, unsigned int lsn)
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
        DLOG_C("SeekSector exceed max image %d %d\n",drive,lsn);
        return false;
    }

    // Seek to logical sector on drive.
    LARGE_INTEGER pos;
    pos.QuadPart = lsn * Disk[drive].sectorsize + Disk[drive].headersize;
    if (!SetFilePointerEx(Disk[drive].hFile,pos,nullptr,FILE_BEGIN)) {
        DLOG_C("SeekSector error %s\n",LastErrorTxt());
        return false;
    }
    return true;
}

//----------------------------------------------------------------------
// Read a sector from drive image and load reply
//----------------------------------------------------------------------
bool sdc_cartridge::ReadDrive(unsigned char cmdcode, unsigned int lsn)
{
    char buf[520];
    DWORD cnt = 0;
    int drive = cmdcode & 1;
    if (Disk[drive].hFile == nullptr) {
        DLOG_C("ReadDrive %d not open\n");
        return false;
    }

    if (!SeekSector(cmdcode,lsn)) {
        return false;
    }

    if (!ReadFile(Disk[drive].hFile,buf,Disk[drive].sectorsize,&cnt,nullptr)) {
        DLOG_C("ReadDrive %d %s\n",drive,LastErrorTxt());
        return false;
    }

    if (cnt != Disk[drive].sectorsize) {
        DLOG_C("ReadDrive %d short read\n",drive);
        return false;
    }

    snprintf(SDC_Status,16,"SDC:%d Rd %d,%d",CurrentBank,drive,lsn);
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
void sdc_cartridge::ReadSector()
{
    unsigned int lsn = (IF.param1 << 16) + (IF.param2 << 8) + IF.param3;

    //DLOG_C("R%d\n",lsn);

    IF.reply_mode = ((IF.cmdcode & 4) == 0) ? 0 : 1; // words : bytes
    if (!ReadDrive(IF.cmdcode,lsn))
        IF.status = STA_FAIL | STA_READERROR;
    else
        IF.status = STA_NORMAL;
}

//----------------------------------------------------------------------
// Stream image data
//----------------------------------------------------------------------
void sdc_cartridge::StreamImage()
{
    // If already streaming continue
    if (streaming) {
        stream_lsn++;
    // Else start streaming
    } else {
        stream_cmdcode = IF.cmdcode;
        IF.reply_mode = ((IF.cmdcode & 4) == 0) ? 0 : 1;
        stream_lsn = (IF.param1 << 16) + (IF.param2 << 8) + IF.param3;
        DLOG_C("StreamImage lsn %d\n",stream_lsn);
    }

    // For now can only stream 512 byte sectors
    int drive = stream_cmdcode & 1;
    Disk[drive].sectorsize = 512;
    Disk[drive].tracksectors = 9;

    if (stream_lsn > (Disk[drive].size/Disk[drive].sectorsize)) {
        DLOG_C("StreamImage done\n");
        streaming = 0;
        return;
    }

    if (!ReadDrive(stream_cmdcode,stream_lsn)) {
        DLOG_C("StreamImage read error %s\n",LastErrorTxt());
        IF.status = STA_FAIL;
        streaming = 0;
        return;
    }
    streaming = 1;
}

//----------------------------------------------------------------------
// Write logical sector
//----------------------------------------------------------------------
void sdc_cartridge::WriteSector()
{
    DWORD cnt = 0;
    int drive = IF.cmdcode & 1;
    unsigned int lsn = (IF.param1 << 16) + (IF.param2 << 8) + IF.param3;
    snprintf(SDC_Status,16,"SDC:%d Wr %d,%d",CurrentBank,drive,lsn);

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
        DLOG_C("WriteSector %d %s\n",drive,LastErrorTxt());
        IF.status = STA_FAIL;
        return;
    }
    if (cnt != Disk[drive].sectorsize) {
        DLOG_C("WriteSector %d short write\n",drive);
        IF.status = STA_FAIL;
        return;
    }
    IF.status = 0;
    return;
}

//----------------------------------------------------------------------
// Get most recent windows error text
//----------------------------------------------------------------------
char * sdc_cartridge::LastErrorTxt() {
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
void  sdc_cartridge::GetSectorCount() {

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
void sdc_cartridge::GetMountedImageRec()
{
    int drive = IF.cmdcode & 1;
    //DLOG_C("GetMountedImageRec %d %s\n",drive,Disk[drive].fullpath);
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
void sdc_cartridge::SDCControl()
{
    // If streaming is in progress abort it.
    if (streaming) {
        DLOG_C ("Streaming abort");
        streaming = 0;
        IF.status = STA_READY;
        IF.bufcnt = 0;
    } else {
        // TODO: Mount in set
        DLOG_C("SDCControl Mount in set unsupported %d %d %d \n",
                  IF.cmdcode,IF.param1,IF.param2);
        IF.status = STA_FAIL | STA_NOTFOUND;
    }
}

//----------------------------------------------------------------------
// Load reply. Count is bytes, 512 max.
//----------------------------------------------------------------------
void sdc_cartridge::LoadReply(const void *data, int count)
{
    if ((count < 2) | (count > 512)) {
        DLOG_C("LoadReply bad count %d\n",count);
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
// Set file size in file record
//----------------------------------------------------------------------
void set_filerecord_file_size(FileRecord& rec, uint32_t size) {
     rec.hihi_size = static_cast<char>((size >> 24) & 0xFF);
     rec.lohi_size = static_cast<char>((size >> 16) & 0xFF);
     rec.hilo_size = static_cast<char>((size >> 8)  & 0xFF);
     rec.lolo_size = static_cast<char>(size & 0xFF);
}

//----------------------------------------------------------------------
// Set file name in file record
//----------------------------------------------------------------------
void set_sdcfile_from_filename(SdcFile& sdcfile, const std::string& filename) {
    auto dot = filename.find_last_of('.');
    std::string base = (dot != std::string::npos) ? filename.substr(0, dot) : filename;
    std::string ext  = (dot != std::string::npos) ? filename.substr(dot + 1) : "";
    std::memset(&sdcfile,' ',sizeof(sdcfile));
    std::memcpy(sdcfile.name, base.data(), base.length());
    std::memcpy(sdcfile.type, ext.data(), ext.length());
}

//----------------------------------------------------------------------
// SDC uses a packed 8.3 format but users will usually input paths with
// normalized format. FixSDCPath normalizes a path if it is 8.3 packed.
//----------------------------------------------------------------------
void FixSDCPath(char *dst, const char *src, std::size_t dst_size)
{
    if (!dst || !src || dst_size == 0) return;

    std::size_t dst_ndx = 0;
    std::size_t src_ndx = 0;

    // Copy directory portion of src path
    const char * slash = strrchr(src,'/');
    if (slash != nullptr ) {
        size_t len = slash-src+1;
        memcpy(dst,src,len);
        src_ndx = len;
        dst_ndx = len;
    }

    std::size_t name_len = 0;
    bool dot_inserted = false;

    // Copy name and extension with paked format conversion
    while (src[src_ndx] != '\0' && dst_ndx + 1 < dst_size) {
        char ch = src[src_ndx++];
        if ((ch == ' ' || ch == '.') && !dot_inserted) {
            if (dst_ndx + 1 < dst_size) {
                dst[dst_ndx++] = '.';
                dot_inserted = true;
            }
        } else if (ch != ' ') {
            dst[dst_ndx++] = ch;
            if (!dot_inserted) {
                if (++name_len == 8 && dst_ndx + 1 < dst_size) {
                    dst[dst_ndx++] = '.';
                    dot_inserted = true;
                }
            }
        }
    }

    // Terminate the destination char array
    dst[dst_ndx] = '\0';

    return;
}

//----------------------------------------------------------------------
// Load a file record with the file found by Find File
//----------------------------------------------------------------------
bool sdc_cartridge::LoadFoundFile(struct FileRecord * rec)
{
    memset(rec,0,sizeof(rec));

    // If CurDir is not root and filename is ".." set to ".."
    if ((strcmp(CurDir,"")!=0) && (strcmp(dFound.cFileName,"..")==0)) {
        std::memset(&rec->file,' ',sizeof(SdcFile));
        std::memset(rec->file.name,'.',2);
        rec->attrib = ATTR_DIR;
        return true;
    }

    // Ignore any other filename that starts with a dot.
    if (dFound.cFileName[0] == '.' ) {
        return false;
    }

    // Load file name and type
    set_sdcfile_from_filename(rec->file,dFound.cFileName);

    // Attributes
    if (dFound.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
        rec->attrib |= ATTR_RDONLY;
    }

    // Directory does not need size
    if (dFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        rec->attrib |= ATTR_DIR;
        return true;
    }

    set_filerecord_file_size(*rec, dFound.nFileSizeLow);
    return true;
}

//----------------------------------------------------------------------
// Create and mount a new disk image
//  IF.param1 == 0 create 161280 byte JVC file
//  IF.param1 SDF image number of cylinders
//  IF.param2 SDC image number of sides
//----------------------------------------------------------------------
void sdc_cartridge::MountNewDisk (int drive, const char * path, int raw)
{
    //DLOG_C("MountNewDisk %d %s %d\n",drive,path,raw);

    // limit drive to 0 or 1
    drive &= 1;

    // Close and clear previous entry
    CloseDrive(drive);
    memset((void *) &Disk[drive],0,sizeof(DiskImage));

    // Convert from 8.3 format if used
    char file[MAX_PATH];
    FixSDCPath(file,path,MAX_PATH);

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
void sdc_cartridge::MountDisk (int drive, const char * path, int raw)
{
    DLOG_C("MountDisk %d %s %d\n",drive,path,raw);

    drive &= 1;

    // Close and clear previous entry
    CloseDrive(drive);
    memset((void *) &Disk[drive],0,sizeof(DiskImage));

    // Check for UNLOAD.  Path will be an empty string.
    if (*path == '\0') {
        DLOG_C("MountDisk unload %d %s\n",drive,path);
        IF.status = STA_NORMAL;
        return;
    }

    char file[MAX_PATH];
    char tmp[MAX_PATH];

    // Convert from 8.3 format if used
    FixSDCPath(file,path,MAX_PATH);

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
        DLOG_C("MountDisk not found '%s'\n",file);
        IF.status = STA_FAIL | STA_NOTFOUND;
        return;
    }

    // Mount first image found
    OpenFound(drive,raw);
    return;
}

//----------------------------------------------------------------------
// Retrieve disk name for configure
//----------------------------------------------------------------------
std::string DiskName(int drive)
{
    return Disk[drive].name;
}

//----------------------------------------------------------------------
// Mount Next Disk from found set
//----------------------------------------------------------------------
bool sdc_cartridge::MountNext (int drive)
{
    if (FindNextFile(hFind,&dFound) == 0) {
        DLOG_C("MountNext no more\n");
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
void sdc_cartridge::OpenNew( int drive, const char * path, int raw)
{
    DLOG_C("OpenNew %d '%s' %d %d %d\n",
          drive,path,raw,IF.param1,IF.param2);

    // Number of sides controls file type
    switch (IF.param2) {
    case 0:    //NEW
        // create JVC DSK file
        break;
    case 1:    //NEW+
    case 2:    //NEW++
        DLOG_C("OpenNew SDF file not supported\n");
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
        DLOG_C("OpenNew %s is a directory\n",fqn);
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
        DLOG_C("OpenNew fail %d file %s\n",drive,Disk[drive].fullpath);
        DLOG_C("... %s\n",LastErrorTxt());
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
void sdc_cartridge::OpenFound (int drive,int raw)
{
    drive &= 1;
    int writeprotect = 0;

    DLOG_C("OpenFound %d %s %d\n",
        drive, dFound.cFileName, Disk[drive].hFile);

    CloseDrive(drive);
    *Disk[drive].name = '\0';

    // Open a directory ontaining DSK files
    if (dFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        DLOG_C("OpenFound %s is a directory\n",dFound.cFileName);
        char path[MAX_PATH];
        strncpy(path,dFound.cFileName,MAX_PATH);
        AppendPathChar(path,'/');
        strncat(path,"*.DSK",MAX_PATH);
        InitiateDir(path);
        // TODO Fix this nonsensical recursion
        if (IF.status == STA_NORMAL) OpenFound(drive,0);
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
        DLOG_C("OpenFound fail %d file %s\n",drive,fqn);
        DLOG_C("... %s\n",LastErrorTxt());
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
            DLOG_C("OpenFound header read error\n");
            IF.status = STA_FAIL | STA_INVALID;
            return;
        }

        // Check for SDF file. SDF file has a 512 byte header and each
        // track record is 6250 bytes. Assume at least 35 tracks so
        // minimum size of a SDF file is 219262 bytes. The first four
        // bytes of the header contains "SDF1"
        if ((Disk[drive].size >= 219262) &&
            (strncmp("SDF1",(const char *) header,4) == 0)) {
            DLOG_C("OpenFound SDF file unsupported\n");
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
            DLOG_C("OpenFound unsuported image type %d %d\n",
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
            DLOG_C("OpenFound reopen fail %d\n",drive);
            DLOG_C("... %s\n",LastErrorTxt());
            IF.status = STA_FAIL | STA_WIN_ERROR;
            return;
        }
    }

    if (drive == 0) update_disk0_box();

    IF.status = STA_NORMAL;
    return;
}

//----------------------------------------------------------------------
// Convert file name from 8.3 format and prepend current dir.
//----------------------------------------------------------------------
void sdc_cartridge::GetFullPath(char * path, const char * file) {
    char tmp[MAX_PATH];
    strncpy(path,SDCard,MAX_PATH);
    AppendPathChar(path,'/');
    strncat(path,CurDir,MAX_PATH);
    AppendPathChar(path,'/');
    FixSDCPath(tmp,file,MAX_PATH);
    strncat(path,tmp,MAX_PATH);
}

//----------------------------------------------------------------------
// Rename disk or directory
//   names contains two consecutive null terminated name strings
//   first is file or directory to rename, second is target name
//----------------------------------------------------------------------
void sdc_cartridge::RenameFile(const char *names)
{
    char from[MAX_PATH];
    char target[MAX_PATH];

    GetFullPath(from,names);
    GetFullPath(target,1+strchr(names,'\0'));
    DLOG_C("UpdateSD rename %s %s\n",from,target);

    if (!std::filesystem::exists(from)) {
        IF.status = STA_FAIL | STA_NOTFOUND;
        return;
    }

    if (std::filesystem::exists(target)) {
        IF.status = STA_FAIL | STA_INVALID;
        return;
    }

    try {
        std::filesystem::rename(from, target);
        IF.status = STA_NORMAL;
    } catch (const std::filesystem::filesystem_error&) {
        IF.status = STA_FAIL | STA_WIN_ERROR;
    }
    return;
}

//----------------------------------------------------------------------
// Delete disk image or directory
//----------------------------------------------------------------------
void sdc_cartridge::KillFile(const char *file)
{
    char path[MAX_PATH];
    GetFullPath(path,file);
    DLOG_C("KillFile delete %s\n",path);

    if (!std::filesystem::exists(path)) {
        IF.status = STA_FAIL | STA_NOTFOUND;
        return;
    }

    if (IsDirectory(path)) {
        if (!std::filesystem::is_empty(path)) {
            IF.status = STA_FAIL | STA_NOTEMPTY;
            return;
        }
    }

    if (std::filesystem::remove(path)) {
        IF.status = STA_NORMAL;
    } else {
        IF.status = STA_FAIL | STA_WIN_ERROR;
    }
    return;
}

//----------------------------------------------------------------------
// Create directory
//----------------------------------------------------------------------
void sdc_cartridge::MakeDirectory(const char *name)
{
    char path[MAX_PATH];
    GetFullPath(path,name);
    DLOG_C("MakeDirectory %s\n",path);

    if (std::filesystem::exists(path)) {
        IF.status = STA_FAIL | STA_INVALID;
        return;
    }

    if (std::filesystem::create_directory(path)) {
        IF.status = STA_NORMAL;
    } else {
        DLOG_C("MakeDirectory %s\n", strerror(errno));
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
void sdc_cartridge::AppendPathChar(char * path, char c)
{
    int l = strlen(path);
    if ((l > 0) && (path[l-1] == c)) return;
    if (l > (MAX_PATH-2)) return;
    path[l] = c;
    path[l+1] = '\0';
}

//----------------------------------------------------------------------
// Set the Current Directory relative to previous current or to SDRoot
// This is complicated by the many ways a user can change the directory
//----------------------------------------------------------------------
void sdc_cartridge::SetCurDir(const char * branch)
{
    DLOG_C("SetCurdir '%s'\n",branch);

    // If branch is "." or "" do nothing
    if ((*branch == '\0') || (strcmp(branch,".") == 0)) {
        DLOG_C("SetCurdir no change\n");
        IF.status = STA_NORMAL;
        return;
    }

    // If branch is "/" set CurDir to root
    if (strcmp(branch,"/") == 0) {
        DLOG_C("SetCurdir to root\n");
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
        DLOG_C("SetCurdir back %s\n",CurDir);
        IF.status = STA_NORMAL;
        return;
    }

    // Disallow branch start with "//"
    if (strncmp(branch,"//",2) == 0) {
        DLOG_C("SetCurdir // invalid\n");
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
        DLOG_C("SetCurdir not a directory %s\n",test);
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

    DLOG_C("SetCurdir set to '%s'\n",CurDir);

    IF.status = STA_NORMAL;
    return;
}

//----------------------------------------------------------------------
//  Start File search.  Searches start from the root of the SDCard.
//----------------------------------------------------------------------
bool sdc_cartridge::SearchFile(const char * pattern)
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

    DLOG_C("SearchFile %s\n",path);

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
void sdc_cartridge::InitiateDir(const char * path)
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
void sdc_cartridge::LoadDirPage()
{
    memset(DirPage,0,sizeof(DirPage));

    if (hFind == INVALID_HANDLE_VALUE) {
        DLOG_C("LoadDirPage Search fail\n");
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
