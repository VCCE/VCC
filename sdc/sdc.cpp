#define USE_LOGGING
//======================================================================
// SDC simulator.  EJ Jaquay 2025
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
//======================================================================
//
//  SDC Floppy port
//  ---------------
//  The SDC interface shares ports with the FDC floppy emulator.
//
//  $FF40 ; SDC latch / FDC control
//  $FF42 ; flash data register
//  $FF43 ; flash control register
//  $FF48 ; command / status
//  $FF49 ; param register 1 / FDC Track
//  $FF4A ; param register 2 / FDC Sector
//  $FF4B ; param register 3 / FDC Data
//
//  The FDC emulation uses following ports;
//
//  $FF40 ; Control register (write)
//      Bit 7 halt flag 0 = disabled 1 = enabled
//      Bit 6 drive select 3 or side select
//      Bit 5 density flag 0 = single 1 = double
//      Bit 4 write precompensation 0 = no precomp 1 = precomp
//      Bit 3 drive motor enable 0 = motors off 1 = motors on
//      Bit 2 drive select 2
//      Bit 1 drive select 1
//      Bit 0 drive select 0
//
//  SDC latch code is 0x43 which is unlikey to be used for FDC
//  control because that would be multiple drives selected.
//
//  $FF48 ; Command register (write)
//      high order nibble; low order nibble type; command
//      0x0 ;   I ; Restore
//      0x1 ;   I ; Seek
//      0x2 ;   I ; Step
//      0x4 ;   I ; Step in
//      0x5 ;   I ; Step out
//      0x8 ;  II ; Read sector
//      0x9 ;  II ; Read sector mfm
//      0xA ;  II ; write sector
//      0xB ;  II ; write sector mfm
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
//
//  The becker port (drivewire) uses port $FF41 for status and port $FF42
//  for data so these must be always allowed for becker.dll to work.
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
//  Data written to the flash data port (0x42) can be read back.
//  When the flash data port (0x43) is written the three low bits
//  select the ROM from the corresponding flash bank (0-7). When
//  read the flash control port returns the currently selected bank
//  in the three low bits and the five bits from the Flash Data port.
//
//  SDC-DOS is typically in bank zero.
//======================================================================

#include <string>
#include <algorithm>
#include <iostream>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <Windows.h>
#include <windowsx.h>
#include <Shlwapi.h>
#pragma warning(push)
#pragma warning(disable:4091)
#include <ShlObj.h>
#pragma warning(pop)
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <vcc/devices/cloud9.h>
#include <vcc/core/logger.h>
#include <vcc/core/DialogOps.h>
#include "../CartridgeMenu.h"
#include <vcc/bus/cpak_cartridge_definitions.h>
#include <vcc/core/limits.h>
#include "sdc.h"

static ::vcc::devices::rtc::cloud9 cloud9_rtc;


//=========================================================================
// Host file utilities.  Most of these are general purpose
//=========================================================================

// Get most recent windows error text
inline const char * LastErrorTxt();
inline std::string LastErrorString();

// Convert backslashes to slashes in directory string / char
void FixDirSlashes(std::string &dir);
void FixDirSlashes(char *dir);

// copy string into fixed-size char array and blank-pad
template <size_t N>
void copy_to_fixed_char(char (&dest)[N], const std::string& src);

// Return copy of string with spaces trimmed from end of a string
inline std::string trim_right_spaces(const std::string &s);

// Convert LFN to FAT filename parts, 8 char name, 3 char ext
void sfn_from_lfn(char (&name)[8], char (&ext)[3], const std::string& lfn);

// Convert FAT file name parts to LFN. Returns empty string if invalid LFN
std::string lfn_from_sfn(const char (&name)[8], const char (&ext)[3]);

// Return slash normalized directory part of a path
inline std::string GetDirectoryPart(const std::string& input);

// Return filename part of a path
inline std::string GetFileNamePart(const std::string& input);

// SDC interface often presents a path which does not use a dot to 
// delimit name and extension: "FOODIR/FOO     DSK" -> FOODIR/FOO.DSK
std::string FixFATPath(const std::string& sdcpath);
void FixFATPath(char* path, const char* sdcpath);

// Determine if path is a direcory
inline bool IsDirectory(const std::string& path);

//----------------------------------------------------------------------
// Get most recent windows error text
//----------------------------------------------------------------------
inline const char * LastErrorTxt() {
    static char msg[200];
    DWORD error_code = GetLastError();
    FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr, error_code,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    msg, sizeof(msg), nullptr );
    return msg;
}
inline std::string LastErrorString() {
    return LastErrorTxt();
}

//-------------------------------------------------------------------
// SDC expects forward slashes as path delimiter, not backslashes
//-------------------------------------------------------------------
void FixDirSlashes(std::string &dir)
{
    if (dir.empty()) return;
    std::replace(dir.begin(), dir.end(), '\\', '/');
    if (dir.back() == '/') dir.pop_back();
}
void FixDirSlashes(char *dir)
{
    if (!dir) return;
    std::string tmp(dir);
    FixDirSlashes(tmp);
    strcpy(dir, tmp.c_str());
}

//-------------------------------------------------------------------
// Copy string to fixed size char array (non terminated)
//-------------------------------------------------------------------
template <size_t N>
void copy_to_fixed_char(char (&dest)[N], const std::string& src)
{
    size_t i = 0;
    for (; i < src.size() && i < N; ++i)
        dest[i] = src[i];
    for (; i < N; ++i)
        dest[i] = ' ';
}

//-------------------------------------------------------------------
// Return copy of string with spaces trimmed from end of a string
//-------------------------------------------------------------------
inline std::string trim_right_spaces(const std::string& s)
{
    size_t end = s.find_last_not_of(' ');
    if (end == std::string::npos)
        return {};
    return s.substr(0, end + 1);
}

//-------------------------------------------------------------------
// Convert LFN to FAT filename parts, 8 char name, 3 char ext
// A LNF file is less than 4GB and has a short (8.3) name.
//-------------------------------------------------------------------
void sfn_from_lfn(char (&name)[8], char (&ext)[3], const std::string& lfn)
{
    // Special case: parent directory
    if (lfn == "..") {
        copy_to_fixed_char(name, "..");
        std::fill(ext, ext + 3, ' ');
        return;
    }

    size_t dot = lfn.find('.');
    std::string base, extension;

    if (dot == std::string::npos) {
        base = lfn;
    } else {
        base = lfn.substr(0, dot);
        extension = lfn.substr(dot + 1);
    }

    copy_to_fixed_char(name, base);
    copy_to_fixed_char(ext, extension);
}

//-------------------------------------------------------------------
// Convert FAT filename parts to LFN. Returns empty string if invalid
//-------------------------------------------------------------------
std::string lfn_from_sfn(const char (&name)[8], const char (&ext)[3])
{
    std::string base(name, 8);
    std::string extension(ext, 3);

    base = trim_right_spaces(base);
    extension = trim_right_spaces(extension);

    if (base == ".." && extension.empty())
    return "..";

    std::string lfn = base;

    if (!extension.empty())
    lfn += "." + extension;

    if (lfn.empty())
        return {};

    if (!PathIsLFNFileSpecA(lfn.c_str()))
        return {};

    return lfn;
}

//-------------------------------------------------------------------
// Return slash normalized directory part of a path
//-------------------------------------------------------------------
inline std::string GetDirectoryPart(const std::string& input)
{
    std::filesystem::path p(input);
    std::string out = p.parent_path().string();
    FixDirSlashes(out);
    return out;
}

//-------------------------------------------------------------------
// Return filename part of a path
//-------------------------------------------------------------------
inline std::string GetFileNamePart(const std::string& input)
{
    std::filesystem::path p(input);
    return p.filename().string();
}

//-------------------------------------------------------------------
// Convert string containing possible FAT name and extension to an
// LFN string. Returns empty string if invalid LFN
//-------------------------------------------------------------------
std::string NormalizeInputToLFN(const std::string& s)
{
    if (s.empty()) return {};
    if (s.size() > 11) return {};
    if (s == "..") return "..";

    // LFN candidate
    if (s.find('.') != std::string::npos) {
        if (!PathIsLFNFileSpecA(s.c_str()))
            return {}; // invalid
        return s;
    }

    // SFN candidate
    char name[8];
    char ext[3];
    sfn_from_lfn(name,ext,s);
    return lfn_from_sfn(name,ext);
}

//-------------------------------------------------------------------
// Determine if path is a direcory
//-------------------------------------------------------------------
inline bool IsDirectory(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::is_directory(path, ec) && !ec;
}

//----------------------------------------------------------------------
// A file path may use 11 char FAT format which does not use a separater
// between name and extension. User is free to use standard dot format.
//    "FOODIR/FOO.DSK"     = FOODIR/FOO.DSK
//    "FOODIR/FOO     DSK" = FOODIR/FOO.DSK
//    "FOODIR/ALONGFOODSK" = FOODIR/ALONGFOO.DSK
//----------------------------------------------------------------------
std::string FixFATPath(const std::string& sdcpath)
{
    std::filesystem::path p(sdcpath);

    auto chop = [](std::string s) {
        size_t space = s.find(' ');
        if (space != std::string::npos) s.erase(space);
        return s;
    };

    std::string fname = p.filename().string();
    if (fname.length() == 11 && fname.find('.') == std::string::npos) {
        auto nam = chop(fname.substr(0,8));
        auto ext = chop(fname.substr(8,3));
        fname = ext.empty() ? nam : nam + "." + ext;
    }

    std::filesystem::path out = p.parent_path() / fname;

    DLOG_C("FixFATPath in %s out %s\n",sdcpath.c_str(),out.generic_string().c_str());
    return out.generic_string();
}

void FixFATPath(char* path, const char* sdcpath)
{
    std::string fixed = FixFATPath(std::string(sdcpath));
    strncpy(path, fixed.c_str(), MAX_PATH);
    path[MAX_PATH - 1] = '\0';
}

//======================================================================
// SDC specific funtions
//======================================================================

LRESULT CALLBACK SDC_Configure(HWND, UINT, WPARAM, LPARAM);
void LoadConfig();
bool SaveConfig(HWND);
void BuildCartridgeMenu();
void SelectCardBox();
void UpdateFlashItem(int);
void InitCardBox();
void InitFlashBoxes();
void FitEditTextPath(HWND, int, const char *);
void InitSDC();
void LoadRom(unsigned char);
void ParseStartup();
bool SearchFile(const char *);

//======================================================================
// SDC interface functions
//======================================================================
void SDCReadSector();
void SDCStreamImage();
void SDCWriteSector();
void SDCGetDriveInfo();
void SDCUpdateSD();
void SDCSetCurDir(const char *);
bool SDCInitiateDir(const char *);
void SDCRenameFile(const char *);
void SDCKillFile(const char *);
void SDCMakeDirectory(const char *);
void SDCGetMountedImageRec();
void SDCGetSectorCount();
void SDCGetDirectoryLeaf();
void SDCMountDisk(int,const char *,int);
void SDCMountNewDisk(int,const char *,int);
bool SDCMountNext(int);
unsigned char SDCRead(unsigned char port);
void SDCOpenFound(int,int);
void SDCOpenNew(int,const char *,int);
void SDCWrite(unsigned char,unsigned char);
void SDCControl();
void SDCCommand();
void SDCBlockReceive(unsigned char);
void SDCFlashControl(unsigned char);
void SDCFloppyCommand(unsigned char);
void SDCFloppyRestore();
void SDCFloppySeek();
void SDCFloppyReadDisk();
void SDCFloppyWriteDisk();
void SDCFloppyTrack(unsigned char);
void SDCFloppySector(unsigned char);
void SDCFloppyWriteData(unsigned char);
unsigned int SDCFloppyLSN(unsigned int,unsigned int,unsigned int);
unsigned char SDCFloppyStatus();
unsigned char SDCFloppyReadData();
unsigned char SDCPickReplyByte(unsigned char);
unsigned char SDCWriteFlashBank(unsigned short);
void SDCLoadReply(const void *, int);
bool SDCSeekSector(unsigned char,unsigned int);
bool SDCReadDrive(unsigned char,unsigned int);
bool SDCLoadNextDirPage();
std::string SDCGetFullPath(const std::string&);

//======================================================================

//=========================================================================
// Host directory handling
//=========================================================================

// Host Directory globals
static std::string gSDRoot {}; // SD card root directory
static std::string gCurDir {}; // Current directory relative to root

//-------------------------------------------------------------------
// HostFile contains a file name, it's size, and directory and readonly flags
// gFileList is a list of HostFiles found within a particular host directory
// using a search pattern. The pattern may include a directory specifier.
// Host file name, size, and attributes are kept in HostFile structs.
//-------------------------------------------------------------------

struct HostFile {
    std::string name;   // LFN 8.3 name
    DWORD size;         // < 4GB
    bool isDir;         // is a directory
    bool isRdOnly;      // is read only
    HostFile() {}
    // Construct a HostFile from an argument list
    HostFile(const char* cName, DWORD nSize, bool isDir, bool isRdOnly) :
        name(cName), size(nSize), isDir(isDir), isRdOnly(isRdOnly) {}
    // Construct a HostFile from a WIN32_FIND_DATAA record.
    HostFile(WIN32_FIND_DATAA fd) :
        name(fd.cFileName), size(fd.nFileSizeLow),
        isDir((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0),
        isRdOnly((fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0) {}
};

struct FileList {
    std::vector<HostFile> files;    // files
    std::string directory;          // directory files are in
    size_t cursor = 0;              // current file curspr
    bool nextload_flag = false;     // enable next disk loading
    // append a host file to the list
    void append(const HostFile& hf) { files.push_back(hf); }
};

static FileList gFileList;

void GetFileList(const std::string& pattern);
void SortFileList();

//-------------------------------------------------------------------
// Get list of files matching pattern starting from CurDir or SDRoot
//-------------------------------------------------------------------
void GetFileList(const std::string& pattern)
{

    DLOG_C("GetFileList %s\n",pattern.c_str());

    // Init search results
    gFileList = {};

    // None if pattern is empty
    if (pattern.empty()) return;

    // Set up search path.
    bool not_at_root = !gCurDir.empty();
    bool rel_pattern = pattern.front() != '/';

    std::string search_path = gSDRoot;
    if (rel_pattern) {
        search_path += "/";
        if (not_at_root)
            search_path += gCurDir + "/";
    }
    search_path += pattern;

    // A directory lookup if pattern name is "*" or "*.*";
    std::string fnpat = GetFileNamePart(pattern);
    bool dir_lookup = (fnpat == "*" || fnpat == "*.*");

    std::string search_dir = GetDirectoryPart(search_path);

    // Include ".." if dir_lookup and search_dir is not SDRoot
    if (dir_lookup && GetDirectoryPart(search_path) != gSDRoot) {
        gFileList.append({"..", 0, true, false});
    }

    // Add matching files to the list
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        gFileList = {};
        return;
    }
    do {
        if (strcmp(fd.cFileName,".") == 0) continue;    // exclude single dot file
        if (PathIsLFNFileSpecA(fd.cFileName)) continue; // exclude ugly or long filenames
        if (fd.nFileSizeHigh != 0) continue;            // exclude files > 4 GB
        gFileList.append({fd});
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);

    // Save directory
    gFileList.directory = search_dir;

    DLOG_C("GetFileList found %d\n",gFileList.files.size());
}

//-------------------------------------------------------------------
// SortFileList() may be needed when mounting a directory
//-------------------------------------------------------------------
void SortFileList()
{
    std::sort(
        gFileList.files.begin(),
        gFileList.files.end(),
        [] (const HostFile& a, const HostFile& b) {
            return a.name < b.name;
        }
    );
    gFileList.cursor = 0;
}

//=========================================================================
// FileRecord is 16 packed bytes file name, type, attrib, and size that can
// be passed directly via the SDC interface. gDirPage is a 256 byte block 
// containing up to 16 packed FileRecords.
//=========================================================================
#pragma pack(push, 1)
struct FileRecord
{
    char FR_name[8];
    char FR_type[3];
    char FR_attrib;
    char FR_hihi_size;
    char FR_lohi_size;
    char FR_hilo_size;
    char FR_lolo_size;
    FileRecord() noexcept {};
    // Construct FileRecord from a HostFile object.
    FileRecord(const HostFile& hf) noexcept { 
        memset(this, 0, sizeof(*this));
        // name and type
        sfn_from_lfn(FR_name, FR_type, hf.name);
        // Attributes
        FR_attrib = 0;
        if (hf.isDir)     FR_attrib |= 0x10;
        if (hf.isRdOnly)  FR_attrib |= 0x01;
        // Size -> 4 reversed endian bytes
        DWORD s = hf.size;
        FR_hihi_size = (s >> 24) & 0xFF;
        FR_lohi_size = (s >> 16) & 0xFF;
        FR_hilo_size = (s >> 8)  & 0xFF;
        FR_lolo_size = (s >> 0)  & 0xFF;
    }
};
static struct FileRecord gDirPage[16] = {};
#pragma pack(pop)

//=========================================================================
// CocoDisk contains info about mounted disk files 0 and 1 
//=========================================================================

enum DiskType {
    DTYPE_RAW = 0,
    DTYPE_DSK,
    DTYPE_JVC,
    DTYPE_VDK,
    DTYPE_SDF
};

// Mounted image data
struct CocoDisk {
    HANDLE hFile;               // file handle
    unsigned int size;          // number of bytes total
    unsigned int headersize;    // number of bytes in header
    DWORD sectorsize;           // number of bytes per sector
    DWORD tracksectors;         // number of sectors per side
    DiskType type;              // Disk image type (RAW,DSK,JVC,VDK,SDF)
    char doublesided;           // false:1 side, true:2 sides
    char name[MAX_PATH];        // name of file (8.3)
    char fullpath[MAX_PATH];    // full file path
    struct HostFile hf{"",0,false,false}; //name,size,isDir,isRdOnly
    struct FileRecord filerec;
    // Constructor
    CocoDisk() noexcept
        : hFile(INVALID_HANDLE_VALUE),
          size(0),
          headersize(0),
          sectorsize(256),
          tracksectors(18),
          type(DiskType::DTYPE_RAW),
          doublesided(1) {
        name[0] = '\0';
        fullpath[0] = '\0';
    };
};
CocoDisk gCocoDisk[2] {};

//----------------------------------------------------------------------
//  Host File search.
//----------------------------------------------------------------------

bool SearchFile(const char* pattern)
{
    std::string s = pattern;
    GetFileList(s);
    DLOG_C("SearchFile found %d pat %s\n",gFileList.files.size(),s.c_str());

    // Update menu to show next disk status
    BuildCartridgeMenu();

    return (gFileList.files.size() > 0);
}

//======================================================================
// Static Globals
//======================================================================

static HINSTANCE gModuleInstance; // Dll handle
static int idle_ctr = 0; // Idle Status counter

// Callback pointers
static void* gCallbackContext = nullptr;
static PakAssertInteruptHostCallback AssertIntCallback = nullptr;
static PakAppendCartridgeMenuHostCallback CartMenuCallback = nullptr;
static char IniFile[MAX_PATH] = {};  // Vcc ini file name

// Data used elsewhere
static HWND gVccWindow = nullptr;
static HWND hConfigureDlg = nullptr;
static int ClockEnable = 0;
static char SDC_Status[16] = {};
static char PakRom[0x4000] = {};
static unsigned char CurrentBank = 0xff;
static unsigned char EnableBankWrite = 0;
static unsigned char BankWriteState = 0;

//======================================================================
// DLL interface
//======================================================================

//----------------------------------------------------------------------
// Functions referenced by DLL interface
//----------------------------------------------------------------------

void LoadConfig();
void BuildCartridgeMenu();
//void CloseDrive(int);
void UnloadDisk(int);
void SDCWrite(unsigned char,unsigned char);
unsigned char SDCRead(unsigned char port);
LRESULT CALLBACK SDC_Configure(HWND, UINT, WPARAM, LPARAM);
bool SDCMountNext(int);
void InitSDC();
unsigned char SDCWriteFlashBank(unsigned short);

//----------------------------------------------------------------------
// DLL exports
//----------------------------------------------------------------------
extern "C"
{
    // PakInitialize gets called first, sets up dynamic menues and captures callbacks
    __declspec(dllexport) void PakInitialize(
        void* const callback_context,
        const char* const configuration_path,
        HWND hVccWnd,
        const cpak_callbacks* const callbacks)
    {
        gVccWindow = hVccWnd;
        DLOG_C("SDC %p %p %p %p %p\n",*callbacks);
        gCallbackContext = callback_context;
        CartMenuCallback = callbacks->add_menu_item;
        AssertIntCallback = callbacks->assert_interrupt;
        strcpy(IniFile, configuration_path);

        LoadConfig();
        BuildCartridgeMenu();
    }

    __declspec(dllexport) const char* PakGetName()
    {
        static char string_buffer[MAX_LOADSTRING];
        LoadString(gModuleInstance, IDS_MODULE_NAME, string_buffer, MAX_LOADSTRING);
        return string_buffer;
    }

    __declspec(dllexport) const char* PakGetCatalogId()
    {
        static char string_buffer[MAX_LOADSTRING];
        LoadString(gModuleInstance, IDS_CATNUMBER, string_buffer, MAX_LOADSTRING);
        return string_buffer;
    }

    __declspec(dllexport) const char* PakGetDescription()
    {
        static char string_buffer[MAX_LOADSTRING];
        LoadString(gModuleInstance, IDS_DESCRIPTION, string_buffer, MAX_LOADSTRING);
        return string_buffer;
    }

    // Clean up must also be done on DLL_UNLOAD incase VCC is closed!
    __declspec(dllexport) void PakTerminate()
    {
        CloseCartDialog(hConfigureDlg);
        hConfigureDlg = nullptr;
        UnloadDisk(0);
        UnloadDisk(1);
        //CloseDrive(0);
        //CloseDrive(1);
    }

    // Write to port
    __declspec(dllexport) void PakWritePort(unsigned char Port,unsigned char Data)
    {
        SDCWrite(Data,Port);
        return;
    }

    // Read from port
    __declspec(dllexport) unsigned char PakReadPort(unsigned char Port)
    {
        if (ClockEnable && ((Port==0x78) | (Port==0x79) | (Port==0x7C))) {
            return cloud9_rtc.read_port(Port);
        } else if ((Port > 0x3F) & (Port < 0x60)) {
            return SDCRead(Port);
        } else {
            return 0;
        }
    }

    // Reset module
    __declspec(dllexport) void PakReset()
    {
        DLOG_C("PakReset\n");
        InitSDC();
    }

    //  Dll export run config dialog
    __declspec(dllexport) void PakMenuItemClicked(unsigned char MenuID)
    {
        switch (MenuID)
        {
        case 10:
            if (hConfigureDlg == nullptr)  // Only create dialog once
                hConfigureDlg = CreateDialog(gModuleInstance, (LPCTSTR) IDD_CONFIG,
                         GetActiveWindow(), (DLGPROC) SDC_Configure);
            ShowWindow(hConfigureDlg,1);
            break;
        case 11:
            SDCMountNext (0);
            break;
        }
        BuildCartridgeMenu();
        return;
    }

    // Return SDC status.
    __declspec(dllexport) void PakGetStatus(char* text_buffer, size_t buffer_size)
    {
        if (idle_ctr < 100) {
            idle_ctr++;
        } else {
            idle_ctr = 0;
            snprintf(SDC_Status,16,"SDC:%d idle",CurrentBank);
        }
        strncpy(text_buffer,SDC_Status,buffer_size);
    }

    // Return a byte from the current PAK ROM
    __declspec(dllexport) unsigned char PakReadMemoryByte(unsigned short adr)
    {
        adr &= 0x3FFF;
        if (EnableBankWrite) {
            return SDCWriteFlashBank(adr);
        } else {
            BankWriteState = 0;  // Any read resets write state
            return(PakRom[adr]);
        }
    }
}

//----------------------------------------------------------------------
// DLL main
//----------------------------------------------------------------------

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID rsvd)
{
    if (reason == DLL_PROCESS_ATTACH) {
        gModuleInstance = hinst;
    } else if (reason == DLL_PROCESS_DETACH) {
        PakTerminate();
    }
    return TRUE;
}

//======================================================================
// Unsorted Globals
//======================================================================

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
static Interface IF = {};

//static std::string gSeaDir {}; // Last directory searched

// Flash banks
static char FlashFile[8][MAX_PATH];
static FILE *h_RomFile = nullptr;
static unsigned char StartupBank = 0;
static unsigned char BankWriteNum = 0;
static unsigned char BankDirty = 0;
static unsigned char BankData = 0;
// Following are in DLL section
//static unsigned char CurrentBank = 0xff;
//static unsigned char EnableBankWrite = 0;
//static unsigned char BankWriteState = 0;

// Clock enable IDC_CLOCK
//static int ClockEnable;

// Windows file lookup handle and data
//static HANDLE hFind = INVALID_HANDLE_VALUE;
//static WIN32_FIND_DATAA dFound;

// Window handles
static HWND hSDCardBox = nullptr;
static HWND hStartupBank = nullptr;
// Following two are in DLL section
//static HWND hConfigureDlg = nullptr;
//static HWND gVccWindow = nullptr;

// Streaming control
static int streaming;
static unsigned char stream_cmdcode;
static unsigned int stream_lsn;

// Status for VCC status line
//static char SDC_Status[16] = {};

// Floppy I/O
static char FlopDrive = 0;
static char FlopData = 0;
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


//=====================================================================
// User interface
//======================================================================

//-------------------------------------------------------------
// Generate menu for configuring the SDC
//-------------------------------------------------------------
void BuildCartridgeMenu()
{
    CartMenuCallback(gCallbackContext, "", MID_BEGIN, MIT_Head);
    CartMenuCallback(gCallbackContext, "", MID_ENTRY, MIT_Seperator);
    CartMenuCallback(gCallbackContext, "SDC Drive 0",MID_ENTRY,MIT_Head);
    char tmp[64]={};
    if (strcmp(gCocoDisk[0].name,"") == 0) {
        strcpy(tmp,"empty");
    } else {
        strcpy(tmp,gCocoDisk[0].name);
        if (gFileList.nextload_flag) {
            strcat(tmp," (load next)");
        } else {
            strcat(tmp," (no next)");
        }
    }
    CartMenuCallback(gCallbackContext, tmp, ControlId(11),MIT_Slave);
    CartMenuCallback(gCallbackContext, "SDC Config", ControlId(10), MIT_StandAlone);
    CartMenuCallback(gCallbackContext, "", MID_FINISH, MIT_Head);
}

//------------------------------------------------------------
// Configure the SDC
//------------------------------------------------------------
LRESULT CALLBACK
SDC_Configure(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
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
        InitFlashBoxes();
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
                if (*tmp != '\0') {
                    gCurDir = tmp;
                    FixDirSlashes(gCurDir);
                    //strncpy(SDCard,tmp,MAX_PATH);
                }
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
void LoadConfig()
{
    char tmp[MAX_PATH];

    GetPrivateProfileString
        ("DefaultPaths", "MPIPath", "", MPIPath, MAX_PATH, IniFile);

    GetPrivateProfileString
        ("SDC", "SDCardPath", "", tmp, MAX_PATH, IniFile);

    gSDRoot = tmp;
    FixDirSlashes(gSDRoot);

    DLOG_C("LoadConfig MPIPath %s\n",MPIPath);
    DLOG_C("LoadConfig gSDRoot %s\n",gSDRoot);

    if (!IsDirectory(gSDRoot)) {
        MessageBox (gVccWindow,"Invalid SDCard Path in VCC init","Error",0);
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
    if (!IsDirectory(gSDRoot)) {
        MessageBox(gVccWindow,"Invalid SDCard Path\n","Error",0);
        return false;
    }
    WritePrivateProfileString("SDC","SDCardPath",gSDRoot.c_str(),IniFile);

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

//------------------------------------------------------------
// Fit path in edit text box (Box must be ES_READONLY)
//------------------------------------------------------------
void FitEditTextPath(HWND hDlg, int ID, const char * path) {
    HDC c;
    HWND h;
    RECT r;
    char p[MAX_PATH];
    if ((c = GetDC(hDlg)) == NULL) return;
    if ((h = GetDlgItem(hDlg, ID)) == NULL) return;
    GetClientRect(h, &r);
    strncpy(p, path, MAX_PATH);
    PathCompactPath(c, p, r.right);
    FixDirSlashes(p);
    SetWindowText(h, p);
    ReleaseDC(hDlg, c);
}

//------------------------------------------------------------
// Init flash box
//------------------------------------------------------------
void InitFlashBoxes()
{
    for (int idx=0; idx<8; idx++) {
        FitEditTextPath(hConfigureDlg,EDBOXES[idx],FlashFile[idx]);
        HWND h = GetDlgItem(hConfigureDlg,UPDBTNS[idx]);
        if (*FlashFile[idx] == '\0') {
            SetWindowText(h,">");
        } else {
            SetWindowText(h,"X");
        }
    }
}

//------------------------------------------------------------
// Init SD card box
//------------------------------------------------------------

void InitCardBox()
{
    hSDCardBox = GetDlgItem(hConfigureDlg,ID_SD_BOX);
    SendMessage(hSDCardBox, WM_SETTEXT, 0, (LPARAM) gSDRoot.c_str());
}

//------------------------------------------------------------
// Modify and or Update flash box item
//------------------------------------------------------------

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
        dlg.setInitialDir(MPIPath);   // FIXME should be SDC rom path
        if (dlg.show(0,hConfigureDlg)) {
            dlg.getupath(filename,MAX_PATH); // cvt to unix style
            strncpy(FlashFile[index],filename,MAX_PATH);
        }
    }
    InitFlashBoxes();

    // Save path to ini file
    char txt[32];
    sprintf(txt,"FlashFile_%d",index);
    WritePrivateProfileString("SDC",txt,FlashFile[index],IniFile);
}

//------------------------------------------------------------
// Dialog to select SD card path in user home directory
//------------------------------------------------------------

// TODO: Replace Win32 browse dialog with the modern IFileDialog API (Vista+)
void SelectCardBox()
{
    namespace fs = std::filesystem;

    // Prepare browse dialog
    BROWSEINFO bi = {};
    bi.hwndOwner = GetActiveWindow();
    bi.lpszTitle = "Set the SD card path";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NONEWFOLDERBUTTON;

    // Set initial folder to user profile
    LPITEMIDLIST pidlRoot = nullptr;
    if (SUCCEEDED(SHGetSpecialFolderLocation(nullptr, CSIDL_PROFILE, &pidlRoot)))
        bi.pidlRoot = pidlRoot;

    // Show dialog
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    // Free root PIDL if allocated
    if (pidlRoot)
        CoTaskMemFree(pidlRoot);

    if (pidl)
    {
        char tmp[MAX_PATH] = {};
        if (SHGetPathFromIDList(pidl, tmp))
        {
            gSDRoot = tmp;
            FixDirSlashes(gSDRoot);
            SendMessage(hSDCardBox, WM_SETTEXT, 0, (LPARAM)gSDRoot.c_str());
        }

        CoTaskMemFree(pidl);
    }
}

//=====================================================================
// SDC Simulation
//======================================================================

//----------------------------------------------------------------------
// Init the controller. This gets called by PakReset
//----------------------------------------------------------------------
void InitSDC()
{

    DLOG_C("\nInitSDC\n");
#ifdef USE_LOGGING
    MoveWindow(GetConsoleWindow(),0,0,300,800,TRUE);
#endif

    // Make sure drives are unloaded
    SDCMountDisk (0,"",0);
    SDCMountDisk (1,"",0);

    // Load SDC settings
    LoadConfig();
    LoadRom(StartupBank);

    SDCSetCurDir(""); // May be changed by ParseStartup()

    gCocoDisk[0] = {};
    gCocoDisk[1] = {};

    gFileList = {};

    // Process the startup config file
    ParseStartup();

    // init the interface
    IF = {};

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

    DLOG_C("LoadRom load flash bank %d\n",bank);

    // Skip if bank is already active
    if (bank == CurrentBank) return;

    // Make sure flash file is closed
    if (h_RomFile) {
        fclose(h_RomFile);
        h_RomFile = nullptr;
    }

    // If bank contents have been changed write to the flash file
    if (BankDirty) {
        RomFile = FlashFile[CurrentBank];
        DLOG_C("LoadRom switching out dirty bank %d %s\n",CurrentBank,RomFile);
        h_RomFile = fopen(RomFile,"wb");
        if (!h_RomFile) {
            MessageBox (gVccWindow,"Can not write Rom file","SDC Rom Save Failed",0);
        } else {
            ctr = 0;
            p_rom = PakRom;
            while (ctr++ < 0x4000) fputc(*p_rom++, h_RomFile);
            fclose(h_RomFile);
            h_RomFile = nullptr;
        }
        BankDirty = 0;
    }

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

    // Open romfile for write if not startup bank
    if (CurrentBank != StartupBank) 
        h_RomFile = fopen(RomFile,"wb");

    if (CurrentBank == StartupBank || h_RomFile == nullptr)
        h_RomFile = fopen(RomFile,"rb");

    if (h_RomFile == nullptr) {
        std::string msg="Check Rom Path and SDC Config\n"+std::string(RomFile);
        MessageBox (gVccWindow,msg.c_str(),"SDC Startup Rom Load Failed",0);
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
void ParseStartup()
{
    namespace fs = std::filesystem;
    fs::path sd = fs::path(gSDRoot);

    if (!fs::is_directory(sd)) {
        DLOG_C("ParseStartup SDCard path invalid\n");
        return;
    }

    fs::path cfg = sd / "startup.cfg";
    FILE* su = std::fopen(cfg.string().c_str(), "r");
    if (su == nullptr) {
        DLOG_C("ParseStartup file not found, %s\n", cfg.string().c_str());
        return;
    }

    // Strict single char followed by '=' then path
    char buf[MAX_PATH];
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
            SDCMountDisk(0,&buf[2],0);
            break;
        case '1':
            SDCMountDisk(1,&buf[2],0);
            break;
        case 'D':
            SDCSetCurDir(&buf[2]);
            break;
        }
    }
    fclose(su);
}


//=====================================================================
// SDC Interface functions
//=====================================================================

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
            if (IF.sdclatch) IF = {};
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
                SDCBlockReceive(data);
            else
                IF.param2 = data;
            break;
        // Command param #3 or block data receive
        case 0x4B:
            if (IF.bufcnt > 0)
                SDCBlockReceive(data);
            else
                IF.param3 = data;
            break;
        // Unhandled
        default:
            DLOG_C("SDCWrite L %02x %02x\n",port,data);
            break;
        }
    } else {
        switch (port) {
        case 0x40:
            // Command latch and floppy drive select
            // Mask out halt, density, precomp, and motor
            switch (data & 0x43) { // 0b01000111
            case 0:
                IF.sdclatch = false;
                break;
            case 0x43:
                IF.sdclatch = true;
                break;
            case 0b00000001:
                FlopDrive = 0;
                break;
            case 0b00000010:
                FlopDrive = 1;
                break;
            case 0b00000100:
                DLOG_C("SDC floppy 2 not supported\n");
                break;
            case 0b01000000:
                DLOG_C("SDC floppy 3 not supported\n");
                break;
            default:
                DLOG_C("SDC invalid select %d\n",data);
                break;
            }
            break;
        // Flash Data
        case 0x42:
            BankData = data;
            break;
        // Flash Control
        case 0x43:
            SDCFlashControl(data);
            break;
        // floppy command
        case 0x48:
            SDCFloppyCommand(data);
            break;
        // floppy set track
        case 0x49:
            SDCFloppyTrack(data);
            break;
        // floppy set sector
        case 0x4A:
            SDCFloppySector(data);
            break;
        // floppy write data
        case 0x4B:
            SDCFloppyWriteData(data);
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
                rpy = SDCPickReplyByte(port);
            } else {
                rpy = IF.reply2;
            }
            break;
        // Reply data 3 or block reply
        case 0x4B:
            if (IF.bufcnt > 0) {
                rpy = SDCPickReplyByte(port);
            } else {
                rpy = IF.reply3;
            }
            break;
        default:
            DLOG_C("SDCRead L %02x\n",port);
            rpy = 0;
            break;
        }
    } else {
        switch (port) {
        case 0x40:
            //Nitros9 driver reads this
            DLOG_C("SDCRead floppy port 40?\n");
            rpy = 0;
            break;
        // Flash control read is used by SDCDOS to detect the SDC
        case 0x43:
            rpy = CurrentBank | (BankData & 0xF8);
            break;
        // Floppy read status
        case 0x48:
            rpy = SDCFloppyStatus();
            break;
        // Current Track
        case 0x49:
            //Nitros9 driver reads this every sector
            //DLOG_C("SDCRead floppy track?\n");
            rpy = 0;
            break;
        // Current Sector
        case 0x4A:
            //Nitros9 driver reads this every sector
            //DLOG_C("SDCRead floppy sector?\n");
            rpy = 0;
            break;
        // Floppy read data
        case 0x4B:
            rpy = SDCFloppyReadData();
            break;
        default:
            DLOG_C("SDCRead U %02x\n",port);
            rpy = 0;
            break;
        }
    }
    return rpy;
}

//----------------------------------------------------------------------
//  Dispatch SDC commands
//----------------------------------------------------------------------
void SDCCommand()
{
    switch (IF.cmdcode & 0xF0) {
    // Read sector
    case 0x80:
        SDCReadSector();
        break;
    // Stream 512 byte sectors
    case 0x90:
        SDCStreamImage();
        break;
    // Get drive info
    case 0xC0:
        SDCGetDriveInfo();
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
// Floppy I/O
//----------------------------------------------------------------------

void SDCFloppyCommand(unsigned char data)
{
    unsigned char cmd = data >> 4;
    switch (cmd) {
    case 0:    //RESTORE
        SDCFloppyRestore();
        break;
    case 1:    //SEEK
        SDCFloppySeek();
        break;
    //case 2:  //STEP
    //case 3:  //STEPUPD
    //case 4:  //STEPIN
    //case 5:  //STEPINUPD
    //case 6:  //STEFOUT
    //case 7:  //STEPOUTUPD
    case 8:    //READSECTOR
    case 9:    //READSECTORM
        SDCFloppyReadDisk();
        break;
    case 10:   //WRITESECTOR
    case 11:   //WRITESECTORM
        SDCFloppyWriteDisk();
        break;
    case 12: //READADDRESS
        //Nitros9 driver does this
        DLOG_C("SDCFloppyCommand read address?\n");
        break;
    case 13:   //FORCEINTERUPT
        //Nitros9 driver does this
        DLOG_C("SDCFloppyCommand force interrupt?\n");
        break;
    //case 14: //READTRACK
    //case 15: //WRITETRACK
    default:
        DLOG_C("SDCFloppyCommand %d not implemented\n",cmd);
        break;
    }
}

//----------------------------------------------------------------------
// Get drive information
//----------------------------------------------------------------------
void SDCGetDriveInfo()
{
    int drive = IF.cmdcode & 1;
    switch (IF.param1) {
    case 0x49:
        // 'I' - return drive information in block
        SDCGetMountedImageRec();
        break;
    case 0x43:
        // 'C' Return current directory leaf in block
        SDCGetDirectoryLeaf();
        break;
    case 0x51:
        // 'Q' Return the size of disk image in p1,p2,p3
        SDCGetSectorCount();
        break;
    case 0x3E:
        // '>' Get directory page
        SDCLoadNextDirPage();
        IF.reply_mode=0;
        SDCLoadReply(gDirPage,256);
        break;
    case 0x2B:
        // '+' Mount next next disk in set.
        SDCMountNext(drive);
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
void SDCUpdateSD()
{
    switch (IF.blkbuf[0]) {
    case 0x4D: //M
        SDCMountDisk(IF.cmdcode&1,&IF.blkbuf[2],0);
        break;
    case 0x6D: //m
        SDCMountDisk(IF.cmdcode&1,&IF.blkbuf[2],1);
        break;
    case 0x4E: //N
        SDCMountNewDisk(IF.cmdcode&1,&IF.blkbuf[2],0);
        break;
    case 0x6E: //n
        SDCMountNewDisk(IF.cmdcode&1,&IF.blkbuf[2],1);
        break;
    case 0x44: //D
        SDCSetCurDir(&IF.blkbuf[2]);
        break;
    case 0x4C: //L
        SDCInitiateDir(&IF.blkbuf[2]);
        break;
    case 0x4B: //K
        SDCMakeDirectory(&IF.blkbuf[2]);
        break;
    case 0x52: //R
        SDCRenameFile(&IF.blkbuf[2]);
        break;
    case 0x58: //X
        SDCKillFile(&IF.blkbuf[2]);
        break;
    default:
        DLOG_C("SDCUpdateSD %02x not Supported\n",IF.blkbuf[0]);
        IF.status = STA_FAIL;
        break;
    }
}

//-------------------------------------------------------------------
// Load a DirPage from FileList. Called until list is exhausted
//-------------------------------------------------------------------
bool SDCLoadNextDirPage()
{
    DLOG_C("SDCLoadNextDirPage cur:%d siz:%d\n",
            gFileList.cursor,gFileList.files.size());

    std::memset(gDirPage, 0, sizeof gDirPage);

    if (gFileList.cursor >= gFileList.files.size()) {
        DLOG_C("SDCLoadNextDirPage no files left\n");
        return false;
    }

    size_t count = 0;
    while (count < 16 && gFileList.cursor < gFileList.files.size()) {
        gDirPage[count++] = FileRecord(gFileList.files[gFileList.cursor]);
        ++gFileList.cursor;
    }

    // gFileList.cursor not valid for next file loading
    gFileList.nextload_flag = false;

    return true;
}

//----------------------------------------------------------------------
// floppy restore
//----------------------------------------------------------------------
void SDCFloppyRestore()
{
    DLOG_C("FloppyRestore\n");
    FlopTrack = 0;
    FlopSector = 0;
    FlopStatus = FLP_NORMAL;
    FlopWrCnt = 0;
    FlopRdCnt = 0;
    AssertIntCallback(gCallbackContext, INT_NMI, IS_NMI);
}

//----------------------------------------------------------------------
// floppy seek.  No attempt is made to simulate seek times here.
//----------------------------------------------------------------------
void SDCFloppySeek()
{
    DLOG_C("FloppySeek %d %d\n",FlopTrack,FlopData);
    FlopTrack = FlopData;
}

//----------------------------------------------------------------------
// convert side (drive), track, sector, to LSN.  The floppy controller
// uses CHS addressing while hard drives, and the SDC, use LBA addressing.
// FIXME:
//  Nine sector tracks, (512 byte sectors)
//  disk type: gCocoDisk[drive].type
//----------------------------------------------------------------------
unsigned int SDCFloppyLSN(
        unsigned int drive,   // 0 or 1
        unsigned int track,   // 0 to num tracks
        unsigned int sector   // 1 to 18
    )
{
    return track * 18 + sector - 1;
}

//----------------------------------------------------------------------
// floppy read sector
//----------------------------------------------------------------------
void SDCFloppyReadDisk()
{
    auto lsn = SDCFloppyLSN(FlopDrive,FlopTrack,FlopSector);
    //DLOG_C("FloppyReadDisk %d %d\n",FlopDrive,lsn);

    snprintf(SDC_Status,16,"SDC:%d Rd %d,%d",CurrentBank,FlopDrive,lsn);
    if (SDCSeekSector(FlopDrive,lsn)) {
        if (ReadFile(gCocoDisk[FlopDrive].hFile,FlopRdBuf,256,&FlopRdCnt,nullptr)) {
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

//----------------------------------------------------------------------
// floppy write sector
//----------------------------------------------------------------------
void SDCFloppyWriteDisk()
{
    // write not implemented
    int lsn = FlopTrack * 18 + FlopSector - 1;
    DLOG_C("FloppyWriteDisk %d %d not implmented\n",FlopDrive,lsn);
    FlopStatus = FLP_READONLY;
}

//----------------------------------------------------------------------
// floppy set track
//----------------------------------------------------------------------
void SDCFloppyTrack(unsigned char data)
{
    FlopTrack = data;
}

//----------------------------------------------------------------------
// floppy set sector
//----------------------------------------------------------------------
void SDCFloppySector(unsigned char data)
{
    FlopSector = data;  // Sector num in track (1-18)
    //int lsn = FlopTrack * 18 + FlopSector - 1;
    //DLOG_C("FloppySector %d lsn %d\n",FlopSector,lsn);
    FlopStatus = FLP_NORMAL;
}

//----------------------------------------------------------------------
// floppy write data
//----------------------------------------------------------------------
void SDCFloppyWriteData(unsigned char data)
{
    //DLOG_C("FloppyWriteData %d\n",data);
    if (FlopWrCnt<256)  {
        FlopWrCnt++;
        FlopWrBuf[FlopWrCnt] = data;
    } else {
        FlopStatus = FLP_NORMAL;
        if ((FlopStatus &= FLP_DATAREQ) != 0)
            AssertIntCallback(gCallbackContext, INT_NMI, IS_NMI);
    }
    FlopData = data;
}

//----------------------------------------------------------------------
// floppy get status
//----------------------------------------------------------------------
unsigned char SDCFloppyStatus()
{
    //DLOG_C("FloppyStatus %02x\n",FlopStatus);
    return FlopStatus;
}

//----------------------------------------------------------------------
// floppy read data
//----------------------------------------------------------------------
unsigned char SDCFloppyReadData()
{
    unsigned char rpy;
    if (FlopRdCnt>0)  {
        rpy = FlopRdBuf[256-FlopRdCnt];
        FlopRdCnt--;
    } else {
        if ((FlopStatus &= FLP_DATAREQ) != 0)
            AssertIntCallback(gCallbackContext, INT_NMI, IS_NMI);
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
unsigned char SDCPickReplyByte(unsigned char port)
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
    if ((IF.bufcnt < 1) && streaming) SDCStreamImage();

    return rpy;
}

//----------------------------------------------------------------------
// Receive block data
//----------------------------------------------------------------------
void SDCBlockReceive(unsigned char byte)
{
    if (IF.bufcnt > 0) {
        IF.bufcnt--;
        *IF.bufptr++ = byte;
    }

    // Done receiving block
    if (IF.bufcnt < 1) {
        switch (IF.cmdcode & 0xF0) {
        case 0xA0:
            SDCWriteSector();
            break;
        case 0xE0:
            SDCUpdateSD();
            break;
        default:
            DLOG_C("SDCBlockReceive invalid cmd %d\n",IF.cmdcode);
            IF.status = STA_FAIL;
            break;
        }
    }
}

//----------------------------------------------------------------------
// Reply with directory leaf. Reply is 32 byte directory record. The first
// 8 bytes are the name of the leaf, the next 3 are blanks, and the
// remainder is undefined ("Private" is SDC docs)
// SDCEXP uses this with the set directory '..' command to learn the full
// path when restore last session is active. The full path is saved in
// SDCX.CFG for the next session.
//----------------------------------------------------------------------
void SDCGetDirectoryLeaf()
{
    namespace fs = std::filesystem;

    char leaf[32] = {};

    if (!gCurDir.empty()) {
        std::string leafName = fs::path(gCurDir).filename().string();
        memset(leaf, ' ', 11);
        size_t copyLen = std::min<size_t>(8,leafName.size());
        memcpy(leaf, leafName.data(), copyLen);
    } else {
        IF.reply_status = STA_FAIL | STA_NOTFOUND;
    }
    DLOG_C("SDCGetDirectoryLeaf CurDir '%s' leaf '%s' \n", gCurDir.c_str(),leaf);

    IF.reply_mode = 0;
    SDCLoadReply(leaf, sizeof(leaf));
}

//----------------------------------------------------------------------
// Flash control
//----------------------------------------------------------------------
void SDCFlashControl(unsigned char data)
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
unsigned char SDCWriteFlashBank(unsigned short adr)
{
    DLOG_C("SDCWriteFlashBank %d %d %04X %02X\n",
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
// Read logical sector
// cmdcode:
//    b0 drive number
//    b1 single sided flag
//    b2 eight bit transfer flag
//----------------------------------------------------------------------
void SDCReadSector()
{
    unsigned int lsn = (IF.param1 << 16) + (IF.param2 << 8) + IF.param3;

    //DLOG_C("R%d\n",lsn);

    IF.reply_mode = ((IF.cmdcode & 4) == 0) ? 0 : 1; // words : bytes
    if (!SDCReadDrive(IF.cmdcode,lsn))
        IF.status = STA_FAIL | STA_READERROR;
    else
        IF.status = STA_NORMAL;
}

//----------------------------------------------------------------------
// Stream image data
//----------------------------------------------------------------------
void SDCStreamImage()
{
    // If already streaming continue
    if (streaming) {
        stream_lsn++;
    // Else start streaming
    } else {
        stream_cmdcode = IF.cmdcode;
        IF.reply_mode = ((IF.cmdcode & 4) == 0) ? 0 : 1;
        stream_lsn = (IF.param1 << 16) + (IF.param2 << 8) + IF.param3;
        DLOG_C("SDCStreamImage lsn %d\n",stream_lsn);
    }

    // For now can only stream 512 byte sectors
    int drive = stream_cmdcode & 1;
    gCocoDisk[drive].sectorsize = 512;
    gCocoDisk[drive].tracksectors = 9;

    if (stream_lsn > (gCocoDisk[drive].size/gCocoDisk[drive].sectorsize)) {
        DLOG_C("SDCStreamImage done\n");
        streaming = 0;
        return;
    }

    if (!SDCReadDrive(stream_cmdcode,stream_lsn)) {
        DLOG_C("SDCStreamImage read error %s\n",LastErrorTxt());
        IF.status = STA_FAIL;
        streaming = 0;
        return;
    }
    streaming = 1;
}

//----------------------------------------------------------------------
// Write logical sector
//----------------------------------------------------------------------
void SDCWriteSector()
{
    DWORD cnt = 0;
    int drive = IF.cmdcode & 1;
    unsigned int lsn = (IF.param1 << 16) + (IF.param2 << 8) + IF.param3;
    snprintf(SDC_Status,16,"SDC:%d Wr %d,%d",CurrentBank,drive,lsn);

    if (gCocoDisk[drive].hFile == nullptr) {
        IF.status = STA_FAIL;
        return;
    }

    if (!SDCSeekSector(drive,lsn)) {
        IF.status = STA_FAIL;
        return;
    }
    if (!WriteFile(gCocoDisk[drive].hFile,IF.blkbuf,
                   gCocoDisk[drive].sectorsize,&cnt,nullptr)) {
        DLOG_C("SDCWriteSector %d %s\n",drive,LastErrorTxt());
        IF.status = STA_FAIL;
        return;
    }
    if (cnt != gCocoDisk[drive].sectorsize) {
        DLOG_C("SDCWriteSector %d short write\n",drive);
        IF.status = STA_FAIL;
        return;
    }
    IF.status = 0;
    return;
}

//----------------------------------------------------------------------
// Return sector count for mounted disk image
//----------------------------------------------------------------------
void  SDCGetSectorCount() {

    int drive = IF.cmdcode & 1;
    unsigned int numsec = gCocoDisk[drive].size/gCocoDisk[drive].sectorsize;
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
void SDCGetMountedImageRec()
{
    int drive = IF.cmdcode & 1;
    //DLOG_C("SDCGetMountedImageRec %d %s\n",drive,gCocoDisk[drive].fullpath);
    if (strlen(gCocoDisk[drive].fullpath) == 0) {
        IF.status = STA_FAIL;
    } else {
        IF.reply_mode = 0;
        SDCLoadReply(&gCocoDisk[drive].filerec,sizeof(FileRecord));
    }
}

//----------------------------------------------------------------------
// $DO Abort stream and mount disk in a set of disks.
// IF.param1  0: Next disk 1-9: specific disk.
// IF.param2 b0: Blink Enable
//----------------------------------------------------------------------
void SDCControl()
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
// Seek sector in drive image
//  cmdcode:
//    b0 drive number
//    b1 single sided LSN flag
//    b2 eight bit transfer flag
//
//----------------------------------------------------------------------
bool SDCSeekSector(unsigned char cmdcode, unsigned int lsn)
{
    int drive = cmdcode & 1; // Drive number 0 or 1
    int sside = cmdcode & 2; // Single sided LSN flag

    int trk = lsn / gCocoDisk[drive].tracksectors;
    int sec = lsn % gCocoDisk[drive].tracksectors;

    // The single sided LSN flag tells the controller that the lsn
    // assumes the disk image is a single-sided floppy disk. If the
    // disk is actually double-sided the LSN must be adjusted.
    if (sside && gCocoDisk[drive].doublesided) {
        DLOG_C("SDCSeekSector sside %d %d\n",drive,lsn);
        lsn = 2 * gCocoDisk[drive].tracksectors * trk + sec;
    }

    // Allow seek to expand a writable file to a resonable limit
    if (lsn > MAX_DSK_SECTORS) {
        DLOG_C("SDCSeekSector exceed max image %d %d\n",drive,lsn);
        return false;
    }

    // Seek to logical sector on drive.
    LARGE_INTEGER pos{0};
    pos.QuadPart = lsn * gCocoDisk[drive].sectorsize + gCocoDisk[drive].headersize;

    if (!SetFilePointerEx(gCocoDisk[drive].hFile,pos,nullptr,FILE_BEGIN)) {
        DLOG_C("SDCSeekSector error %s\n",LastErrorTxt());
        return false;
    }
    return true;
}

//----------------------------------------------------------------------
// Read a sector from drive image and load reply
//----------------------------------------------------------------------
bool SDCReadDrive(unsigned char cmdcode, unsigned int lsn)
{
    char buf[520];
    DWORD cnt = 0;
    int drive = cmdcode & 1;
    if (gCocoDisk[drive].hFile == nullptr) {
        DLOG_C("SDCReadDrive %d not open\n");
        return false;
    }

    if (!SDCSeekSector(cmdcode,lsn)) {
        return false;
    }

    if (!ReadFile(gCocoDisk[drive].hFile,buf,gCocoDisk[drive].sectorsize,&cnt,nullptr)) {
        DLOG_C("SDCReadDrive %d %s\n",drive,LastErrorTxt());
        return false;
    }

    if (cnt != gCocoDisk[drive].sectorsize) {
        DLOG_C("SDCReadDrive %d short read\n",drive);
        return false;
    }

    snprintf(SDC_Status,16,"SDC:%d Rd %d,%d",CurrentBank,drive,lsn);
    SDCLoadReply(buf,cnt);
    return true;
}

//----------------------------------------------------------------------
// Load reply. Count is bytes, 512 max.
//----------------------------------------------------------------------
void SDCLoadReply(const void *data, int count)
{
    if ((count < 2) | (count > 512)) {
        DLOG_C("SDCLoadReply bad count %d\n",count);
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
// Create and mount a new disk image
//  IF.param1 == 0 create 161280 byte JVC file
//  IF.param1 SDF image number of cylinders
//  IF.param2 SDC image number of sides
//----------------------------------------------------------------------
void SDCMountNewDisk (int drive, const char * path, int raw)
{
    DLOG_C("SDCMountNewDisk %d %s %d\n",drive,path,raw);
    // limit drive to 0 or 1
    drive &= 1;

    // Close and clear previous entry
    UnloadDisk(drive);
    //CloseDrive(drive);
    //gCocoDisk[drive] = {};

    // Convert from SDC format
    char file[MAX_PATH];
    FixFATPath(file,path);

    // Look for pre-existing file
    if (SearchFile(file)) {
        SDCOpenFound(drive,raw);
        return;
    }

    SDCOpenNew(drive,path,raw);
    return;
}

//----------------------------------------------------------------------
// Unload disk in drive
//----------------------------------------------------------------------
void UnloadDisk(int drive) {
    int d = drive & 1;
    if ( gCocoDisk[d].hFile && 
         gCocoDisk[d].hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(gCocoDisk[d].hFile);
        DLOG_C("UnloadDisk %d %s\n",drive,gCocoDisk[d].name);
    }
    gCocoDisk[d] = {};
}

//----------------------------------------------------------------------
// Mount Disk. If image path starts with '/' load drive relative
// to SDRoot, else load drive relative to the current directory.
// If there is no '.' in the path first appending '.DSK' will be
// tried then wildcard. If wildcarded the set will be available for
// the 'Next Disk' function.
// TODO: Sets of type SOMEAPPn.DSK
//----------------------------------------------------------------------
void SDCMountDisk (int drive, const char * path, int raw)
{
    DLOG_C("SDCMountDisk %d %s %d\n",drive,path,raw);

    drive &= 1;

    // Unload previous dsk
    UnloadDisk(drive);

    // Check for UNLOAD.  Path will be an empty string.
    if (*path == '\0') {
        DLOG_C("SDCMountDisk unload %d %s\n",drive,path);
        IF.status = STA_NORMAL;
        if (drive == 0) BuildCartridgeMenu();
        return;
    }

    // Convert from SDC format
    char file[MAX_PATH];
    FixFATPath(file,path);  //TODO use new routine

    // Look for the file
    bool found = SearchFile(file);

    char tmp[MAX_PATH];

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
        DLOG_C("SDCMountDisk not found '%s'\n",file);
        IF.status = STA_FAIL | STA_NOTFOUND;
        return;
    }

    // Mount first image found
    SDCOpenFound(drive,raw);
    return;
}

//----------------------------------------------------------------------
// Mount Next Disk from found set
//----------------------------------------------------------------------
bool SDCMountNext (int drive)
{

    // Can't mount next unless disk set has been loaded
    if (!gFileList.nextload_flag) {
        DLOG_C("SDCMountNext disabled\n");
        return false;
    }

    if (gFileList.files.size() <= 1) {
        DLOG_C("SDCMountNext List empty or only one file\n");
        return false;
    }

    gFileList.cursor++;

    if (gFileList.cursor >= gFileList.files.size()) {
        gFileList.cursor = 0;
        DLOG_C("SDCMountNext reset cursor\n");
    }
    
    DLOG_C("SDCMountNext %d %d\n", 
                gFileList.cursor, gFileList.files.size());

    // Open next image found on the drive
    SDCOpenFound(drive,0);

    return true;
}

//----------------------------------------------------------------------
// Open new disk image
//
//  IF.Param1: $FF49 B   number of cylinders for SDF
//  IF.Param2: $FF4A X.H number of sides for SDF image
//
//  SDCOpenNew 0 'A.DSK' 0 40 0 NEW
//  SDCOpenNew 0 'B.DSK' 0 40 1 NEW++ one side
//  SDCOpenNew 0 'C.DSK' 0 40 2 NEW++ two sides
//
//  Currently new JVC files are 35 cylinders, one sided
//  Possibly future num cylinders could specify 40 or more
//  cylinders with num cyl controlling num sides
//
//----------------------------------------------------------------------
void SDCOpenNew( int drive, const char * path, int raw)
{
    DLOG_C("SDCOpenNew %d '%s' %d %d %d\n",
          drive,path,raw,IF.param1,IF.param2);

    // Number of sides controls file type
    switch (IF.param2) {
    case 0:    //NEW
        // create JVC DSK file
        break;
    case 1:    //NEW+
    case 2:    //NEW++
        DLOG_C("SDCOpenNew SDF file not supported\n");
        IF.status = STA_FAIL | STA_INVALID;
        return;
    }

    namespace fs = std::filesystem;
    fs::path fqn = fs::path(gSDRoot) / gCurDir / path;

    if (fs::is_directory(fqn)) {
        DLOG_C("SDCOpenNew %s is a directory\n",path);
        IF.status = STA_FAIL | STA_INVALID;
        return;
    }
    UnloadDisk(drive);

    strncpy(gCocoDisk[drive].fullpath,fqn.string().c_str(),MAX_PATH);

    // Open file for write
    gCocoDisk[drive].hFile = CreateFile(
        gCocoDisk[drive].fullpath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
        nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);

    if (gCocoDisk[drive].hFile == INVALID_HANDLE_VALUE) {
        DLOG_C("SDCOpenNew fail %d file %s\n",drive,gCocoDisk[drive].fullpath);
        DLOG_C("... %s\n",LastErrorTxt());
        IF.status = STA_FAIL | STA_WIN_ERROR;
        return;
    }

    // Sectorsize and sectors per track
    gCocoDisk[drive].sectorsize = 256;
    gCocoDisk[drive].tracksectors = 18;

    IF.status = STA_FAIL;
    if (raw) {
        // New raw file is empty - can be any format
        IF.status = STA_NORMAL;
        gCocoDisk[drive].doublesided = 0;
        gCocoDisk[drive].headersize = 0;
        gCocoDisk[drive].size = 0;
    } else {
        // TODO: handle SDF
        // Create headerless 35 track JVC file
        gCocoDisk[drive].doublesided = 0;
        gCocoDisk[drive].headersize = 0;
        gCocoDisk[drive].size = 35
                         * gCocoDisk[drive].sectorsize
                         * gCocoDisk[drive].tracksectors
                         + gCocoDisk[drive].headersize;
        // Extend file to size
        LARGE_INTEGER l_siz;
        l_siz.QuadPart = gCocoDisk[drive].size;
        if (SetFilePointerEx(gCocoDisk[drive].hFile,l_siz,nullptr,FILE_BEGIN)) {
            if (SetEndOfFile(gCocoDisk[drive].hFile)) {
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
void SDCOpenFound (int drive,int raw)
{
    drive &= 1;
    int writeprotect = 0;

    if (gFileList.cursor >= gFileList.files.size()) {
        DLOG_C("SDCOpenFound no file %d %d %d\n",
            drive, gFileList.cursor, gFileList.files.size());
        return;
    }

    UnloadDisk(drive);

    std::string name = gFileList.files[gFileList.cursor].name.c_str();
    DLOG_C("SDCOpenFound drive %d %d %s\n",
            drive, gFileList.cursor, name.c_str());

    if (gFileList.files[gFileList.cursor].isDir)
    {
        std::string pattern = name + "/*.DSK";
        DLOG_C("SDCOpenFound %s directory initiate\n",pattern.c_str());
        if (SDCInitiateDir(pattern.c_str())) {
            SDCOpenFound(drive, 0);
            if (drive == 0) BuildCartridgeMenu();
        }
        return;
    }

    std::string file = gFileList.directory + "/" + name;
    DLOG_C("SDCOpenFound %s\n", file.c_str());

    gCocoDisk[drive].hFile = CreateFileA(
        file.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (gCocoDisk[drive].hFile == INVALID_HANDLE_VALUE) {
        DLOG_C("SDCOpenFound fail %d file %s\n",drive,file.c_str());
        DLOG_C("... %s\n",LastErrorTxt());
        int ecode = GetLastError();
        if (ecode == ERROR_SHARING_VIOLATION) {
            IF.status = STA_FAIL | STA_INUSE;
        } else {
            IF.status = STA_FAIL | STA_WIN_ERROR;
        }
        return;
    }

    // If more than one file in list enable nextload function
    if (gFileList.files.size() > 1) gFileList.nextload_flag = true;

    strncpy(gCocoDisk[drive].fullpath,file.c_str(),MAX_PATH);
    strncpy(gCocoDisk[drive].name,name.c_str(),MAX_PATH);

    // Default sectorsize and sectors per track
    gCocoDisk[drive].sectorsize = 256;
    gCocoDisk[drive].tracksectors = 18;

    // Grab filesize 
    gCocoDisk[drive].size = gFileList.files[gFileList.cursor].size;

    // Determine file type (RAW,DSK,JVC,VDK,SDF)
    if (raw) {
        writeprotect = 0;
        gCocoDisk[drive].type = DTYPE_RAW;
        gCocoDisk[drive].headersize = 0;
        gCocoDisk[drive].doublesided = 0;
    } else {

        // Read a few bytes of the file to determine it's type
        unsigned char header[16];
        if (ReadFile(gCocoDisk[drive].hFile,header,12,nullptr,nullptr) == 0) {
            DLOG_C("SDCOpenFound header read error\n");
            IF.status = STA_FAIL | STA_INVALID;
            return;
        }

        // Check for SDF file. SDF file has a 512 byte header and each
        // track record is 6250 bytes. Assume at least 35 tracks so
        // minimum size of a SDF file is 219262 bytes. The first four
        // bytes of the header contains "SDF1"
        if ((gCocoDisk[drive].size >= 219262) &&  // is this reasonable?
            (strncmp("SDF1",(const char *) header,4) == 0)) {
            gCocoDisk[drive].type = DTYPE_SDF;
            DLOG_C("SDCOpenFound SDF file unsupported\n");
            IF.status = STA_FAIL | STA_INVALID;
            return;
        }

        unsigned int numsec;
        gCocoDisk[drive].headersize = gCocoDisk[drive].size & 255;
        DLOG_C("SDCOpenFound headersize %d\n",gCocoDisk[drive].headersize);
        switch (gCocoDisk[drive].headersize) {
        // JVC optional header bytes
        case 4: // First Sector     = header[3]   1 assumed
        case 3: // Sector Size code = header[2] 256 assumed {128,256,512,1024}
        case 2: // Number of sides  = header[1]   1 or 2
        case 1: // Sectors per trk  = header[0]  18 assumed
            gCocoDisk[drive].doublesided = (header[1] == 2);
            gCocoDisk[drive].type = DTYPE_JVC;
            break;
        // No apparant header
        // JVC or OS9 disk if no header, side count per file size
        case 0:
            numsec = gCocoDisk[drive].size >> 8;
            DLOG_C("SDCOpenFound JVC/OS9 sectors %d\n",numsec);
            gCocoDisk[drive].doublesided = ((numsec > 720) && (numsec <= 2880));
            gCocoDisk[drive].type = DTYPE_JVC;
            break;
        // VDK
        case 12:
            gCocoDisk[drive].doublesided = (header[8] == 2);
            writeprotect = header[9] & 1;
            gCocoDisk[drive].type = DTYPE_VDK;
            break;
        // Unknown or unsupported
        default: // More than 4 byte header is not supported
            DLOG_C("SDCOpenFound unsuported image type %d %d\n",
                drive, gCocoDisk[drive].headersize);
            IF.status = STA_FAIL | STA_INVALID;
            return;
            break;
        }
    }

    gCocoDisk[drive].hf = gFileList.files[gFileList.cursor];
    gCocoDisk[drive].filerec = FileRecord(gCocoDisk[drive].hf);

    // Set readonly attrib per find status or file header
    if ((gCocoDisk[drive].filerec.FR_attrib & ATTR_RDONLY) != 0) {
        writeprotect = 1;
    } else if (writeprotect) {
        gCocoDisk[drive].filerec.FR_attrib |= ATTR_RDONLY;
    }

    // If file is writeable reopen read/write
    if (!writeprotect) {
        CloseHandle(gCocoDisk[drive].hFile);
        gCocoDisk[drive].hFile = CreateFile(
            gCocoDisk[drive].fullpath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
            nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if (gCocoDisk[drive].hFile == INVALID_HANDLE_VALUE) {
            DLOG_C("SDCOpenFound reopen fail %d\n",drive);
            DLOG_C("... %s\n",LastErrorTxt());
            IF.status = STA_FAIL | STA_WIN_ERROR;
            return;
        }
    }

    if (drive == 0) BuildCartridgeMenu();

    IF.status = STA_NORMAL;
    return;
}

//----------------------------------------------------------------------
// Convert file name from FAT format and prepend current dir.
//----------------------------------------------------------------------
std::string SDCGetFullPath(const std::string& file)
{
    std::string out = gSDRoot;
    if (gCurDir.size() > 0) out += '/' + gCurDir;
    out += '/' + FixFATPath(file);

    DLOG_C("SDCGetFullPath in %s out %s\n",file.c_str(),out.c_str());
    return (out);
}

//----------------------------------------------------------------------
// Rename disk or directory
//   names contains two consecutive null terminated name strings
//   first is file or directory to rename, second is target name
//----------------------------------------------------------------------
void SDCRenameFile(const char *names)
{
    const char* p = names;
    std::string sfrom = p;
    p += sfrom.size() + 1;
    std::string starget = p;

    DLOG_C("SDCRenameFile %s to %s\n",sfrom.c_str(),starget.c_str());
    std::string from = SDCGetFullPath(sfrom);
    std::string target = SDCGetFullPath(starget);

    if (std::rename(from.c_str(),target.c_str())) {
        DLOG_C("SDCRenameFile %s\n", strerror(errno));
        IF.status = STA_FAIL | STA_WIN_ERROR;
    } else {
        IF.status = STA_NORMAL;
    }
    return;
}

//----------------------------------------------------------------------
// Delete disk or directory
//----------------------------------------------------------------------
void SDCKillFile(const char* file)
{
    namespace fs = std::filesystem;

    fs::path p = SDCGetFullPath(std::string(file));
    std::error_code ec;

    DLOG_C("SDCKillFile %s\n",file);

    // Does it exist?
    if (!fs::exists(p, ec)) {
        DLOG_C("SDCKillFile does not exist\n");
        IF.status = STA_FAIL | STA_NOTFOUND;
        return;
    }
    // Regular file
    if (fs::is_regular_file(p, ec)) {
        if (fs::remove(p, ec)) {
            IF.status = STA_NORMAL;
        } else {
            DLOG_C("SDCKillFile error: %s\n", ec.message().c_str());
            IF.status = STA_FAIL | STA_WPROTECT;
        }
        return;
    }
    // Directory
    if (fs::is_directory(p, ec)) {
        if (fs::is_empty(p, ec)) {
            if (fs::remove(p, ec)) {
                IF.status = STA_NORMAL;
            } else {
                IF.status = STA_FAIL | STA_WPROTECT;
                DLOG_C("SDCKillFile dir error: %s\n", ec.message().c_str());
            }
        } else {
            DLOG_C("SDCKillFile directory not empty\n");
            IF.status = STA_FAIL | STA_NOTEMPTY;
        }
        return;
    }
    // Not a file or directory (symlink, device, etc.)
    IF.status = STA_FAIL;
}

//----------------------------------------------------------------------
// Create directory
//----------------------------------------------------------------------

void SDCMakeDirectory(const char* name)
{
    namespace fs = std::filesystem;

    std::string s = SDCGetFullPath(std::string(name));
    fs::path p(s);

    DLOG_C("SDCMakeDirectory %s\n", s.c_str());

    std::error_code ec;

    if (fs::exists(p, ec)) {
        DLOG_C("SDCMakeDirectory already exists\n");
        IF.status = STA_FAIL | STA_INVALID;
        return;
    }

    if (fs::create_directory(p, ec)) {
        IF.status = STA_NORMAL;
    } else {
        DLOG_C("SDCMakeDirectory error: %s\n", ec.message().c_str());
        IF.status = STA_FAIL | STA_WIN_ERROR;
    }
}

//----------------------------------------------------------------------
// Set the Current Directory relative to previous current or to SDRoot
// This is complicated by the many ways a user can change the directory
//----------------------------------------------------------------------

void SDCSetCurDir(const char* branch)
{
    namespace fs = std::filesystem;

    DLOG_C("SetCurdir '%s'\n", branch);

    fs::path cur  = gCurDir;    // current relative directory
    fs::path root = gSDRoot;    // SD card root

    // "." or "" no change
    if (*branch == '\0' || strcmp(branch, ".") == 0) {
        DLOG_C("SetCurdir no change\n");
        IF.status = STA_NORMAL;
        return;
    }

    // "/" go to root
    if (strcmp(branch, "/") == 0) {
        gCurDir = "";
        DLOG_C("SetCurdir to root\n");
        IF.status = STA_NORMAL;
        return;
    }

    // ".." parent directory
    if (strcmp(branch, "..") == 0) {
        cur = cur.parent_path();
        gCurDir = cur.string();
        FixDirSlashes(gCurDir);
        DLOG_C("SetCurdir back %s\n", gCurDir.c_str());
        IF.status = STA_NORMAL;
        return;
    }

    //  Reject "//"
    if (strncmp(branch, "//", 2) == 0) {
        DLOG_C("SetCurdir // invalid\n");
        IF.status = STA_FAIL | STA_INVALID;
        return;
    }

    //  Build the candidate directory
    bool relative = (branch[0] != '/');

    fs::path candidate;
    if (relative) {
        candidate = root / cur / branch;
    } else {
        candidate = root / fs::path(branch).relative_path();
    }

    //  Validate directory
    if (!fs::is_directory(candidate)) {
        DLOG_C("SetCurdir not a directory %s\n", candidate.string().c_str());
        IF.status = STA_FAIL | STA_NOTFOUND;
        return;
    }

    //  Update CurDir
    fs::path newCur;
    if (relative) {
        newCur = cur / branch;
    } else {
        newCur = fs::path(branch).relative_path();
    }

    // String based host files
    gCurDir = newCur.string();
    FixDirSlashes(gCurDir);

    DLOG_C("SetCurdir set to '%s'\n", gCurDir.c_str());
    IF.status = STA_NORMAL;
}

//----------------------------------------------------------------------
// SDCInitiateDir command.
//----------------------------------------------------------------------
bool SDCInitiateDir(const char * path)
{
    DLOG_C("SDCInitiateDir '%s'\n",path);

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
    if (rc) {
        IF.status = STA_NORMAL;
        return true;
    } else {
        IF.status = STA_FAIL | STA_INVALID;
        return false;
    }
}
