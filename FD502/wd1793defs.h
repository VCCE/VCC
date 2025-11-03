#pragma once
#include "defines.h"
#include <Windows.h>



struct DiskInfo 
{
	HANDLE FileHandle;
	char ImageName[MAX_PATH];
	long FileSize;				
	unsigned char HeaderSize;
	unsigned char Sides;		// Number of Sides 1 ot 2
	unsigned char Sectors;		//Sectors Per Track
	unsigned char SectorSize;	//0=128 1=256 2=512 3=1024
	unsigned char FirstSector;	// 0 or 1
	unsigned char ImageType;	//0=JVC 1=VDK 2=DMK 3=OS9
	unsigned short TrackSize;
	unsigned char WriteProtect;
	unsigned char HeadPosition;	// The "Physical" Track the head is over
	char ImageTypeName[4];
};

struct SectorInfo
{
	unsigned char Track = 0;
	unsigned char Side = 0;
	unsigned char Sector = 0;
	unsigned short Lenth = 0;
	unsigned short CRC = 0;
	long DAM = 0;
	unsigned char Density = 0;
};


#define IDLE 255
#define NONE 4



//Control register masks
#define CTRL_DRIVE0		0x01
#define CTRL_DRIVE1		0x02
#define CTRL_DRIVE2		0x04
#define CTRL_MOTOR_EN	0x08
#define CTRL_WRT_PRECMP	0x10
#define CTRL_DENSITY	0x20
#define CTRL_DRIVE3		0x40
#define CTRL_HALT_FLAG	0x80
#define SIDESELECT		0x40

//Status Register return codes
#define READY			0
#define	BUSY			0x01
#define DRQ				0x02
#define INDEXPULSE		0x02
#define TRACK_ZERO		0x04
#define LOSTDATA		0x04
#define RECNOTFOUND		0x10
#define WRITEPROTECT	0x40


#define WAITTIME		5
#define STEPTIME		15	// 1 Millisecond = 15.78 Pings
#define SETTLETIME		10	// CyclestoSettle

#define RESTORE			0
#define	SEEK			1
#define STEP			2
#define STEPUPD			3
#define	STEPIN			4
#define	STEPINUPD		5
#define STEPOUT			6
#define STEPOUTUPD		7
#define	READSECTOR		8
#define READSECTORM		9
#define WRITESECTOR		10
#define	WRITESECTORM	11
#define	READADDRESS		12
#define	FORCEINTERUPT	13
#define	READTRACK		14
#define WRITETRACK		15

#define HEADERBUFFERSIZE	256

//
#define DISK_SIDES          1       // sides per disk
#define DISK_TRACKS         35      // tracks per side
#define DISK_SECTORS        18      // sectors per track
#define DISK_DATARATE       2       // 2 is double-density
#define SECTOR_SIZE_CODE    1       // 0=128, 1=256, 2=512, 3=1024, ...
#define SECTOR_GAP3         16    // gap3 size between sectors
#define SECTOR_FILL         0xFF    // fill byte for formatted sectors
#define SECTOR_BASE         1       // first sector number on track
#define TRACK_SKEW          1       // format skew to the same sector on the next track

#define TRACK_SIZE          ((128<<SECTOR_SIZE_CODE)*DISK_SECTORS)
