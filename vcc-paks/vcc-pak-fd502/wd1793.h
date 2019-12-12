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

#ifndef _wd1793_h_
#define _wd1793_h_

/*****************************************************************************/

#include "file.h"
#include "emuDevice.h"
#include "cpu.h"

#include <stdio.h>

/*****************************************************************************/

#define IDLE 255
#define NONE 254

#define JVC	0
#define VDK	1
#define DMK	2
#define OS9	3
#define RAW 4


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

#if defined KEYBOARD_LEDS_SUPPORTED
#define IOCTL_KEYBOARD_SET_INDICATORS        CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_TYPEMATIC       CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_INDICATORS      CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#define KEYBOARD_CAPS_LOCK_ON     4
#define KEYBOARD_NUM_LOCK_ON      2
#define KEYBOARD_SCROLL_LOCK_ON   1


//
#define DISK_SIDES          1       // sides per disk
#define DISK_TRACKS         35      // tracks per side
#define DISK_SECTORS        18      // sectors per track
#define DISK_DATARATE       2       // 2 is double-density
#define SECTOR_SIZE_CODE    1       // 0=128, 1=256, 2=512, 3=1024, ...
#define SECTOR_GAP3         16		// gap3 size between sectors
#define SECTOR_FILL         0xFF    // fill byte for formatted sectors
#define SECTOR_BASE         1       // first sector number on track
#define TRACK_SKEW          1       // format skew to the same sector on the next track

#define TRACK_SIZE          ((128<<SECTOR_SIZE_CODE)*DISK_SECTORS)

/*****************************************************************************/
/**
 */
typedef struct DiskInfo DiskInfo;
struct DiskInfo 
{
	FILE *			FileHandle;
	char *	        pPathname;
	size_t			FileSize;				
	unsigned char	HeaderSize;
	unsigned char	Sides;			// Number of Sides 1 ot 2
	unsigned char	Sectors;		// Sectors Per Track
	unsigned char	SectorSize;		// 0=128 1=256 2=512 3=1024
	unsigned char	FirstSector;	// 0 or 1
	unsigned char	ImageType;		// 0=JVC 1=VDK 2=DMK 3=OS9
	unsigned short	TrackSize;
	unsigned char	WriteProtect;
	unsigned char	HeadPosition;	// The "Physical" Track the head is over
	unsigned char	RawDrive;
	char			ImageTypeName[4];
};

/**
 */
typedef struct SectorInfo SectorInfo;
struct SectorInfo
{
	unsigned char	Track;
	unsigned char	Side;
	unsigned char	Sector;
	unsigned short	Length;
	unsigned short	CRC;
	long			DAM;
	unsigned char	Density;
};

#if defined KEYBOARD_LEDS_SUPPORTED
/**
 */
typedef struct _KEYBOARD_INDICATOR_PARAMETERS 
{
    u16_t			UnitId;			// Unit identifier.
    u16_t			LedFlags;		// LED indicator state.	
} KEYBOARD_INDICATOR_PARAMETERS, *PKEYBOARD_INDICATOR_PARAMETERS;
#endif

typedef struct wd1793_t wd1793_t;

typedef unsigned char (*getbytefromdisk_t)(wd1793_t * pWD11793, unsigned char);
typedef unsigned char (*writebytetodisk_t)(wd1793_t * pWD11793, unsigned char);

/*****************************************************************************/

#define VCC_WD1793_ID	XFOURCC('1','7','9','3')

#define ASSERT_WD1793(pWD1793)  assert(pWD1793 != NULL && "NULL pointer"); \
                                assert(pWD1793->device.id == EMU_DEVICE_ID && "Invalid Emu device ID"); \
                                assert(pWD1793->device.idModule == VCC_WD1793_ID && "Invalid module ID");

/*****************************************************************************/

struct wd1793_t
{
	emudevice_t			device;
	
    cpu_t *             pCPU;
    
    DiskInfo			Drive[5];				// why 5?
	
	getbytefromdisk_t	GetBytefromDisk;
	writebytetodisk_t	WriteBytetoDisk;
	
    // settings
    bool                TurboMode;

    // timings
    int INDEXTIME;
    int WAITTIME;
    int STEPTIME;
    int SETTLETIME;

    // run-time
    // TODO: convert to int/bool/etc
    unsigned char		TransferBuffer[16384];	// TODO: allocate?
	unsigned char		StatusReg;
	unsigned char		DataReg;
	unsigned char		TrackReg;
	unsigned char		SectorReg;
	unsigned char		ControlReg;
	unsigned char		Side;
	unsigned char		CurrentCommand;
	unsigned char		CurrentDisk;
	unsigned char		LastDisk;
	unsigned char		MotorOn;
	unsigned char		InteruptEnable;
	unsigned char		HaltEnable;
	unsigned char		HeadLoad;
	unsigned char		TrackVerify;
	unsigned char		StepTimeMS;
	unsigned char		SideCompare;
	unsigned char		SideCompareEnable;
	unsigned char		Delay15ms;
	unsigned char		StepDirection;
	unsigned char		IndexPulse;
	unsigned char		CyclestoSettle;
	unsigned char		CyclesperStep;
	unsigned char		MSectorFlag; 
	unsigned char		LostDataFlag;
	int					ExecTimeWaiter;
	long				TransferBufferIndex;
	long				TransferBufferSize;
	unsigned int		IOWaiter;
	short				IndexCounter;

    bool                DirtyDisk;
    
#if defined KEYBOARD_LEDS_SUPPORTED
    unsigned char        KBLeds;
	KEYBOARD_INDICATOR_PARAMETERS	InputBuffer;		// Input buffer for DeviceIoControl
	KEYBOARD_INDICATOR_PARAMETERS	OutputBuffer;		// Output buffer for DeviceIoControl
	ULONG							DataLength;
	ULONG							ReturnedLength;		// Number of bytes returned in output buffer
	HANDLE							hKbdDev;	
#endif
	
#if (defined RAW_DRIVE_ACCESS_SUPPORTED)
    unsigned char        MSectorCount;
    unsigned char        UseLeds;
    void *                RawReadBuf;
    FD_READ_WRITE_PARAMS            rwp;
	unsigned char		PhysicalDriveA;
	unsigned char		PhysicalDriveB;
	unsigned char		OldPhysicalDriveA;
	unsigned char		OldPhysicalDriveB;
#endif	
} ;

/*****************************************************************************/

// ???
extern const char		ImageFormat[5][4];

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
    
	void			wd1793Init(wd1793_t * pWD1793);
	void			wd1793Reset(wd1793_t * pWD1793);

    unsigned char   wd1793GetTurboMode(wd1793_t * pWD1793);
    void            wd1793SetTurboMode(wd1793_t * pWD1793, bool enabled);
    
	int				wd1793MountDiskImage(wd1793_t * pWD1793, const char * pPathname, unsigned char drive);
	void			wd1793UnmountDiskImage(wd1793_t * pWD1793, unsigned char drive);
	
	unsigned char	wd1793IORead(wd1793_t * pWD1793, unsigned char port);
	void			wd1793IOWrite(wd1793_t * pWD1793, unsigned char port, unsigned char data);
	
	void			wd1793HeartBeat(wd1793_t * pWD1793);
    
#if defined KEYBOARD_LEDS_SUPPORTED
	unsigned char	UseKeyboardLeds(wd1793_t * pWD1793, unsigned char);
	int				CloseKeyboardDevice(wd1793_t * pWD1793, HANDLE);
	HANDLE			OpenKeyboardDevice(wd1793_t * pWD1793, int *);
#endif

#if (defined RAW_DRIVE_ACCESS_SUPPORTED)
    unsigned short  InitController(wd1793_t * pWD1793);
    unsigned long   UseRawDisk(wd1793_t * pWD1793, unsigned char,unsigned char);
    uint32_t        GetDriverVersion(wd1793_t * pWD1793);
#endif
    
    //void            DiskStatus(wd1793_t * pWD1793, char *);

#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif


