/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

// cc3vhd.c

/****************************************************************************
*   Technical specs on the Virtual Hard Disk interface
*
*   Address       Description
*   -------       -----------
*   FF80          Logical record number (high byte)
*   FF81          Logical record number (middle byte)
*   FF82          Logical record number (low byte)
*   FF83          Command/status register
*   FF84          Buffer address (high byte)
*   FF85          Buffer address (low byte)
*   FF86          Drive select (0 or 1)
*
*   Set the other registers, and then issue a command to FF83 as follows:
*
*    0 = read 256-byte sector at LRN
*    1 = write 256-byte sector at LRN
*    2 = flush write cache (Closes and then opens the image file)
*        Note: Vcc just issues a "FlushFileBuffers" command.
*
*   Error values:
*
*    0 = no error
*   -1 = power-on state (before the first command is recieved)
*   -2 = invalid command
*    2 = VHD image does not exist
*    4 = Unable to open VHD image file
*    5 = access denied (may not be able to write to VHD image)
*
*   IMPORTANT: The I/O buffer must NOT cross an 8K MMU bank boundary.
*   Note: This is not an issue for Vcc.
****************************************************************************/

#include <Windows.h>
#include <stdio.h>
#include "cc3vhd.h"
#include "harddisk.h"
#include "defines.h"
#include "../fileops.h"

typedef union {
    unsigned int All;
    struct {
        unsigned char lswlsb,lswmsb,mswlsb,mswmsb;
    } Byte;
} SECOFF;

typedef union {
    unsigned int word;
    struct {
        unsigned char lsb,msb;
    } Byte;
} Address;

static int DriveSelect=0;
static HANDLE HardDrive[2]={INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE};
static SECOFF SectorOffset;
static Address DMAaddress;
static unsigned char SectorBuffer[SECTORSIZE];
static unsigned char Mounted[2]={0,0};
static unsigned char WpHD[2]={0,0};
static unsigned short ScanCount=64;
static unsigned long LastSectorNum=0;
static char DStatus[128]="";
static char Status = HD_PWRUP;
unsigned long BytesMoved=0;

void HDcommand(unsigned char);

int MountHD(const char* FileName, int drive)
{
    drive = drive&1;  // Drive can be 0 or 1

    // Unmount existing
    if (HardDrive[drive] != INVALID_HANDLE_VALUE) UnmountHD(drive);

    // Assume mount will work for now
    WpHD[drive]=0;
    Mounted[drive]=1;
    Status = HD_OK;
    SectorOffset.All = 0;
    DMAaddress.word = 0;
    HardDrive[drive] = CreateFile( FileName,
                                   GENERIC_READ | GENERIC_WRITE,
                                   0,0,OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,0);

    // If can't open read/write try read only.
    if (HardDrive[drive] == INVALID_HANDLE_VALUE) {
        HardDrive[drive] = CreateFile( FileName,
                                       GENERIC_READ,
                                       0,0,OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL,0);
        WpHD[drive]=1; // drive is write protected
    }

    // Give up if can't open either way
    if (HardDrive[drive] == INVALID_HANDLE_VALUE) {
        WpHD[drive]=0;
        Mounted[drive]=0;
        Status = HD_NODSK;
        return 0;
    }
    return 1;
}

void UnmountHD(int drive)
{
    drive = drive&1;  // Drive can be 0 or 1

    if (HardDrive[drive] != INVALID_HANDLE_VALUE) {
        CloseHandle(HardDrive[drive]);
        HardDrive[drive] = INVALID_HANDLE_VALUE;
        Mounted[drive] = 0;
        Status = HD_NODSK;
    }
    return;
}

// Clear drive select on reset
void VhdReset(void) {
    MemWrite(0,0xFF86);
}

void HDcommand(unsigned char Command) {

    unsigned short Temp=0;

    // Verify drive is mounted
    if (Mounted[DriveSelect] == 0) {
        Status = HD_NODSK;
        return;
    }

    switch (Command) {

    case SECTOR_READ:
        // Verify desired sector is in range
        if (SectorOffset.All > MAX_SECTOR) {
            Status = HD_NODSK;
            return;
        }

        // Seek desired sector
        SetFilePointer(HardDrive[DriveSelect],SectorOffset.All,0,FILE_BEGIN);

        // Read it; zero fill if past end of file
        ReadFile(HardDrive[DriveSelect],SectorBuffer,SECTORSIZE,&BytesMoved,nullptr);
        for (Temp=0; Temp < SECTORSIZE;Temp++) {
            if (Temp > BytesMoved) {
                MemWrite(0,Temp+DMAaddress.word);
            } else {
                MemWrite(SectorBuffer[Temp],Temp+DMAaddress.word);
            }
        }
        Status = HD_OK;

        // Put disk status text
        sprintf(DStatus,"HD%d: Rd %000000.6X",DriveSelect,SectorOffset.All>>8);

        break;

    case SECTOR_WRITE:
        // Verify desired sector is in range
        if (SectorOffset.All > MAX_SECTOR) {
            Status = HD_NODSK;
            return;
        }
        
        // Check for write protect (file opened read only)
        if (WpHD[DriveSelect] == 1 ) {
            Status = HD_WP;
            return;
        }

        // Copy block from from CoCo RAM
        for (Temp=0; Temp <SECTORSIZE;Temp++) {
            SectorBuffer[Temp]=MemRead(Temp+DMAaddress.word);
        }

        // Seek desired sector
        SetFilePointer(HardDrive[DriveSelect],SectorOffset.All,0,FILE_BEGIN);

        // Write it
        WriteFile(HardDrive[DriveSelect],SectorBuffer,SECTORSIZE,&BytesMoved,nullptr);
        if (BytesMoved != SECTORSIZE) {
            Status = HD_NODSK;
        } else {
            Status = HD_OK;
        }

        // Put disk status text
        sprintf(DStatus,"HD: Wr Sec %000000.6X",SectorOffset.All>>8);

        break;

    case DISK_FLUSH:
        FlushFileBuffers(HardDrive[DriveSelect]);
        SectorOffset.All=0;
        DMAaddress.word=0;
        Status = HD_OK;
        break;

    default:
        Status = HD_INVLD;
        return;
    }
}

void DiskStatus(char *Temp)
{
    strcpy(Temp,DStatus);
    ScanCount++;

    if (SectorOffset.All != LastSectorNum) {
        ScanCount=0;
        LastSectorNum=SectorOffset.All;
    }

    if (ScanCount > 63) {
        ScanCount=0;
        if (Mounted[DriveSelect]==1) {
            sprintf(DStatus,"HD%d:IDLE",DriveSelect);
        } else {
            sprintf(DStatus,"HD%d:No Image!",DriveSelect);
        }
    }
    return;
}

void IdeWrite(unsigned char data,unsigned char port)
{
    switch (port-0x80) {
    case 0:
        SectorOffset.Byte.mswmsb = data;
        break;
    case 1:
        SectorOffset.Byte.mswlsb = data;
        break;
    case 2:
        SectorOffset.Byte.lswmsb = data;
        break;
    case 3:
        HDcommand(data);
        break;
    case 4:
        DMAaddress.Byte.msb=data;
        break;
    case 5:
        DMAaddress.Byte.lsb=data;
        break;
    case 6:
        // Only select drive if data is 0 or 1
        if (data == (data&1)) DriveSelect = data;
        break;
    }
    return;
}

unsigned char IdeRead(unsigned char port)
{
    switch (port-0x80) {
    case 0:
        return(SectorOffset.Byte.mswmsb);
    case 1:
        return(SectorOffset.Byte.mswlsb);
    case 2:
        return(SectorOffset.Byte.lswmsb);
    case 3:
        return Status;
    case 4:
        return(DMAaddress.Byte.msb);
    case 5:
        return(DMAaddress.Byte.lsb);
    case 6:
        return DriveSelect;
    }
    return 0;
}
