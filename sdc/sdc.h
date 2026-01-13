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
//======================================================================

#pragma once

#include <Windows.h>
#include <vcc/core/logger.h>
#include <vcc/core/interrupts.h>

#include "resource.h"

// Return values for status register
#define STA_NORMAL    0x00
#define STA_BUSY      0x01
#define STA_READY     0x02   // reply from command is ready
#define STA_INVALID   0x04   // file or dir path is invalid
#define STA_WIN_ERROR 0x08   // Misc windows error
#define STA_INITERROR 0x08   // Directory not initiated
#define STA_READERROR 0x08   // Read error
#define STA_BAD_LSN   0x10   // LSN or no image error
#define STA_NOTFOUND  0x10   // file (directory) not found
#define STA_DELETED   0x20   // Sector deleted mark
#define STA_INUSE     0x20   // image already in use
#define STA_NOTEMPTY  0x20   // Delete directory error
#define STA_WPROTECT  0x40   // write protect error
#define STA_FAIL      0x80

#define FLP_NORMAL    0x00
#define FLP_BUSY      0x01   // b0 Busy
#define FLP_DATAREQ   0x02   // b1 DRQ
#define FLP_DATALOST  0x04   // b2 Lost data
#define FLP_READERR   0x08   // b3 Read error (CRC)
#define FLP_SEEKERR   0x10   // b4 Seek error
#define FLP_WRITEERR  0x20   // b5 Write error (Fault)
#define FLP_READONLY  0x40   // b6 Write protected
#define FLP_NOTREADY  0x80   // b7 Not ready

// Single byte file attributes for File info records
#define ATTR_NORM     0x00
#define ATTR_RDONLY   0x01
#define ATTR_HIDDEN   0x02
#define ATTR_SDF      0x04
#define ATTR_DIR      0x10

// Disk Types
enum DiskType {
    DTYPE_RAW = 0,
    DTYPE_DSK,
    DTYPE_JVC,
    DTYPE_VDK,
    DTYPE_SDF
};

// Limit maximum dsk file size.
#define MAX_DSK_SECTORS 2097152

// HostFile contains a file name, it's size, and directory and readonly flags
struct HostFile
{
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

// FileList contains a list of HostFiles
struct FileList
{
    std::vector<HostFile> files;    // files
    std::string directory;          // directory files are in
    size_t cursor = 0;              // current file curspr
    bool nextload_flag = false;     // enable next disk loading
    // append a host file to the list
    void append(const HostFile& hf) { files.push_back(hf); }
};

// FileRecord is 16 packed bytes file name, type, attrib, and size that can
// be passed directly via the SDC interface.
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
    FileRecord(const HostFile& hf) noexcept;
};
#pragma pack(pop)

// CocoDisk contains info about mounted disk files 0 and 1 
struct CocoDisk
{
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

