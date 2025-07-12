// Uncomment to get console logging
#define USE_LOGGING

#include "../defines.h"
#include "../logger.h"

#include "resource.h"
#include "cloud9.h"

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

