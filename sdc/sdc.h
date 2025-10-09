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

#include <vcc/common/logger.h>
#include <vcc/common/interrupts.h>

#include "resource.h"
#include "cloud9.h"

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

// Self imposed limit on maximum dsk file size.
#define MAX_DSK_SECTORS 2097152

