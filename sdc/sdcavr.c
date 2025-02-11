//----------------------------------------------------------------------
//
// This file is part of VCC (Virtual Color Computer).
// Vcc is Copyright 2015 by Joseph Forgione
//
// VCC (Virtual Color Computer) is free software, you can redistribute it 
// and/or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see 
// <http://www.gnu.org/licenses/>.
//
// This is the Vcc SDC interface by E J Jaquay
//
//----------------------------------------------------------------------
//
//  Address       Description
//  -------       -----------
//  FF40          Latch Register 
//  FF48          Status/Command
//  FF49          Param 1
//  FF4A          Param 2
//  FF4B          Param 3
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

#include "sdcavr.h"
#include "defines.h"
#include "..\fileops.h"
#include "..\logger.h"

void SetMode(unsigned int);
void SDCCommand(unsigned int);
void ReadSector(unsigned int);
void StreamImage(unsigned int);
void WriteSector(unsigned int);
void UnusedCommand(unsigned int);
void GetDriveInfo(unsigned int);
void SDCCONTROL(unsigned int);
void SetDrive(unsigned int);
unsigned char ReadNextByte();
void LoadReply(char *, int);

bool SDC_Mode=false;

unsigned char DriveNum = 0;
unsigned char CmdPrm1 = 0;
unsigned char CmdPrm2 = 0;
unsigned char CmdPrm3 = 0;

// Status Register values
#define STA_FAIL  0x80
#define STA_READY 0x02
#define STA_BUSY  0X01

// File Attribute values
#define ATTR_DIR 0x10
#define ATTR_SDF 0x04
#define ATTR_HIDDEN 0x02
#define ATTR_LOCKED 0x01

unsigned char CmdSta = 0;
unsigned char CmdRpy1 = 0;
unsigned char CmdRpy2 = 0;
unsigned char CmdRpy3 = 0;

// Reply buffer. ReplyCount is the number of bytes remaining
// in the buffer. Characters from the buffer are consumed as 
// ports 0xFF4A and 0xFF4B are read. Bytes are pre-swapped in
// the buffer to match the order in which they are read.
char ReplyBuf[260];
int ReplyCount = 0;
char *ReplyPtr = ReplyBuf;

// Write buffer.
char WriteBuf[260];
int WriteCount = 0;
char *WritePtr = WriteBuf;

// Drive handles
HANDLE hDrive[2] = { NULL, NULL };

// Drive info buffers
#pragma pack(1)
struct Drive_Info {
	union {
		struct {
			char name[8];
			char type[3];
			char attrib;
			DWORD lo_size;
			DWORD hi_size;
   		} file;
   		char bytes[256];
   		DWORD words[128];
	};
};
//struct Drive_Info Drive1_Info;
//struct Drive_Info Drive2_Info;
#pragma pack()

struct Drive_Info DriveInfo[2];
void UnLoadDrive (int);
void LoadDrive (char*,int);

//----------------------------------------------------------------------
// Write SDC port
//----------------------------------------------------------------------

void SDCWrite(unsigned char data,unsigned char port)
{
    switch (port) {
    case 0x40:
		SetMode(data);
        break;
    case 0x48:
        SDCCommand(data);
        break;
    case 0x49:
//PrintLogC("W1:%02X ",data);
        CmdPrm1 = data;
        break;
    case 0x4A:
//PrintLogC("W2:%02X ",data);
        CmdPrm2 = data;
        break;
    case 0x4B:
//PrintLogC("W3:%02X ",data);
        CmdPrm3 = data;
        break;
    }
    return;
}

//----------------------------------------------------------------------
// Read SDC port
//----------------------------------------------------------------------
unsigned char SDCRead(unsigned char port)
{
	unsigned char rpy;

	switch (port) {
    case 0x48:
		rpy = CmdSta;
PrintLogC("RS:%02X ",rpy);
		break;

	case 0x49:
		rpy = CmdRpy1;
PrintLogC("R1:%02X ",rpy);
		break;

	case 0x4A:
		if (ReplyCount > 0) {
			rpy = ReadNextByte();
		} else {
			rpy = CmdRpy2;
PrintLogC("R2:%02X ",rpy);
		}
		break;
   
	case 0x4B:
		if (ReplyCount > 0) {
			rpy = ReadNextByte();
		} else {
			rpy = CmdRpy3;
PrintLogC("R3:%02X ",rpy);
		}
		break;

	default:
		rpy = 0;
		break;
    }
    return rpy;
}

//----------------------------------------------------------------------
// Init the controller
//----------------------------------------------------------------------
void SDCInit()
{
// Load a drive for testing
PrintLogC("\nLoading a test drive\n");
char *file1 = "C:\\Users\\ed\\vcc\\sets\\sdc-boot\\bgames.dsk";
char *file0 = "C:\\Users\\ed\\vcc\\sets\\sdc-boot\\63sdc.vhd";
LoadDrive(file0,0);
LoadDrive(file1,1);
	SetMode(0);
	return;
}

//----------------------------------------------------------------------
// Reset the controller
//----------------------------------------------------------------------
void SDCReset()
{
PrintLogC("SDCREset Closing Drives\n");
	CloseHandle(hDrive[0]);
	CloseHandle(hDrive[1]);
	hDrive[0] = NULL;
	hDrive[1] = NULL;
	SetMode(0);
	return;
}

//----------------------------------------------------------------------
// Set SDC controller mode
//----------------------------------------------------------------------
void SetMode(unsigned int data)
{
	
//if (data==0x43) PrintLogC("\n");
//PrintLogC("M:%02X ",data);

	// Wait for commands to finish here
	if (data == 0) {
	    SDC_Mode = false;
		CmdSta  = 0;
		CmdRpy1 = 0;
		CmdRpy2 = 0;
		CmdRpy3 = 0;
	} else if (data == 0x43) {
	    SDC_Mode = true;
		CmdSta  = 0;
	}
	return;
}

//----------------------------------------------------------------------
//  Dispatch SDC commands
//----------------------------------------------------------------------
void SDCCommand(unsigned int code)
{
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
        WriteSector(loNib);
        break;
        break;
    case 0xC0:
        GetDriveInfo(loNib);
        break;
	case 0xD0:
		SDCCONTROL(loNib);
		break;
    case 0xE0:
        SetDrive(loNib);
        break;
	default:
    	CmdSta = STA_FAIL;  // Fail
        break;		
	}
	return;
}

//----------------------------------------------------------------------
// Log the last error
//----------------------------------------------------------------------
char * LastError() {
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
void ReadSector(unsigned int loNib)
{

// loNib:
// b0 Drive Num
// b1 single sided LSN
// b2 8bit transfers flag
// p1 high order LSN
// p2,p3 low order LSN

	if (!SDC_Mode) {
PrintLogC("\nRead Sector Ignored\n"); 
		 return;
	}
	if ((loNib & 4) !=0 ) {
PrintLogC("8bit transfer not supported\n");
	 	CmdSta = STA_FAIL;
	}

	int lsn = CmdPrm1 * 256 * 256 + CmdPrm2 * 256 + CmdPrm3;
PrintLogC("\nRead drv %d lsn %d\n",loNib&1,lsn);

 	char dsec[260];
 	HANDLE hf = hDrive[loNib&1];
 	if (hf != NULL) {
 		LARGE_INTEGER pos;
 		pos.QuadPart = lsn * 256;
 		SetFilePointerEx(hf,pos,NULL,FILE_BEGIN);
 		DWORD cnt;
		DWORD num = 256;
 		ReadFile(hf,dsec,num,&cnt,NULL);
		if (cnt != num) {
	    	CmdSta = STA_FAIL;  // Fail
PrintLogC("h:%d Read %d bytes %s\n",hf,cnt,LastError());
		} else {
 			LoadReply(dsec,256);
		}
	} else {
PrintLogC("\nRead null handle\n");
	    CmdSta = STA_FAIL;  // Fail
	}
}

//----------------------------------------------------------------------
// Stream image data
//----------------------------------------------------------------------
void StreamImage(unsigned int loNib)
{
	if (!SDC_Mode) {
PrintLogC("Stream Ignored\n"); 
		return;
	}
PrintLogC("\nStream %d %02X %02X %02X ",loNib,CmdPrm1,CmdPrm2,CmdPrm3); 
PrintLogC(" Not supported\n"); 
    CmdSta = STA_FAIL;  // Fail
}

//----------------------------------------------------------------------
// Write logical sector
//----------------------------------------------------------------------
void WriteSector(unsigned int loNib)
{
	if (!SDC_Mode) {
PrintLogC("\nWriteSec Ignored\n"); 
		return;
	}
	int lsn = CmdPrm1 * 256 * 256 + CmdPrm2 * 256 + CmdPrm3;
PrintLogC("\nWriteSec %d lsn %d ",lsn); 
PrintLogC("Not supported\n"); 
    CmdSta = STA_FAIL;  // Fail
}

//----------------------------------------------------------------------
// Get drive information
//----------------------------------------------------------------------
void GetDriveInfo(unsigned int loNib)
{

	 if (!SDC_Mode) {
PrintLogC("GetInfo Ignored\n"); 
		 return;
	 }

	int drive = loNib & 1;
	switch (CmdPrm1) {
	case 'I':
		LoadReply(DriveInfo[drive].bytes,64);
		break;

	default:
PrintLogC("\nGetInfo drive %d ",loNib,CmdPrm1,CmdPrm2,CmdPrm3); 
PrintLogC("Not supported\n",loNib);
		CmdSta = STA_FAIL;
		break;
	}
	return;
}

//----------------------------------------------------------------------
// Abort stream or mount disk in a set of disks?
//----------------------------------------------------------------------
void SDCCONTROL(unsigned int loNib)
{
PrintLogC("\nAbortStream\n");
//  Why is CmdPrm1 being set to 0x5A ('Z') by SDC-DOS when it starts up?
//  TODO abort stream here.
	SetMode(false);
}

//----------------------------------------------------------------------
//  Mount or create image, set or create directory, 
//  list directory, delete file or directory
//----------------------------------------------------------------------
void SetDrive(unsigned int loNib)
{
PrintLogC("\nSetDrive %d %02X %02X %02X ",loNib,CmdPrm1,CmdPrm2,CmdPrm3); 
	 if (!SDC_Mode) {
PrintLogC("Ignored\n"); 
		 return;
	 }
    CmdSta = STA_FAIL;  // Fail
}

//----------------------------------------------------------------------
// Load reply.  Buffer bytes are swapped in words so they
// are read in the correct order.  Count is bytes, 256 max
//----------------------------------------------------------------------
void LoadReply(char *data, int count)
{
	if (CmdSta != 0) {
PrintLogC("Load reply busy\n");
	    CmdSta = STA_FAIL;
		return;
	} else if ((count < 2) | (count > 256)) {
PrintLogC("Load reply bad count\n");
	    CmdSta = STA_FAIL;
		return;
	}

	char *dp = data;
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
	ReplyCount = 256;
PrintLogC("Loaded %d reply bytes\n",count);
	return;
}

//----------------------------------------------------------------------
// Get next byte from reply buffer.
//----------------------------------------------------------------------
unsigned char ReadNextByte()
{
	ReplyCount--;
	if (ReplyCount < 1) {
		CmdSta = 0;
	} else {
		CmdSta |= STA_BUSY;
	}
	unsigned char byte = *ReplyPtr++;
	return byte;
}

//----------------------------------------------------------------------
// Unload virtual disk from drive
//----------------------------------------------------------------------
void UnLoadDrive (int drivenum)
{
	drivenum &= 1;
	if (hDrive[drivenum] != NULL) {
PrintLogC("\nClosing Drive %d\n",drivenum);
 		CloseHandle(hDrive[drivenum]);
		hDrive[drivenum] = NULL;
	}
	memset(DriveInfo[drivenum].bytes,0,sizeof(Drive_Info));
}	

//----------------------------------------------------------------------
// Load virtual disk into drive
//----------------------------------------------------------------------
void LoadDrive (char * filename, int drivenum)
{
PrintLogC("\nLoading Drive %d ",drivenum);

	drivenum &= 1;
	if (hDrive[drivenum]) UnLoadDrive(drivenum);

	HANDLE hf;
 	hf = CreateFile(filename,GENERIC_READ,0,NULL,
		 			OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf == NULL) {
PrintLogC("failed %s\n",LastError());
		CmdSta = STA_FAIL;
	} else {
 		hDrive[drivenum] = hf;

//TODO: fill in real file info
		strncpy(DriveInfo[0].file.name,"F0",8);
		strncpy(DriveInfo[0].file.type,"DAT",3);
		DriveInfo[0].file.attrib = ATTR_DIR;
		DriveInfo[0].file.lo_size = 0;
		DriveInfo[0].file.hi_size = 32;

		strncpy(DriveInfo[1].file.name,"F1",8);
		strncpy(DriveInfo[1].file.type,"DAT",3);
		DriveInfo[1].file.attrib = ATTR_DIR;
		DriveInfo[1].file.lo_size = 0;
		DriveInfo[1].file.hi_size = 32;

PrintLogC("hf %d %d Success\n",hf,hDrive[drivenum]);

	}
}

