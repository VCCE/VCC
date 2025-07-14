// Uncomment to get console logging
//#define USE_LOGGING

#include "../defines.h"
#include "../logger.h"

#include "resource.h"
#include "cloud9.h"

// Return values for status register
#define STA_NORMAL    0x00
#define STA_BUSY      0x01
#define STA_READY     0x02   // reply from command is ready
#define STA_INVALID   0x04   // file or dir path is invalid
#define STA_WIN_ERROR 0x08   // Misc windows error
#define STA_INITERROR 0x08   // Directory not initiated
#define STA_CRCERROR  0x08   // Read error
#define STA_BAD_LSN   0x10   // LSN or no image error
#define STA_NOTFOUND  0x10   // file not found
#define STA_DELETED   0x20   // Sector deleted mark
#define STA_INUSE     0x20   // image already in use
#define STA_NOTEMPTY  0x20   // Delete directory error
#define STA_WPROTECT  0x40   // write protect error
#define STA_FAIL      0x80

// Single byte file attributes for File info records
#define ATTR_NORM     0x00
#define ATTR_RDONLY   0x01
#define ATTR_HIDDEN   0x02
#define ATTR_SDF      0x04
#define ATTR_DIR      0x10
#define ATTR_INVALID   -1   //not 0xFF because byte is signed

// Self imposed limit on maximum dsk file size.
#define MAX_DSK_SECTORS 2097152

// Mount disk flags
enum MountFlags
{
    MOUNT_NORM = 0,
    MOUNT_RAW = 1 << 0
};
DEFINE_ENUM_FLAG_OPERATORS(MountFlags)

