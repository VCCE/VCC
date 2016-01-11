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

/****************************************************************************
*	Technical specs on the Virtual Hard Disk interface
*
*	Address       Description
*	-------       -----------
*	FF80          Logical record number (high byte)
*	FF81          Logical record number (middle byte)
*	FF82          Logical record number (low byte)
*	FF83          Command/status register
*	FF84          Buffer address (high byte)
*	FF85          Buffer address (low byte)
*
*	Set the other registers, and then issue a command to FF83 as follows:
*
*	 0 = read 256-byte sector at LRN
*	 1 = write 256-byte sector at LRN
*	 2 = flush write cache (Closes and then opens the image file)
*		 Note: Vcc just issues a "FlushFileBuffers" command.
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
****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include "cc3vhd.h"
#include "harddisk.h"
#include "defines.h"
#include "..\fileops.h"

typedef union
{
	unsigned int All;
	struct
	{
		unsigned char lswlsb,lswmsb,mswlsb,mswmsb;
	} Byte;

} SECOFF;

typedef union
{
	unsigned int word;
	struct
	{
		unsigned char lsb,msb;
	} Byte;

} Address;

static HANDLE HardDrive=INVALID_HANDLE_VALUE;
static SECOFF SectorOffset;
static Address DMAaddress;
static unsigned char SectorBuffer[SECTORSIZE];
static unsigned char Mounted=0,WpHD=0;
static unsigned short ScanCount=0;
static unsigned long LastSectorNum=0;
static char DStatus[256]="";
static char Status = HD_PWRUP;
unsigned long BytesMoved=0;

void HDcommand(unsigned char);

int MountHD(char FileName[MAX_PATH])
{
	if (HardDrive!=INVALID_HANDLE_VALUE)	//Unmount any existing image
		UnmountHD();
	WpHD=0;
	Mounted=1;
	Status = HD_OK;
	SectorOffset.All = 0;
	DMAaddress.word = 0;
	HardDrive = CreateFile(FileName,GENERIC_READ | GENERIC_WRITE,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if (HardDrive == INVALID_HANDLE_VALUE)	//Can't open read/write. try read only
	{
		HardDrive = CreateFile(FileName,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
		WpHD=1;
	}
	if (HardDrive == INVALID_HANDLE_VALUE)	//Giving up
	{
		WpHD=0;
		Mounted=0;
		Status = HD_NODSK;
		return(0);
	}
	return(1);
}

void UnmountHD(void)
{
	if (HardDrive != INVALID_HANDLE_VALUE)
	{
		CloseHandle(HardDrive);
		HardDrive = INVALID_HANDLE_VALUE;
		Mounted = 0;
		Status = HD_NODSK;
	}
	return;
}

void HDcommand(unsigned char Command)
{
	unsigned short Temp=0;

	if (Mounted==0)
	{
		Status = HD_NODSK;
		return;
	}

	switch (Command)
	{
	case SECTOR_READ:	
		if (SectorOffset.All >MAX_SECTOR)
		{
			Status = HD_NODSK;
			return;
		}
		SetFilePointer(HardDrive,SectorOffset.All,0,FILE_BEGIN);
		ReadFile(HardDrive,SectorBuffer,SECTORSIZE,&BytesMoved,NULL);
		for (Temp=0; Temp < SECTORSIZE;Temp++)
			MemWrite(SectorBuffer[Temp],Temp+DMAaddress.word);
		Status = HD_OK;
		sprintf(DStatus,"HD: Rd Sec %000000.6X",SectorOffset.All>>8);
	break;

	case SECTOR_WRITE:
		if (WpHD == 1 )
		{
			Status = HD_WP;
			return;
		}

		if (SectorOffset.All >MAX_SECTOR)
		{
			Status = HD_NODSK;
			return;
		}
			
		for (Temp=0; Temp <SECTORSIZE;Temp++)
			SectorBuffer[Temp]=MemRead(Temp+DMAaddress.word);
		SetFilePointer(HardDrive,SectorOffset.All,0,FILE_BEGIN);
		WriteFile(HardDrive,SectorBuffer,SECTORSIZE,&BytesMoved,NULL);
		Status = HD_OK;
		sprintf(DStatus,"HD: Wr Sec %000000.6X",SectorOffset.All>>8);
	break;

	case DISK_FLUSH:
		FlushFileBuffers(HardDrive);
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
	if (SectorOffset.All != LastSectorNum)
	{
		ScanCount=0;
		LastSectorNum=SectorOffset.All;
	}
	if (ScanCount > 63)
	{
		ScanCount=0;
		if (Mounted==1)
			strcpy(DStatus,"HD:IDLE");
		else
			strcpy(DStatus,"HD:No Image!");
	}
	return;
}

void IdeWrite(unsigned char data,unsigned char port)
{
	port-=0x80;
	switch (port)
	{
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
	}
	return;
}

unsigned char IdeRead(unsigned char port)
{
	port-=0x80;
	switch (port)
	{
	case 0:
		return(SectorOffset.Byte.mswmsb); 
		break;
	case 1:
		return(SectorOffset.Byte.mswlsb);
		break;
	case 2:
		return(SectorOffset.Byte.lswmsb);
		break;
	case 3:
		return(Status);
		break;
	case 4:
		return(DMAaddress.Byte.msb);
		break;
	case 5:
		return(DMAaddress.Byte.lsb);
		break;
	}
	return(0);
}
