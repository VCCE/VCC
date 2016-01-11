/****************************************************************************/
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
/****************************************************************************/
/*	Technical specs on the Virtual Hard Disk interface
*	Use EmuDsk OS-9 Driver or integrated RGB-DOS
*
*	Address       Description
*	-------       -----------
*	FF80          Logical record number (high byte)
*	FF81          Logical record number (middle byte)
*	FF82          Logical record number (low byte)
*	FF83          Command/status register
*	FF84          Buffer address (high byte)
*	FF85          Buffer address (low byte)
*	FF86		  Controls .vhd drive 0=drive1 1=drive2
*
*	Set the other registers, and then issue a command to FF83 as follows:
*
*	 0 = read 256-byte sector at LRN
*	 1  = write 256-byte sector at LRN
*	 2 = flush write cache (Closes and then opens the image file)
*		 Note: Vcc just issues a "FlushFileBuffers" command.
*
*	Error values:
*
*	 0 = no error
*	-1 = power-on state (before the first command is recieved)
*	-2 = invalid command
*	 2 = VHD image does not exist
*	 4 = Unable to open VHD image file
*	 5 = access denied (may not be able to write to VHD image)
*
*	IMPORTANT: The I/O buffer must NOT cross an 8K MMU bank boundary.
*	Note: This is not an issue for Vcc.
*/
/****************************************************************************/

#include "cc3vhd.h"
#include "harddisk.h"

#include <windows.h>
#include <stdio.h>
#include <assert.h>

/****************************************************************************/

typedef union SECOFF
{
	unsigned int All;
	struct
	{
		unsigned char lswlsb,lswmsb,mswlsb,mswmsb;
	} Byte;

} SECOFF;

/****************************************************************************/

typedef union Address
{
	unsigned int word;
	struct
	{
		unsigned char lsb,msb;
	} Byte;

} Address;

/****************************************************************************/

typedef struct _vhd_t_
{
	HANDLE			hHardDrive;
	unsigned char	SectorBuffer[SECTORSIZE];
	SECOFF			SectorOffset;
	Address			DMAaddress;
	unsigned char	Mounted;
	unsigned char	WpHD;
	unsigned long	LastSectorNum;
	char			Status;
	unsigned long	BytesMoved;
} vhd_t;

/****************************************************************************/

// we can have two hard drives
vhd_t g_vhd[2] =	
{
	{ INVALID_HANDLE_VALUE },
	{ INVALID_HANDLE_VALUE }
};

static int				g_iCurDrive		= 0;
static char				DStatus[128]	= "";
static unsigned short	ScanCount;

void HDcommand(unsigned char);

int MountHD(char * FileName, int iDiskNum)
{
	if ( g_vhd[iDiskNum].hHardDrive != INVALID_HANDLE_VALUE )	
	{
		// Unmount any existing image
		UnmountHD(iDiskNum);
	}
	 g_vhd[iDiskNum].Status		= HD_PWRUP;
	 g_vhd[iDiskNum].hHardDrive	= INVALID_HANDLE_VALUE;

	 g_vhd[iDiskNum].WpHD=0;
	 g_vhd[iDiskNum].Mounted=1;
	 g_vhd[iDiskNum].Status = HD_OK;
	 g_vhd[iDiskNum].SectorOffset.All = 0;
	 g_vhd[iDiskNum].DMAaddress.word = 0;

	g_vhd[iDiskNum].hHardDrive = CreateFile(FileName,GENERIC_READ | GENERIC_WRITE,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if ( g_vhd[iDiskNum].hHardDrive == INVALID_HANDLE_VALUE )	//Can't open read/write. try read only
	{
		g_vhd[iDiskNum].hHardDrive = CreateFile(FileName,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
		g_vhd[iDiskNum].WpHD=1;
	}
	if ( g_vhd[iDiskNum].hHardDrive == INVALID_HANDLE_VALUE )	//Giving up
	{
		g_vhd[iDiskNum].WpHD=0;
		g_vhd[iDiskNum].Mounted=0;
		g_vhd[iDiskNum].Status = HD_NODSK;

		return(0);
	}

	return (1);
}

void UnmountHD(int iDiskNum)
{
	if ( g_vhd[iDiskNum].hHardDrive != INVALID_HANDLE_VALUE )
	{
		CloseHandle(g_vhd[iDiskNum].hHardDrive);
		g_vhd[iDiskNum].hHardDrive	= INVALID_HANDLE_VALUE;
		g_vhd[iDiskNum].Mounted		= 0;
		g_vhd[iDiskNum].Status		= HD_NODSK;
	}
}

void HDcommand(unsigned char Command)
{
	unsigned short	Temp		= 0;
	int				iDiskNum	= g_iCurDrive;

	if ( g_vhd[iDiskNum].Mounted == 0 )
	{
		g_vhd[iDiskNum].Status = HD_NODSK;
		return;
	}

	switch (Command)
	{
	case SECTOR_READ:	
		if ( g_vhd[iDiskNum].SectorOffset.All > MAX_SECTOR )
		{
			g_vhd[iDiskNum].Status = HD_NODSK;
			return;
		}
		SetFilePointer(g_vhd[iDiskNum].hHardDrive,g_vhd[iDiskNum].SectorOffset.All,0,FILE_BEGIN);
		ReadFile(g_vhd[iDiskNum].hHardDrive,g_vhd[iDiskNum].SectorBuffer,SECTORSIZE,&g_vhd[iDiskNum].BytesMoved,NULL);
		for (Temp=0; Temp < SECTORSIZE && Temp < g_vhd[iDiskNum].BytesMoved; Temp++)
		{
			MemWrite(g_vhd[iDiskNum].SectorBuffer[Temp],Temp+g_vhd[iDiskNum].DMAaddress.word);
		}
		g_vhd[iDiskNum].Status = HD_OK;
		sprintf(DStatus,"HD%d: Rd Sec %000000.6X",iDiskNum,g_vhd[iDiskNum].SectorOffset.All>>8);
	break;

	case SECTOR_WRITE:
		if ( g_vhd[iDiskNum].WpHD == 1 )
		{
			g_vhd[iDiskNum].Status = HD_WP;
			return;
		}

		if ( g_vhd[iDiskNum].SectorOffset.All > MAX_SECTOR )
		{
			g_vhd[iDiskNum].Status = HD_NODSK;
			return;
		}
			
		for (Temp=0; Temp <SECTORSIZE;Temp++)
		{
			g_vhd[iDiskNum].SectorBuffer[Temp] = MemRead(Temp+g_vhd[iDiskNum].DMAaddress.word);
		}
		SetFilePointer(g_vhd[iDiskNum].hHardDrive,g_vhd[iDiskNum].SectorOffset.All,0,FILE_BEGIN);
		WriteFile(g_vhd[iDiskNum].hHardDrive,g_vhd[iDiskNum].SectorBuffer,SECTORSIZE,&g_vhd[iDiskNum].BytesMoved,NULL);
		g_vhd[iDiskNum].Status = HD_OK;
		sprintf(DStatus,"HD%d: Wr Sec %000000.6X",iDiskNum,g_vhd[iDiskNum].SectorOffset.All>>8);
	break;

	case DISK_FLUSH:
		FlushFileBuffers(g_vhd[iDiskNum].hHardDrive);
		g_vhd[iDiskNum].SectorOffset.All=0;
		g_vhd[iDiskNum].DMAaddress.word=0;
		g_vhd[iDiskNum].Status = HD_OK;
	break;

	default:
		g_vhd[iDiskNum].Status = HD_INVLD;
	return;
	}
}

void DiskStatus(char *Temp)
{
	int iDiskNum = g_iCurDrive;

	strcpy(Temp,DStatus);
	ScanCount++;
	if ( g_vhd[iDiskNum].SectorOffset.All != g_vhd[iDiskNum].LastSectorNum )
	{
		ScanCount = 0;
		g_vhd[iDiskNum].LastSectorNum = g_vhd[iDiskNum].SectorOffset.All;
	}
	if  ( ScanCount > 63 )
	{
		ScanCount = 0;
		if ( g_vhd[iDiskNum].Mounted == 1 )
			strcpy(DStatus,"HD:IDLE");
		else
			strcpy(DStatus,"HD:No Image!");
	}
}

void IdeWrite(unsigned char data, unsigned char port)
{
	int iDiskNum = g_iCurDrive;

	port -= EMUHDD_PORT;
	switch (port)
	{
	case 0:
		g_vhd[iDiskNum].SectorOffset.Byte.mswmsb = data;
		break;
	case 1:
		g_vhd[iDiskNum].SectorOffset.Byte.mswlsb = data;
		break;
	case 2:
		g_vhd[iDiskNum].SectorOffset.Byte.lswmsb = data;
		break;
	case 3:
		HDcommand(data);
		break;
	case 4:
		g_vhd[iDiskNum].DMAaddress.Byte.msb = data;
		break;
	case 5:
		g_vhd[iDiskNum].DMAaddress.Byte.lsb = data;
		break;
	case 6:
		g_iCurDrive = (data!=0);
	}
}

unsigned char IdeRead(unsigned char port)
{
	int iDiskNum = g_iCurDrive;

	port -= EMUHDD_PORT;
	switch (port)
	{
	case 0:
		return(g_vhd[iDiskNum].SectorOffset.Byte.mswmsb); 
		break;
	case 1:
		return(g_vhd[iDiskNum].SectorOffset.Byte.mswlsb);
		break;
	case 2:
		return(g_vhd[iDiskNum].SectorOffset.Byte.lswmsb);
		break;
	case 3:
		return (g_vhd[iDiskNum].Status);
		break;
	case 4:
		return (g_vhd[iDiskNum].DMAaddress.Byte.msb);
		break;
	case 5:
		return (g_vhd[iDiskNum].DMAaddress.Byte.lsb);
		break;
	case 6:
		return iDiskNum;
		break;
	}
	return(0);
}
