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

/*******************************************************************************/
/*
	File name: wd1793.c
	Copyright 2005 by Joseph Forgione
	
	This will Emulate the Western Digital wd1793 Floppy Disk Controller chip
	and support circuts as implemented in the Tandy Color Computer 1, 2, and 3.
*/
/*******************************************************************************/
/*
	Last change date 03/16/2005				
	Last change date 07/14/2005 Changes NMI assert fucntion
	Last change date 07/26/2005 Complete rewrite to get nitros09 to boot
	Last change date 02/04/2006 rewritten again for DMK and writetrack support
	Last change date 06/27/2006 seek time more closely emulated
	Last change date 05/16/2007 Better DMK support. Diecom protection works now
	Last change date 05/21/2007 Added fdrawcmd support for real disk access
 
	Branched at v1.41
	made cross platform
	changed plug-in API to match updated VCC
*/
/*******************************************************************************/
/*
 TODO: Change port read and write so WD1793 register block can be remapped easily
 TODO: Restore RAW drive access for Windows, add raw drive access for *nix
*/
/*******************************************************************************/

#include "wd1793.h"

#include "fd502.h"
#include "citt_crc16.h"

#include "coco3.h"
#include "file.h"

#include "xDebug.h"

#ifdef WIN32
#	include "fdrawcmd.h"	// http://simonowen.com/fdrawcmd/
#endif

#include <stdio.h>
#include <assert.h>

/*****************************************************************************/

const unsigned char		StepTimesMS[4]		= {6,12,20,30};
const unsigned short	BytesperSector[4]	= {128,256,512,1024};
const char				ImageFormat[5][4]	= {"JVC","VDK","DMK","OS9","RAW"};

/*******************************************************************************/
/*
	Fuction Protos for this Module
 */

unsigned char	GetBytefromSector (wd1793_t * pWD1793, unsigned char);
unsigned char	GetBytefromAddress(wd1793_t * pWD1793, unsigned char);
unsigned char	GetBytefromTrack  (wd1793_t * pWD1793, unsigned char);

unsigned char	WriteBytetoSector (wd1793_t * pWD1793, unsigned char);
unsigned char	WriteBytetoTrack  (wd1793_t * pWD1793, unsigned char);

unsigned char	MountDisk(wd1793_t * pWD1793, const char * pPathname, unsigned char);
void			DispatchCommand(wd1793_t * pWD1793, unsigned char);
void			DecodeControlReg(wd1793_t * pWD1793, unsigned char);
void			SetType1Flags(wd1793_t * pWD1793, unsigned char);
void			SetType2Flags(wd1793_t * pWD1793, unsigned char);
void			SetType3Flags(wd1793_t * pWD1793, unsigned char);

long			ReadSector (wd1793_t * pWD1793, unsigned char,unsigned char,unsigned char,unsigned char *);
long			WriteSector(wd1793_t * pWD1793, unsigned char,unsigned char,unsigned char,unsigned char *,long);
long			ReadTrack  (wd1793_t * pWD1793, unsigned char,unsigned char,unsigned char,unsigned char *);
long			WriteTrack (wd1793_t * pWD1793, unsigned char,unsigned char,unsigned char,unsigned char *);

long			GetSectorInfo (wd1793_t * pWD1793, SectorInfo *, unsigned char *);
void			CommandDone(wd1793_t * pWD1793);

#if (defined RAW_DRIVE_ACCESS_SUPPORTED)
bool_t			FormatTrack (wd1793_t * pWD1793, void * , BYTE , BYTE,BYTE );
bool_t			CmdFormat (wd1793_t * pWD1793, HANDLE , PFD_FORMAT_PARAMS , ULONG );
void *			OpenFloppy (wd1793_t * pWD1793, int );
bool_t			SetDataRate (wd1793_t * pWD1793, HANDLE , BYTE );
#endif


/****************************************************************
*	7 halt enable flag											*
*	6 drive select #3 or SideSelect								*	
*	5 density (0=single, 1=double)  and NMI enable flag			*
*	4 write precompensation										*	
*	3 drive motor activation									*	
*	2 drive select #2											*
*	1 drive select #1											*
*	0 drive select #0											*
*																*
* Reading from $ff48-$ff4f clears bit 7 of DSKREG ($ff40)		*
*																*
****************************************************************/

void DecodeControlReg(wd1793_t * pWD1793, unsigned char Tmp)
{
	pWD1793->MotorOn=0;
	pWD1793->CurrentDisk=NONE;
	pWD1793->Side=0;
	pWD1793->InteruptEnable=0;
	pWD1793->HaltEnable=0;

#if defined KEYBOARD_LEDS_SUPPORTED
	pWD1793->KBLeds=0;
#endif
	
	if (Tmp & CTRL_DRIVE0)
	{
		pWD1793->CurrentDisk=0;
        
#if defined KEYBOARD_LEDS_SUPPORTED
		pWD1793->KBLeds|=KEYBOARD_NUM_LOCK_ON;
#endif
	}
    
	if (Tmp & CTRL_DRIVE1)
	{
		pWD1793->CurrentDisk=1;
        
#if defined KEYBOARD_LEDS_SUPPORTED
		pWD1793->KBLeds|=KEYBOARD_CAPS_LOCK_ON;
#endif
	}
    
	if (Tmp & CTRL_DRIVE2)
	{
		pWD1793->CurrentDisk=2;
        
#if defined KEYBOARD_LEDS_SUPPORTED
		pWD1793->KBLeds|=KEYBOARD_SCROLL_LOCK_ON;
#endif
	}
	
	if (Tmp & SIDESELECT)	//DRIVE3 Select in "all single sided" systems
		pWD1793->Side=1;
    
	if (Tmp & CTRL_MOTOR_EN)
	{
		pWD1793->MotorOn = 1;
		
#if defined(RAW_DRIVE_ACCESS_SUPPORTED)
		if ( pWD1793->Drive[CurrentDisk].ImageType == RAW )
		{
			DeviceIoControl(pWD1793->Drive[CurrentDisk].FileHandle, IOCTL_FD_MOTOR_ON, NULL, NULL, NULL, 0, &dwRet, NULL);
		}
#endif
	}

	if ( (pWD1793->Side==1) & (pWD1793->CurrentDisk==NONE) )
	{
		pWD1793->CurrentDisk=3;
		pWD1793->Side=0;
	}
	if ( !(Tmp & CTRL_MOTOR_EN))	//Turning off the drive makes the disk dirty
		pWD1793->DirtyDisk=1;

	if (pWD1793->LastDisk != pWD1793->CurrentDisk)	//If we switch from reading one Physical disk to another we need to invalidate the cache
		pWD1793->DirtyDisk=1;
	pWD1793->LastDisk=pWD1793->CurrentDisk;
	if (Tmp & CTRL_DENSITY)	//Strange, Density and Interupt enable flag
		pWD1793->InteruptEnable=1;
	if (Tmp & CTRL_HALT_FLAG)
		pWD1793->HaltEnable=1;
#if defined(KEYBOARD_LEDS_SUPPORTED)
	InputBuffer.LedFlags=KBLeds ;

	if (UseLeds)
	{
		if (hKbdDev==NULL)
			hKbdDev=OpenKeyboardDevice(NULL);
		DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_SET_INDICATORS,&InputBuffer, DataLength,NULL,0,&ReturnedLength, NULL);
	}
#endif
}

/********************************************************************************************
* ReadSector returns only the valid sector data with out the low level housekeeping info	*																											 *
* Should work for all formats																*
********************************************************************************************/

long ReadSector (wd1793_t * pWD1793, 
				 unsigned char Side,	//0 or 1
				 unsigned char Track,	//0 to 255 "REAL" values are 1 to 80
				 unsigned char Sector,	//1 to 18 could be 0 to 17
				 unsigned char *ReturnBuffer)
{
	unsigned long BytesRead=0;
	//unsigned long Result=0;
	//unsigned long SectorLenth=0;
	//unsigned short IdamIndex=0;
	//unsigned short Temp1=0;
	long FileOffset=0;
	//unsigned char Density=0;
	unsigned char TempBuffer[16384];
	SectorInfo CurrentSector;
#if (defined RAW_DRIVE_ACCESS_SUPPORTED)
//Needed for RAW access
	uint32_t dwRet;
	FD_SEEK_PARAMS sp;
	unsigned char Ret=0;
	unsigned char *pva=NULL;
#endif
	
	if (pWD1793->Drive[pWD1793->CurrentDisk].FileHandle==NULL)
	{
		return(0);
	}
	
	switch ( pWD1793->Drive[pWD1793->CurrentDisk].ImageType )
	{
		case JVC:
		case VDK:
		case OS9:
		{
			if ( ((pWD1793->Side+1) > pWD1793->Drive[pWD1793->CurrentDisk].Sides) | (pWD1793->TrackReg != pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition) )
			{
				return(0);
			}
			
			FileOffset = pWD1793->Drive[pWD1793->CurrentDisk].HeaderSize + ( (Track * pWD1793->Drive[pWD1793->CurrentDisk].Sides * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize)+ (Side * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize) + (BytesperSector[pWD1793->Drive[pWD1793->CurrentDisk].SectorSize] * (Sector - pWD1793->Drive[pWD1793->CurrentDisk].FirstSector) ) ) ;
			fseek(pWD1793->Drive[pWD1793->CurrentDisk].FileHandle, FileOffset, SEEK_SET);
            BytesRead = fread(ReturnBuffer, 1, BytesperSector[pWD1793->Drive[pWD1793->CurrentDisk].SectorSize], pWD1793->Drive[pWD1793->CurrentDisk].FileHandle);
			if ( BytesRead != BytesperSector[pWD1793->Drive[pWD1793->CurrentDisk].SectorSize] ) //Fake the read for short images
			{
				memset(ReturnBuffer,0xFF,BytesperSector[pWD1793->Drive[pWD1793->CurrentDisk].SectorSize]); 
                BytesRead = BytesperSector[pWD1793->Drive[pWD1793->CurrentDisk].SectorSize];
			}
			
			return (BytesperSector[pWD1793->Drive[pWD1793->CurrentDisk].SectorSize]);
		}
		break;

		case DMK:	
			FileOffset = pWD1793->Drive[pWD1793->CurrentDisk].HeaderSize + ( (Track * pWD1793->Drive[pWD1793->CurrentDisk].Sides * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize)+ (Side * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize));
			fseek(pWD1793->Drive[pWD1793->CurrentDisk].FileHandle, FileOffset, SEEK_SET);
			
			//Need to read the entire track due to the emulation of interleave on these images
            BytesRead = fread(TempBuffer, 1, pWD1793->Drive[pWD1793->CurrentDisk].TrackSize, pWD1793->Drive[pWD1793->CurrentDisk].FileHandle);
			//DMK can't be short the image is corupt
			if ( BytesRead != pWD1793->Drive[pWD1793->CurrentDisk].TrackSize ) 
			{
					return (0);
			}
			CurrentSector.Sector=Sector;
			if ( GetSectorInfo(pWD1793,&CurrentSector,TempBuffer) == 0 )
			{
				return(0);
			}
			
			memcpy(ReturnBuffer,&TempBuffer[CurrentSector.DAM],CurrentSector.Length);
			
			return(CurrentSector.Length);
		break;

#if (defined RAW_DRIVE_ACCESS_SUPPORTED)
		case RAW:
			pva=(unsigned char *)RawReadBuf;
			if (pWD1793->TrackReg != pWD1793->Drive[CurrentDisk].HeadPosition)
				return(0);
			//Read the entire track and cache it. Speeds up disk reads 
			if ((DirtyDisk) | (rwp.phead != Side) | (rwp.cyl != pWD1793->Drive[CurrentDisk].HeadPosition ) )
			{
				rwp.flags = FD_OPTION_MFM;
				rwp.phead = Side;
				rwp.cyl = pWD1793->Drive[CurrentDisk].HeadPosition;
				rwp.head = Side;
				rwp.sector = 1;
				rwp.size = 1;	//256 Byte Secotors
				rwp.eot = 1+18;
				rwp.gap = 0x0a;
				rwp.datalen = 0xff;
				// details of seek location
				sp.cyl = Track;
				sp.head = Side;
				// seek to cyl 
				DeviceIoControl(pWD1793->Drive[CurrentDisk].FileHandle , IOCTL_FDCMD_SEEK, &sp, sizeof(sp), NULL, 0, &dwRet, NULL);
				Ret=DeviceIoControl(pWD1793->Drive[CurrentDisk].FileHandle , IOCTL_FDCMD_READ_DATA, &rwp, sizeof(rwp), RawReadBuf,4608, &dwRet, NULL);
				if (dwRet != 4608)
					return(0);
				DirtyDisk=0;
			}
			memcpy(ReturnBuffer,&pva[(Sector-1)*256],256);
			return(256);
		break;
#endif
	}
	return(0);
}

/**********************************************************/
/**
 */
long WriteSector (	wd1793_t * pWD1793, 
					unsigned char Side,		//0 or 1
					unsigned char Track,	//0 to 255 "REAL" values are 1 to 80
					unsigned char Sector,	//1 to 18 could be 0 to 17
					unsigned char *WriteBuffer, //)
					long BytestoWrite)
{
	unsigned long BytesWritten=0;
	//unsigned long Result=0;
	unsigned long BytesRead=0;
	//unsigned short IdamIndex=0;
	//unsigned short Temp1=0;
	//unsigned short Temp2=0;
	unsigned int FileOffset=0;
	//unsigned char Density=0;
	unsigned char TempBuffer[16384];
	unsigned short Crc=0xABCD;
#if (defined RAW_DRIVE_ACCESS_SUPPORTED)
//Needed for RAW access
	u32_t dwRet;
	FD_SEEK_PARAMS sp;
	//unsigned char Ret=0;
	//unsigned char *pva=NULL;
#endif
	SectorInfo CurrentSector;
	
	if ( (pWD1793->Drive[pWD1793->CurrentDisk].FileHandle==NULL) | ((pWD1793->Side+1) > pWD1793->Drive[pWD1793->CurrentDisk].Sides) )
    {
		return(0);
    }
    
	switch ( pWD1793->Drive[pWD1793->CurrentDisk].ImageType )
	{
		case JVC:
		case VDK:
		case OS9:
			FileOffset = pWD1793->Drive[pWD1793->CurrentDisk].HeaderSize + ( (Track * pWD1793->Drive[pWD1793->CurrentDisk].Sides * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize)+ (Side * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize) + (BytesperSector[pWD1793->Drive[pWD1793->CurrentDisk].SectorSize] * (Sector - pWD1793->Drive[pWD1793->CurrentDisk].FirstSector) ) ) ;
			fseek(pWD1793->Drive[pWD1793->CurrentDisk].FileHandle,FileOffset, SEEK_SET);
            BytesWritten = fwrite(WriteBuffer, 1, BytesperSector[pWD1793->Drive[pWD1793->CurrentDisk].SectorSize], pWD1793->Drive[pWD1793->CurrentDisk].FileHandle);
			return(BytesWritten);
		break;

		case DMK:	//DMK 
			FileOffset= pWD1793->Drive[pWD1793->CurrentDisk].HeaderSize + ( (Track * pWD1793->Drive[pWD1793->CurrentDisk].Sides * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize)+ (Side * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize));
			fseek(pWD1793->Drive[pWD1793->CurrentDisk].FileHandle, FileOffset, SEEK_SET);
			// Need to read the entire track due to the emulation of interleave on these images			
            BytesRead = fread(TempBuffer, 1, pWD1793->Drive[pWD1793->CurrentDisk].TrackSize, pWD1793->Drive[pWD1793->CurrentDisk].FileHandle);
			if ( BytesRead != pWD1793->Drive[pWD1793->CurrentDisk].TrackSize )
			{
				return (0);
			}
			
			CurrentSector.Sector=Sector;
			if ( GetSectorInfo(pWD1793,&CurrentSector,TempBuffer) == 0 )
			{
				return(0);
			}
			memcpy(&TempBuffer[CurrentSector.DAM],WriteBuffer,BytestoWrite);
			Crc = ccitt_crc16(0xE295,&TempBuffer[CurrentSector.DAM], CurrentSector.Length); //0xcdb4
			TempBuffer[CurrentSector.DAM + CurrentSector.Length  ] =Crc>>8;
			TempBuffer[CurrentSector.DAM + CurrentSector.Length +1 ] =(Crc & 0xFF);
            FileOffset = pWD1793->Drive[pWD1793->CurrentDisk].HeaderSize + ( (Track * pWD1793->Drive[pWD1793->CurrentDisk].Sides * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize)+ (Side * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize));
			fseek(pWD1793->Drive[pWD1793->CurrentDisk].FileHandle, FileOffset, SEEK_SET);
            /* BytesWritten = */ fwrite(TempBuffer, 1, pWD1793->Drive[pWD1793->CurrentDisk].TrackSize, pWD1793->Drive[pWD1793->CurrentDisk].FileHandle);
			return (CurrentSector.Length);
		break;

#if (defined RAW_DRIVE_ACCESS_SUPPORTED)
		case RAW:
			DirtyDisk=1;
			pva=(unsigned char *) RawReadBuf;
			rwp.flags = FD_OPTION_MFM;
			rwp.phead = Side;
			rwp.cyl = Drive[CurrentDisk].HeadPosition;
			rwp.head = Side;
			rwp.sector = Sector;
			rwp.size = 1;	//256 Byte Secotors
			rwp.eot = 1+Sector;
			rwp.gap = 0x0a;
			rwp.datalen = 0xff;
			// details of seek location
			sp.cyl = Track;
			sp.head = Side;
			// seek to cyl
			DeviceIoControl(Drive[CurrentDisk].FileHandle , IOCTL_FDCMD_SEEK, &Track, sizeof(Track), NULL, 0, &dwRet, NULL);
			memcpy(RawReadBuf,WriteBuffer,256);
			Ret = DeviceIoControl(Drive[CurrentDisk].FileHandle , IOCTL_FDCMD_WRITE_DATA, &rwp, sizeof(rwp), RawReadBuf,18*(128<<rwp.size), &dwRet, NULL);
			return(dwRet);
		break;
#endif
	}
	return(0);
}

/**********************************************************/
/**
 */
long WriteTrack (	wd1793_t * pWD1793, 
					unsigned char Side,		//0 or 1
					unsigned char Track,	//0 to 255 "REAL" values are 1 to 80
					unsigned char Dummy,	//Sector Value unused
					unsigned char *WriteBuffer)
{
    unsigned char xTrack=0;
    unsigned char xSide=0;
    unsigned char xSector=0;
    unsigned char xLenth=0;
	unsigned short BufferIndex=0,WriteIndex=0,IdamIndex=0;
	unsigned long FileOffset=0;
	//unsigned long Result=0;
	unsigned long BytesWritten=0;
	unsigned char *TempBuffer=NULL;
	//unsigned short TBufferIndex=0;
	//unsigned short IdamPointer=0;
	unsigned char TempChar=0;
	//unsigned char WriteToggle=0;
	unsigned short Crc=0,DataBlockPointer=0,Seed=0;	

	if ( pWD1793->Drive[pWD1793->CurrentDisk].FileHandle == NULL )
	{
		return(0);
	}
	
	switch ( pWD1793->Drive[pWD1793->CurrentDisk].ImageType )
	{
		case JVC:
		case VDK:
		case OS9:
			for (BufferIndex=0; BufferIndex<6272; BufferIndex++)
			{
				if ( WriteBuffer[BufferIndex] == 0xFE) //Look for ID.A.M.
				{
					xTrack	= WriteBuffer[BufferIndex+1];
					xSide	= WriteBuffer[BufferIndex+2];
					xSector = WriteBuffer[BufferIndex+3];
					xLenth	= WriteBuffer[BufferIndex+4];
					BufferIndex += 6;
				}

				if ( WriteBuffer[BufferIndex] == 0xFB)	//Look for D.A.M.
				{
					BufferIndex++;
					FileOffset= pWD1793->Drive[pWD1793->CurrentDisk].HeaderSize + ( (Track * pWD1793->Drive[pWD1793->CurrentDisk].Sides * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize)+ (Side * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize) + (BytesperSector[pWD1793->Drive[pWD1793->CurrentDisk].SectorSize] * (xSector - 1) ) ) ;
					fseek(pWD1793->Drive[pWD1793->CurrentDisk].FileHandle, FileOffset, SEEK_SET);
                    BytesWritten = fwrite(&WriteBuffer[BufferIndex], 1, BytesperSector[pWD1793->Drive[pWD1793->CurrentDisk].SectorSize], pWD1793->Drive[pWD1793->CurrentDisk].FileHandle);
                    
					BufferIndex += BytesperSector[pWD1793->Drive[pWD1793->CurrentDisk].SectorSize];
				}
			}
		break;

		case DMK:
			BufferIndex=128;	//Index to cooked buffer
			WriteIndex=0;		// Index to raw Buffer

			TempBuffer=(unsigned char *)malloc(pWD1793->Drive[pWD1793->CurrentDisk].TrackSize);
			memset(TempBuffer,0,pWD1793->Drive[pWD1793->CurrentDisk].TrackSize);
			while ( BufferIndex < pWD1793->Drive[pWD1793->CurrentDisk].TrackSize )
			{
				TempChar=WriteBuffer[WriteIndex++];
				if (TempChar == 0xF7)			//This is working now
				{
					Crc = ccitt_crc16(Seed, &TempBuffer[DataBlockPointer],BufferIndex - DataBlockPointer );
					TempBuffer[BufferIndex++]= (Crc >>8);
					TempBuffer[BufferIndex++]= (Crc & 0xFF);
				}
				else
				{
					TempBuffer[BufferIndex]=TempChar;
					if (TempChar == 0xFE) //ID Address mark Build Track Header Pointers
					{
						TempBuffer[IdamIndex++]=  (BufferIndex & 0xFF);
						TempBuffer[IdamIndex++]= 0x80 |(BufferIndex>>8);
					}
					if (TempChar == 0xFB)	//data Address Marker
					{
						DataBlockPointer = BufferIndex+1;
						Seed = 0xE295;
					}
					if (TempChar == 0xFE)	//ID address marker
					{
						DataBlockPointer = BufferIndex;    
						Seed = 0xCDB4;
					}
					
					if (TempChar == 0xF5)
						TempBuffer[BufferIndex]=0xA1;
					if (TempChar == 0xF6)
						TempBuffer[BufferIndex]=0xC2;
					BufferIndex++;
				}	
			}

			FileOffset= pWD1793->Drive[pWD1793->CurrentDisk].HeaderSize + ( (Track * pWD1793->Drive[pWD1793->CurrentDisk].Sides * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize)+ (Side * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize)  ) ;
			fseek(pWD1793->Drive[pWD1793->CurrentDisk].FileHandle, FileOffset, SEEK_SET);
            BytesWritten = fwrite(TempBuffer, 1, pWD1793->Drive[pWD1793->CurrentDisk].TrackSize, pWD1793->Drive[pWD1793->CurrentDisk].FileHandle);
			
			free(TempBuffer);
		break;

#if (defined RAW_DRIVE_ACCESS_SUPPORTED)
		case RAW:
			DirtyDisk=1;
			DeviceIoControl(pWD1793->Drive[pWD1793->CurrentDisk].FileHandle , IOCTL_FDCMD_SEEK, &Track, sizeof(Track), NULL, 0, &dwRet, NULL);
			return(FormatTrack (pWD1793->Drive[pWD1793->CurrentDisk].FileHandle , Track , Side, WriteBuffer[100] )); //KLUDGE!

		break;
#endif
	}
	
	return(BytesWritten);
}

/**********************************************************/
/**
 */
long ReadTrack (	wd1793_t * pWD1793, 
					unsigned char Side,		//0 or 1
					unsigned char Track,	//0 to 255 "REAL" values are 1 to 80
					unsigned char Dummy,	//Sector Value unused
					unsigned char *WriteBuffer)
{
	unsigned long BytesRead=0;
	//unsigned long Result=0;
	long FileOffset=0;
	//unsigned char Density=0;

	if (pWD1793->Drive[pWD1793->CurrentDisk].FileHandle==NULL)
		return(0);

	switch (pWD1793->Drive[pWD1793->CurrentDisk].ImageType)
	{
		case JVC:
		case VDK:
		case OS9:
			if ((Side+1) > pWD1793->Drive[pWD1793->CurrentDisk].Sides)
				return(0);
			//STUB Write Me
			return(0);
		break;

		case DMK:	
			FileOffset = pWD1793->Drive[pWD1793->CurrentDisk].HeaderSize + ( (Track * pWD1793->Drive[pWD1793->CurrentDisk].Sides * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize)+ (Side * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize)+128);
			fseek(pWD1793->Drive[pWD1793->CurrentDisk].FileHandle, FileOffset, SEEK_SET);
            BytesRead = fread(WriteBuffer, 1, (pWD1793->Drive[pWD1793->CurrentDisk].TrackSize-128), pWD1793->Drive[pWD1793->CurrentDisk].FileHandle);
			if ( BytesRead != ((unsigned)pWD1793->Drive[pWD1793->CurrentDisk].TrackSize-128) )
			{
				return (0);
			}
		break;
	}
	return BytesRead;
}

/**********************************************************/

void DispatchCommand(wd1793_t * pWD1793, unsigned char Tmp)
{
	unsigned char Command= (Tmp >>4);
	
	if (   (pWD1793->CurrentCommand !=IDLE) 
		 & (Command != 13) 
	   )
	{
		return;
	}
	
	pWD1793->CurrentCommand=Command;
	
	switch (Command)
	{
		case RESTORE:	
			SetType1Flags(pWD1793,Tmp);
			pWD1793->ExecTimeWaiter = pWD1793->CyclestoSettle + (pWD1793->CyclesperStep * pWD1793->StepTimeMS);// * pWD1793->Drive[pWD1793->CurrentDrive].HeadPosition);
			pWD1793->StatusReg= BUSY;
//			WriteLog("RESTORE",0);
			break;

		case SEEK:	
			SetType1Flags(pWD1793,Tmp);
			pWD1793->ExecTimeWaiter = pWD1793->CyclestoSettle + (pWD1793->CyclesperStep * pWD1793->StepTimeMS);// * Diff(pWD1793->Drive[pWD1793->CurrentDrive].HeadPosition,DataReg));
			pWD1793->TrackReg=pWD1793->DataReg;
			pWD1793->StatusReg= BUSY;
			if ( pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition > pWD1793->DataReg)
				pWD1793->StepDirection=0;
			else
				pWD1793->StepDirection=1;
//			WriteLog("SEEK",0);
			break;

		case STEP:
		case STEPUPD:	
			SetType1Flags(pWD1793,Tmp);
			pWD1793->ExecTimeWaiter= pWD1793->CyclestoSettle + (pWD1793->CyclesperStep * pWD1793->StepTimeMS);
			pWD1793->StatusReg= BUSY;
	//		WriteLog("STEP",0);
			break;

		case STEPIN:
		case STEPINUPD:	
			SetType1Flags(pWD1793,Tmp);
			pWD1793->ExecTimeWaiter= pWD1793->CyclestoSettle + (pWD1793->CyclesperStep * pWD1793->StepTimeMS);
			pWD1793->StatusReg= BUSY;
//			WriteLog("STEPIN",0);
			break;

		case STEPOUT:
		case STEPOUTUPD:	
			SetType1Flags(pWD1793,Tmp);
			pWD1793->ExecTimeWaiter= pWD1793->CyclestoSettle + (pWD1793->CyclesperStep * pWD1793->StepTimeMS);
			pWD1793->StatusReg= BUSY;
//			WriteLog("STEPOUT",0);
			break;

		case READSECTOR:
		case READSECTORM:
			SetType2Flags(pWD1793,Tmp);
			pWD1793->TransferBufferIndex=0;
			pWD1793->StatusReg= BUSY | DRQ;
			pWD1793->GetBytefromDisk=&GetBytefromSector;
			pWD1793->ExecTimeWaiter= 1;
			pWD1793->IOWaiter=0;
			pWD1793->MSectorFlag=Command & 1;
//			WriteLog("READSECTOR",0);
			break;

		case WRITESECTOR:
		case WRITESECTORM: 
//			WriteLog("Getting WriteSector Command",0);
			SetType2Flags(pWD1793,Tmp);
			pWD1793->TransferBufferIndex=0;
//			pWD1793->StatusReg= BUSY | DRQ;
			pWD1793->StatusReg= BUSY; //IRON
			pWD1793->WriteBytetoDisk=&WriteBytetoSector;
			pWD1793->ExecTimeWaiter=5;
			pWD1793->IOWaiter=0;
			pWD1793->MSectorFlag=Command & 1;
//			WriteLog("WRITESECTOR",0);
			break;

		case READADDRESS: 
//			MessageBox(0,"Hitting Get Address","Ok",0);
			SetType3Flags(pWD1793,Tmp);
			pWD1793->TransferBufferIndex=0;
			pWD1793->StatusReg= BUSY | DRQ;
			pWD1793->GetBytefromDisk=&GetBytefromAddress;
			pWD1793->ExecTimeWaiter=1;
			pWD1793->IOWaiter=0;

			break;

		case FORCEINTERUPT: 
			pWD1793->CurrentCommand=IDLE;
			pWD1793->TransferBufferSize=0;
			pWD1793->StatusReg=READY;
			pWD1793->ExecTimeWaiter=1;
			if ( (Tmp & 15) != 0 )
			{
				coco3_t *	pCoco3;
				
				pCoco3 = (coco3_t *)emuDevGetParentModuleByID(NULL, VCC_COCO3_ID);
				ASSERT_CC3(pCoco3);
				cpuAssertInterrupt(pCoco3->machine.pCPU,NMI,0);
			}
//			WriteLog("FORCEINTERUPT",0);
			break;

		case READTRACK:
//			WriteLog("Hitting ReadTrack",0);
			SetType3Flags(pWD1793,Tmp);
			pWD1793->TransferBufferIndex=0;
			pWD1793->StatusReg= BUSY | DRQ;
			pWD1793->GetBytefromDisk=&GetBytefromTrack;
			pWD1793->ExecTimeWaiter=1;
			pWD1793->IOWaiter=0;
			break;

		case WRITETRACK:
//			WriteLog("Getting WriteTrack Command",0);
			SetType3Flags(pWD1793,Tmp);
			pWD1793->TransferBufferIndex=0;
			pWD1793->StatusReg= BUSY ;		//IRON
//			StatusReg= BUSY | DRQ;
			pWD1793->WriteBytetoDisk=&WriteBytetoTrack;
			//pWD1793->ExecTimeWaiter=1;
			pWD1793->ExecTimeWaiter=5;		//IRON
			pWD1793->IOWaiter=0;
			break;
	}
	return;
}

/**********************************************************/

unsigned char GetBytefromSector (wd1793_t * pWD1793, unsigned char Tmp)
{
	unsigned char RetVal=0;

	if (pWD1793->TransferBufferSize == 0)
	{
		pWD1793->TransferBufferSize = ReadSector(pWD1793,pWD1793->Side,pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition,pWD1793->SectorReg,pWD1793->TransferBuffer);
//		WriteLog("BEGIN Readsector",0);
	}

	if (pWD1793->TransferBufferSize==0)// IRON| (TrackReg != Drive[CurrentDrive].HeadPosition) ) //| (SectorReg > Drive[CurrentDrive].Sectors)
	{
		CommandDone(pWD1793);
		pWD1793->StatusReg=RECNOTFOUND;
		return(0);
	}

	if ( pWD1793->TransferBufferIndex < pWD1793->TransferBufferSize )
	{
		RetVal=pWD1793->TransferBuffer[pWD1793->TransferBufferIndex++];
		pWD1793->StatusReg= BUSY | DRQ;
		pWD1793->IOWaiter=0;
//		WriteLog("Transfering 1 Byte",0);
	}
	else
	{
//		WriteLog("READSECTOR DONE",0);
		pWD1793->StatusReg=READY;
		CommandDone(pWD1793);
		if (pWD1793->LostDataFlag==1)
		{
			pWD1793->StatusReg=LOSTDATA;
			pWD1793->LostDataFlag=0;
		}
		pWD1793->SectorReg++;
	}
	return(RetVal);
}

/**********************************************************/

unsigned char GetBytefromAddress (wd1793_t * pWD1793, unsigned char Tmp)
{
	unsigned char	RetVal=0;
	unsigned short	Crc=0;
//	SectorInfo		CurrentSector;
//	unsigned char	TempBuffer[8192];
	//long			FileOffset=0;
	//long			Result=0;
	
	if (pWD1793->TransferBufferSize == 0)
	{
		switch (pWD1793->Drive[pWD1793->CurrentDisk].ImageType)
		{
			case JVC:
			case VDK:
			case OS9:
				pWD1793->TransferBuffer[0]= pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition;
				pWD1793->TransferBuffer[1]= pWD1793->Drive[pWD1793->CurrentDisk].Sides;
				pWD1793->TransferBuffer[2]= pWD1793->IndexCounter/176;
				pWD1793->TransferBuffer[3]= pWD1793->Drive[pWD1793->CurrentDisk].SectorSize;
		//		Crc = ccitt_crc16(0xE295, &TempBuffer[DataBlockPointer],pWD1793->Drive[pWD1793->CurrentDrive].SectorSize );
				pWD1793->TransferBuffer[4]= (Crc >> 8);	
				pWD1793->TransferBuffer[5]= (Crc & 0xFF);
				pWD1793->TransferBufferSize=6;
			break;

			case DMK:
				pWD1793->TransferBuffer[0]= pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition;; //CurrentSector.Track; not right need to get from image
				pWD1793->TransferBuffer[1]= pWD1793->Drive[pWD1793->CurrentDisk].Sides;
				pWD1793->TransferBuffer[2]= pWD1793->IndexCounter/176;
				pWD1793->TransferBuffer[3]= pWD1793->IndexCounter/176; //Drive[CurrentDrive].SectorSize;
				Crc = 0;//CurrentSector.CRC;
				pWD1793->TransferBuffer[4]= (Crc >> 8);	
				pWD1793->TransferBuffer[5]= (Crc & 0xFF);
				pWD1793->TransferBufferSize=6;				
			break;
		} //END Switch
	}

	if (   pWD1793->Drive[pWD1793->CurrentDisk].FileHandle==NULL  )
	{
		pWD1793->StatusReg=RECNOTFOUND;
		CommandDone(pWD1793);
		return(0);
	}

	if (pWD1793->TransferBufferIndex < pWD1793->TransferBufferSize)
	{
		RetVal=pWD1793->TransferBuffer[pWD1793->TransferBufferIndex++];
		pWD1793->StatusReg= BUSY | DRQ;
		pWD1793->IOWaiter=0;
	}
	else
	{
		pWD1793->StatusReg=READY;
		CommandDone(pWD1793);
		if (pWD1793->LostDataFlag==1)
		{
			pWD1793->StatusReg=LOSTDATA;
			pWD1793->LostDataFlag=0;
		}
	}
	
	return(RetVal);
}

/**********************************************************/

unsigned char GetBytefromTrack (wd1793_t * pWD1793, unsigned char Tmp)
{
	unsigned char RetVal=0;

	if (pWD1793->TransferBufferSize == 0)
	{
		pWD1793->TransferBufferSize = ReadTrack(pWD1793,pWD1793->Side,pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition,pWD1793->SectorReg,pWD1793->TransferBuffer);
//		WriteLog("BEGIN READTRACK",0);
	}

	if (  pWD1793->TransferBufferSize==0 )//iron | (TrackReg != pWD1793->Drive[pWD1793->CurrentDrive].HeadPosition) ) //| (pWD1793->SectorReg > pWD1793->Drive[pWD1793->CurrentDrive].Sectors)
	{
		CommandDone(pWD1793);
		pWD1793->StatusReg=RECNOTFOUND;
		return(0);
	}

	if ( pWD1793->TransferBufferIndex < pWD1793->TransferBufferSize)
	{
		RetVal=pWD1793->TransferBuffer[pWD1793->TransferBufferIndex++];
		pWD1793->StatusReg= BUSY | DRQ;
		pWD1793->IOWaiter=0;
	}
	else
	{
//		WriteLog("READTRACK DONE",0);
		pWD1793->StatusReg=READY;
		CommandDone(pWD1793);
		if (pWD1793->LostDataFlag==1)
		{
			pWD1793->StatusReg=LOSTDATA;
			pWD1793->LostDataFlag=0;
		}
		pWD1793->SectorReg++;
	}
	return(RetVal);
}

/**********************************************************/

unsigned char WriteBytetoSector (wd1793_t * pWD1793, unsigned char Tmp)
{
	unsigned long BytesRead=0;
	//unsigned long Result=0;
	long FileOffset=0,RetVal=0;;
	unsigned char TempBuffer[16384];
	SectorInfo CurrentSector;

	if (pWD1793->TransferBufferSize==0)
	{
//		WriteLog("Begining WriteSector data collection Command",0);
		switch (pWD1793->Drive[pWD1793->CurrentDisk].ImageType)
		{
			case JVC:
			case VDK:
			case OS9:
				pWD1793->TransferBufferSize=BytesperSector[pWD1793->Drive[pWD1793->CurrentDisk].SectorSize] ; 
				if ((pWD1793->TransferBufferSize==0)  | (pWD1793->TrackReg != pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition) | (pWD1793->SectorReg > pWD1793->Drive[pWD1793->CurrentDisk].Sectors) )
				{
					pWD1793->StatusReg=RECNOTFOUND;
					CommandDone(pWD1793);
					return(0);
				}
			break;

			case DMK:
				FileOffset= pWD1793->Drive[pWD1793->CurrentDisk].HeaderSize + ( (pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition * pWD1793->Drive[pWD1793->CurrentDisk].Sides * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize)+ (pWD1793->Side * pWD1793->Drive[pWD1793->CurrentDisk].TrackSize));
				fseek(pWD1793->Drive[pWD1793->CurrentDisk].FileHandle,FileOffset,SEEK_SET);
                BytesRead = fread(TempBuffer, 1, pWD1793->Drive[pWD1793->CurrentDisk].TrackSize, pWD1793->Drive[pWD1793->CurrentDisk].FileHandle);
				CurrentSector.Sector=pWD1793->SectorReg;
				GetSectorInfo(pWD1793,&CurrentSector,TempBuffer);
				if (   (CurrentSector.DAM == 0) 
					 | (BytesRead != pWD1793->Drive[pWD1793->CurrentDisk].TrackSize) 
				   )
				{
					pWD1793->StatusReg=RECNOTFOUND;
					CommandDone(pWD1793);
					
					return(0);
				}
				pWD1793->TransferBufferSize = CurrentSector.Length;
			break;

			case RAW:
				pWD1793->TransferBufferSize = 256;
			break;
		}	//END Switch
	}	//END if

	if (pWD1793->TransferBufferIndex>=pWD1793->TransferBufferSize) 
	{
		RetVal=WriteSector(pWD1793,pWD1793->Side,pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition,pWD1793->SectorReg,pWD1793->TransferBuffer,pWD1793->TransferBufferSize);
		pWD1793->StatusReg=READY;
		if (( RetVal==0) | (pWD1793->LostDataFlag==1) )
			pWD1793->StatusReg=LOSTDATA;
		if (pWD1793->Drive[pWD1793->CurrentDisk].WriteProtect != 0)
			pWD1793->StatusReg=WRITEPROTECT | RECNOTFOUND;
		CommandDone(pWD1793);	
		pWD1793->LostDataFlag=0;
		pWD1793->SectorReg++;
	}
	else
	{
		pWD1793->TransferBuffer[pWD1793->TransferBufferIndex++]=Tmp;
		pWD1793->StatusReg= BUSY | DRQ;
		pWD1793->IOWaiter=0;
	}
	return(0);
}

/**********************************************************/

unsigned char WriteBytetoTrack (wd1793_t * pWD1793, unsigned char Tmp)
{
	long RetVal=0;
	if (pWD1793->TransferBufferSize==0)
		pWD1793->TransferBufferSize=6272 ;


	if (pWD1793->TransferBufferIndex>=pWD1793->TransferBufferSize) 
	{
		RetVal=WriteTrack(pWD1793,pWD1793->Side,pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition,pWD1793->SectorReg,pWD1793->TransferBuffer);

		pWD1793->StatusReg=READY;
		if (( RetVal==0) | (pWD1793->LostDataFlag==1) )
			pWD1793->StatusReg=LOSTDATA;
		if (pWD1793->Drive[pWD1793->CurrentDisk].WriteProtect != 0)
			pWD1793->StatusReg=WRITEPROTECT | RECNOTFOUND;
		CommandDone(pWD1793);
		pWD1793->LostDataFlag=0;
	}
	else
	{
		pWD1793->TransferBuffer[pWD1793->TransferBufferIndex++]=Tmp;
		pWD1793->StatusReg= BUSY | DRQ;
		pWD1793->IOWaiter=0;
	}
	return(0);
}

/**********************************************************/

void SetType1Flags(wd1793_t * pWD1793, unsigned char Tmp)
{
	Tmp&=15;
	pWD1793->HeadLoad= (Tmp>>3)&1;
	pWD1793->TrackVerify= (Tmp>>2)&1;
	pWD1793->StepTimeMS = StepTimesMS[(Tmp & 3)];
	return;
}

/**********************************************************/

void SetType2Flags(wd1793_t * pWD1793, unsigned char Tmp)
{
	Tmp&=15;
	pWD1793->SideCompare= (Tmp>>3)&1;
	pWD1793->SideCompareEnable=(Tmp>>1)&1;
	pWD1793->Delay15ms=(Tmp>>2)&1;
	return;
}

/**********************************************************/

void SetType3Flags(wd1793_t * pWD1793, unsigned char Tmp)
{
	pWD1793->Delay15ms=(Tmp>>2)&1;
	return;
}

/**********************************************************/

#if (defined KEYBOARD_LEDS_SUPPORTED)

HANDLE OpenKeyboardDevice(wd1793_t * pWD1793, int *ErrorNumber)
{
	HANDLE	hndKbdDev;
	int		*LocalErrorNumber;
	int		Dummy;

	if (ErrorNumber == NULL)
		LocalErrorNumber = &Dummy;
	else
		LocalErrorNumber = ErrorNumber;

	*LocalErrorNumber = 0;
	
	if (!DefineDosDevice (DDD_RAW_TARGET_PATH, "Kbd",
				"\\Device\\KeyboardClass0"))
	{
		*LocalErrorNumber = GetLastError();
		return INVALID_HANDLE_VALUE;
	}

	hndKbdDev = CreateFile("\\\\.\\Kbd", GENERIC_WRITE, 0,
				NULL,	OPEN_EXISTING,	0,	NULL);
	
	if (hndKbdDev == INVALID_HANDLE_VALUE)
		*LocalErrorNumber = GetLastError();

	return hndKbdDev;
}


int CloseKeyboardDevice(wd1793_t * pWD1793, HANDLE hndKbdDev)
{
	int e = 0;

	if (!DefineDosDevice (DDD_REMOVE_DEFINITION, "Kbd", NULL))
		e = GetLastError();

	if (!CloseHandle(hndKbdDev))					
		e = GetLastError();
	hKbdDev=NULL;
	return e;
}

unsigned char UseKeyboardLeds(wd1793_t * pWD1793, unsigned char Tmp)
{
	if (Tmp!=QUERY)
	{
		UseLeds=Tmp;
		if (!UseLeds)
			CloseKeyboardDevice(&hKbdDev);
	}
	return(UseLeds);
}

#endif // KEYBOARD_LEDS_SUPPORTED

/**********************************************************/

long GetSectorInfo (wd1793_t * pWD1793, SectorInfo * Sector, unsigned char * TempBuffer)
{
	unsigned short Temp1=0,Temp2=0;
	unsigned char Density=0;
	long IdamIndex=0;

	do		//Scan IDAM table for correct sector
	{
		Temp1= (TempBuffer[IdamIndex+1]<<8) | TempBuffer[IdamIndex];	//Reverse Bytes and Get the pointer 
		Density= Temp1>>15;												//Density flag
		Temp1=  (0x3FFF & Temp1) % pWD1793->Drive[pWD1793->CurrentDisk].TrackSize;				//Mask and keep from overflowing
		IdamIndex+=2;
	}
	while ( (IdamIndex<128) & (Temp1 !=0) & (Sector->Sector != TempBuffer[Temp1+3]) );
		
	if ((Temp1 ==0) | (IdamIndex>128) )
		return (0);		//Can't Find the Specified Sector on this track

	//At this point Temp1 should be pointing to the IDAM $FE for the track in question
	Sector->Track = TempBuffer[Temp1+1];
	Sector->Side = TempBuffer[Temp1+2];
	Sector->Sector = TempBuffer[Temp1+3];
	Sector->Length = BytesperSector[ TempBuffer[Temp1+4] & 3]; //128 256 512 1024
	Sector->CRC = (TempBuffer[Temp1+5]<<8) | TempBuffer[Temp1+6] ;
	Sector->Density = Density;
	if ( (Sector->Track != pWD1793->TrackReg) | (Sector->Sector != pWD1793->SectorReg) )
		return(0); 
	
	Temp1+=7;				//Ignore the CRC
	Temp2=Temp1+43;			//Scan for D.A.M.
	while ( (TempBuffer[Temp1] !=0xFB) & (Temp1<Temp2) )
		Temp1++;
	if (Temp1==Temp2)		//Can't find data address mark
		return(0);
	Sector->DAM = Temp1+1;
	Sector->CRC = ccitt_crc16(0xE295, &TempBuffer[Sector->DAM], Sector->Length);
	return(Sector->DAM);	//Return Pointer to Start of sector data

}

/**********************************************************/

void CommandDone(wd1793_t * pWD1793)
{
	if ( pWD1793->InteruptEnable )
	{
		cpuAssertInterrupt(pWD1793->pCPU,NMI,0);

		pWD1793->TransferBufferSize=0;
		pWD1793->CurrentCommand=IDLE;
	}
}

/**********************************************************/

#if (defined RAW_DRIVE_ACCESS_SUPPORTED)
//Stolen from fdrawcmd.sys Demo Disk Utility by Simon Owen <simon@simonowen.com>

/**********************************************************/

u32_t GetDriverVersion (wd1793_t * pWD1793)
{
    u32_t dwVersion = 0;
    HANDLE h = CreateFile("\\\\.\\fdrawcmd", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (h != INVALID_HANDLE_VALUE)
    {
        DeviceIoControl(h, IOCTL_FDRAWCMD_GET_VERSION, NULL, 0, &dwVersion, sizeof(dwVersion), &dwRet, NULL);
        CloseHandle(h);
    }
    return dwVersion;
}

/**********************************************************/

void * OpenFloppy (wd1793_t * pWD1793, int nDrive_)
{
    char szDevice[32],szTemp[128]="";
	HANDLE h=NULL;
    wsprintf(szDevice, "\\\\.\\fdraw%u", nDrive_);
//	MessageBox(0,szDevice,"ok",0);
    h = CreateFile(szDevice, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE)
	{
		sprintf(szTemp,"Unable to open RAW device %s",szDevice);
		MessageBox(0,szTemp,"Ok",0);
	}
    return (h != INVALID_HANDLE_VALUE && SetDataRate(h, DISK_DATARATE)) ? h : NULL;
}

/**********************************************************/

bool_t SetDataRate (wd1793_t * pWD1793, HANDLE h_, BYTE bDataRate_)
{
	assert(0);
#if 0
    return !!DeviceIoControl(h_,IOCTL_FD_SET_DATA_RATE,&bDataRate_,sizeof(bDataRate_),NULL,0,&dwRet,NULL);
#else
	return FALSE;
#endif
}

/**********************************************************/

bool_t CmdFormat (wd1793_t * pWD1793, HANDLE h_, PFD_FORMAT_PARAMS pfp_, ULONG ulSize_)
{
	//	printf("phead %i, size %i sectors %i, gap %i, fill %i ulSize %i\n",pfp_->phead,pfp_->size,pfp_->sectors,pfp_->gap,pfp_->fill,ulSize_);
	//	printf("pfp_.Header->cyl %i, pfp_.Header->head %i, pfp.Header->sector %i, pfp_.Header->size %i\n", pfp_->Header->cyl,pfp_->Header->head,pfp_->Header->sector,pfp_->Header->size);
    return !!DeviceIoControl(h_, IOCTL_FDCMD_FORMAT_TRACK, pfp_, ulSize_, NULL, 0, &dwRet, NULL);
}

/**********************************************************/

bool_t FormatTrack (wd1793_t * pWD1793, HANDLE h_, BYTE cyl_, BYTE head_, BYTE Fill)
{
    BYTE abFormat[sizeof(FD_FORMAT_PARAMS) + sizeof(FD_ID_HEADER)*DISK_SECTORS];

    PFD_FORMAT_PARAMS pfp = (PFD_FORMAT_PARAMS)abFormat;
    pfp->flags = FD_OPTION_MFM;
    pfp->phead = head_;
    pfp->size = SECTOR_SIZE_CODE;
    pfp->sectors = DISK_SECTORS;
    pfp->gap = SECTOR_GAP3;
    pfp->fill = Fill;//SECTOR_FILL;

    PFD_ID_HEADER ph = pfp->Header;

    for (BYTE s = 0 ; s < pfp->sectors ; s++, ph++)
    {
//		printf("s = %i ph = %i\n",s,ph);
        ph->cyl = cyl_;
        ph->head = head_;
        ph->sector = SECTOR_BASE + ((s + cyl_*(pfp->sectors - TRACK_SKEW)) % pfp->sectors);
        ph->size = pfp->size;
    }

    return CmdFormat(h_, pfp, (PBYTE)ph - abFormat);
}

/**********************************************************/

unsigned short InitController (wd1793_t * pWD1793)
{
	long RawDriverVersion=0;
//	MessageBox(0,"Init Controller Called","Ok",0);
	RawDriverVersion=GetDriverVersion();
	if (RawDriverVersion != FDRAWCMD_VERSION)	//Drive either not loaded or not the right version
		return(0);
	if (RawReadBuf==NULL)
		RawReadBuf = VirtualAlloc(NULL, 4608, MEM_COMMIT, PAGE_READWRITE);
	if ( (RawReadBuf==NULL) )
		return(0);
	return(1);
}

/**********************************************************/

#endif // RAW_DRIVE_ACCESS_SUPPORTED

/**********************************************************/

unsigned char MountDisk(wd1793_t * pWD1793, const char * pPathname, unsigned char disk)
{
	//unsigned long	BytesRead	=0;
    long			TotalSectors=0;
	unsigned char	TmpSides	=0;
	unsigned char	TmpSectors	=0;
	unsigned char	TmpMod		=0;
    //unsigned char Index1		=0;
	unsigned char	HeaderBlock[HEADERBUFFERSIZE]="";
	
	if ( pWD1793->Drive[disk].FileHandle != NULL )
	{
		wd1793UnmountDiskImage(pWD1793,disk);
	}
	
	//Image Geometry Defaults
	pWD1793->Drive[disk].FirstSector=1;
	pWD1793->Drive[disk].Sides=1;
	pWD1793->Drive[disk].Sectors=18;
	pWD1793->Drive[disk].SectorSize=1;
	pWD1793->Drive[disk].WriteProtect=0;
	pWD1793->Drive[disk].RawDrive=0;
	
#if (defined RAW_DRIVE_ACCESS_SUPPORTED)
	assert(false && "Re-test");
    // TODO: Windows specific
	if (!strcmp(FileName,"*Floppy A:"))
		pWD1793->Drive[disk].RawDrive=1;
	
	if (!strcmp(FileName,"*Floppy B:"))
		pWD1793->Drive[disk].RawDrive=2;
#endif
	
	if ( pWD1793->Drive[disk].RawDrive == 0 )
	{
		pWD1793->Drive[disk].FileHandle = fopen(pPathname,"r+");
		if ( pWD1793->Drive[disk].FileHandle == NULL )
		{	
			//Can't open read/write might be read only
			pWD1793->Drive[disk].FileHandle = fopen(pPathname,"r");
			pWD1793->Drive[disk].WriteProtect = 0xFF;
		}
		if (pWD1793->Drive[disk].FileHandle == NULL )
		{
			return (1); //Give up cant mount it
		}
		
		pWD1793->Drive[disk].pPathname = strdup(pPathname);
		
		fseek(pWD1793->Drive[disk].FileHandle, 0, SEEK_END);
		pWD1793->Drive[disk].FileSize = ftell(pWD1793->Drive[disk].FileHandle);
		
		pWD1793->Drive[disk].HeaderSize = (unsigned char)(pWD1793->Drive[disk].FileSize % 256);
		
		fseek(pWD1793->Drive[disk].FileHandle, 0, SEEK_SET);
        /* BytesRead = */ fread(HeaderBlock, 1, HEADERBUFFERSIZE, pWD1793->Drive[disk].FileHandle);
	}
	else
	{
		pWD1793->Drive[disk].HeaderSize=0xFF;
	}
	
	switch (pWD1793->Drive[disk].HeaderSize)
	{
		case 4:
			pWD1793->Drive[disk].FirstSector = HeaderBlock[3];
		case 3:
			pWD1793->Drive[disk].SectorSize = HeaderBlock[2];
		case 2:
			pWD1793->Drive[disk].Sectors = HeaderBlock[0];
			pWD1793->Drive[disk].Sides = HeaderBlock[1];
			
		case 0:	
			//OS9 Image checks
			TotalSectors = (HeaderBlock[0]<<16) + (HeaderBlock[1]<<8) + (HeaderBlock[2]);
			TmpSides = (HeaderBlock[16] & 1)+1;
			TmpSectors = HeaderBlock[3];
			TmpMod=1;
			if ( (TmpSides*TmpSectors)!=0)
				TmpMod = (unsigned char)(TotalSectors%(TmpSides*TmpSectors));	
			//if ((TmpSectors ==18) & (TmpMod==0) & (pWD1793->Drive[disk].HeaderSize ==0))	//Sanity Check 
			if ((TmpSectors >=18) & (TmpMod==0) & (pWD1793->Drive[disk].HeaderSize ==0))	//Sanity Check 
			{
				pWD1793->Drive[disk].ImageType=OS9;
				pWD1793->Drive[disk].Sides = TmpSides;
				pWD1793->Drive[disk].Sectors = TmpSectors;
			}
			else
				pWD1793->Drive[disk].ImageType=JVC; 
			pWD1793->Drive[disk].TrackSize = pWD1793->Drive[disk].Sectors * BytesperSector[pWD1793->Drive[disk].SectorSize];
			break;
			
		case 12:
			pWD1793->Drive[disk].ImageType=VDK;	//VDK
			pWD1793->Drive[disk].Sides = HeaderBlock[9];
			pWD1793->Drive[disk].TrackSize = pWD1793->Drive[disk].Sectors * BytesperSector[pWD1793->Drive[disk].SectorSize];
			break;
			
		case 16:
			pWD1793->Drive[disk].ImageType=DMK;	//DMK 
			if (pWD1793->Drive[disk].WriteProtect != 0xFF) 
				pWD1793->Drive[disk].WriteProtect = HeaderBlock[0];
			pWD1793->Drive[disk].TrackSize = (HeaderBlock[3]<<8 | HeaderBlock[2]);
			pWD1793->Drive[disk].Sides = 2-((HeaderBlock[4] & 16 )>>4);
			break;
			
#if defined(RAW_DRIVE_ACCESS_SUPPORTED)
		case 0xFF:
			if ( pWD1793->Drive[disk].RawDrive )
			{
				if (pWD1793->Drive[disk].FileHandle !=NULL)
					wd1793UnmountDiskImage(disk);
				pWD1793->Drive[disk].ImageType=RAW;
				pWD1793->Drive[disk].Sides=2;
				
				pWD1793->Drive[disk].FileHandle = OpenFloppy(pWD1793->Drive[disk].RawDrive-1);
				if (pWD1793->Drive[disk].FileHandle == NULL)
					return(1);
				strcpy(pWD1793->Drive[disk].ImageName,FileName);
				if (pWD1793->Drive[disk].RawDrive==1)
					pWD1793->PhysicalDriveA=disk+1;
				if (pWD1793->Drive[disk].RawDrive==2)
					pWD1793->PhysicalDriveB=disk+1;
			}
			break;
#endif
			
		default:
			return(1);
	}
	
	strncpy(pWD1793->Drive[disk].ImageTypeName,ImageFormat[pWD1793->Drive[disk].ImageType],4);
	
	return(0); //Return 0 on success
}

/********************************************************************************/

#pragma mark -
#pragma mark --- Images ---

/**********************************************************/

int wd1793MountDiskImage(wd1793_t * pWD1793, const char * pPathname, unsigned char drive)
{
	unsigned int Temp=0;
	
	Temp = MountDisk(pWD1793,pPathname,drive);
	
	return !Temp;
}

/**********************************************************/

void wd1793UnmountDiskImage(wd1793_t * pWD1793, unsigned char drive)
{
	if ( pWD1793->Drive[drive].FileHandle != NULL )
	{
		fclose(pWD1793->Drive[drive].FileHandle);
	}
	pWD1793->Drive[drive].FileHandle=NULL;
	
	pWD1793->Drive[drive].ImageType=0;
	
	free(pWD1793->Drive[drive].pPathname);
	pWD1793->Drive[drive].pPathname = NULL;
	
	pWD1793->DirtyDisk=1;
    
#if (defined RAW_DRIVE_ACCESS_SUPPORTED)
	if ( drive == (pWD1793->PhysicalDriveA-1) )
	{
		pWD1793->PhysicalDriveA = 0;
	}
	if ( drive == (pWD1793->PhysicalDriveB-1) )
	{
		pWD1793->PhysicalDriveB = 0;
	}
#endif
}

/********************************************************************************/

#pragma mark -
#pragma mark --- Config ---

/**********************************************************/

#if 0
unsigned char wd1793GetTurboMode(wd1793_t * pWD1793)
{
    return pWD1793->TurboMode;
}
#endif

void wd1793SetTurboMode(wd1793_t * pWD1793, bool enabled)
{
    pWD1793->TurboMode = enabled;
    
    if (!pWD1793->TurboMode)
    {
        pWD1793->CyclestoSettle = pWD1793->SETTLETIME;
        pWD1793->CyclesperStep  = pWD1793->STEPTIME;
    }
    else
    {
        pWD1793->CyclestoSettle = 1;
        pWD1793->CyclesperStep  = 0;
    }
}

/********************************************************************************/

#pragma mark -
#pragma mark --- emulator device callbacks ---

/**********************************************************/
/*
 This gets called by the FD502 emulator device heatbeat
 
 i.e. at the end of every scan line so the controller has
 acurate timing.
 */
void wd1793HeartBeat(wd1793_t * pWD1793)
{
    if (   pWD1793->pCPU == NULL
        && pWD1793->device.pParent != NULL
        )
    {
        emurootdevice_t * root = emuDevGetRootDevice(&pWD1793->device);
        assert(root != NULL);
        
        pWD1793->pCPU = (cpu_t *)emuDevFindByModuleID(&root->device, EMU_CPU_ID);
        assert(pWD1793->pCPU);
    }
    
    static char wobble=0;
    if ( pWD1793->MotorOn == 0 )
        return;
    
    pWD1793->IndexCounter++;
    if (pWD1793->IndexCounter > ((pWD1793->INDEXTIME-30)+wobble))
    {
        pWD1793->IndexPulse=1;
    }
    if (pWD1793->IndexCounter > (pWD1793->INDEXTIME+wobble))
    {
        pWD1793->IndexPulse=0;
        pWD1793->IndexCounter=0;
        wobble=rand();
        wobble/=4;
    }
    
    if ( ((pWD1793->CurrentCommand<=7) | (pWD1793->CurrentCommand==IDLE)) & (pWD1793->Drive[pWD1793->CurrentDisk].FileHandle!=NULL) )
    {
        if ( pWD1793->IndexPulse  )
            pWD1793->StatusReg|=INDEXPULSE;
        else
            pWD1793->StatusReg &=(~INDEXPULSE);
    }
    
    if (pWD1793->CurrentCommand==IDLE)
        return;
    
    pWD1793->ExecTimeWaiter--;
    if (pWD1793->ExecTimeWaiter > 0)  //Click Click Click
        return;
    
    pWD1793->ExecTimeWaiter=1;
    pWD1793->IOWaiter++;
    
    switch (pWD1793->CurrentCommand)
    {
        case RESTORE:
            if (pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition!=0)
            {
                pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition-=1;
                pWD1793->ExecTimeWaiter=(pWD1793->CyclesperStep * pWD1793->StepTimeMS);
                
#if (defined RAW_DRIVE_ACCESS_SUPPORTED)
                if (pWD1793->Drive[pWD1793->CurrentDisk].ImageType==RAW)
                    DeviceIoControl(pWD1793->Drive[pWD1793->CurrentDisk].FileHandle , IOCTL_FDCMD_SEEK, 0, 1, NULL, 0, &dwRet, NULL);
#endif
            }
            else
            {
                pWD1793->TrackReg=0;
                pWD1793->StatusReg=READY;
                pWD1793->StatusReg|=TRACK_ZERO;
                pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition=0;
                if (pWD1793->Drive[pWD1793->CurrentDisk].WriteProtect)
                    pWD1793->StatusReg|=WRITEPROTECT;
                CommandDone(pWD1793);
            }
            break;
            
        case SEEK:
            if (pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition!=pWD1793->DataReg)
            {
                if (pWD1793->StepDirection==1)
                    pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition+=1;
                else
                    pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition-=1;
                pWD1793->ExecTimeWaiter=(pWD1793->CyclesperStep * pWD1793->StepTimeMS);
            }
            else
            {
                pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition=pWD1793->TrackReg;
                pWD1793->StatusReg=READY;
                if (pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition==0)
                    pWD1793->StatusReg|=TRACK_ZERO;
                if (pWD1793->Drive[pWD1793->CurrentDisk].WriteProtect)
                    pWD1793->StatusReg|=WRITEPROTECT;
                CommandDone(pWD1793);
            }
            
#if (defined RAW_DRIVE_ACCESS_SUPPORTED)
            if (pWD1793->Drive[pWD1793->CurrentDisk].ImageType==RAW)
                DeviceIoControl(pWD1793->Drive[pWD1793->CurrentDisk].FileHandle , IOCTL_FDCMD_SEEK, &TrackReg, sizeof(TrackReg), NULL, 0, &dwRet, NULL);
#endif
            break;
            
        case STEP:
        case STEPUPD:
            if (pWD1793->StepDirection==1)
                pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition+=1;
            else
            {
                if (pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition >0)
                    pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition-=1;
            }
            
            if (pWD1793->CurrentCommand & 1)
            {
                if (pWD1793->StepDirection==1)
                    pWD1793->TrackReg+=1;
                else
                    pWD1793->TrackReg-=1;
            }
            if ( pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition == 0)
                pWD1793->StatusReg=TRACK_ZERO;
            if (pWD1793->Drive[pWD1793->CurrentDisk].WriteProtect)
                pWD1793->StatusReg|=WRITEPROTECT;
            pWD1793->StatusReg=READY;
            CommandDone(pWD1793);
            break;
            
        case STEPIN:
        case STEPINUPD:
            pWD1793->StepDirection=1;
            pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition+=1;
            if (pWD1793->CurrentCommand & 1)
                pWD1793->TrackReg+=1;
            if (pWD1793->Drive[pWD1793->CurrentDisk].WriteProtect)
                pWD1793->StatusReg|=WRITEPROTECT;
            pWD1793->StatusReg=READY;
            CommandDone(pWD1793);
            break;
            
        case STEPOUT:
        case STEPOUTUPD:
            pWD1793->StepDirection=0;
            if ( pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition > 0 )
                pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition-=1;
            if ( pWD1793->CurrentCommand & 1)
                pWD1793->TrackReg-=1;
            if ( pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition == 0)
                pWD1793->StatusReg=TRACK_ZERO;
            if (pWD1793->Drive[pWD1793->CurrentDisk].WriteProtect)
                pWD1793->StatusReg|=WRITEPROTECT;
            pWD1793->StatusReg=READY;
            CommandDone(pWD1793);
            break;
            
        case READSECTOR:
        case READSECTORM:
            if (pWD1793->IOWaiter > pWD1793->WAITTIME)
            {
                pWD1793->LostDataFlag=1;
                GetBytefromSector(pWD1793,0);
            }
            break;
            
        case WRITESECTOR:
        case WRITESECTORM:
            pWD1793->StatusReg |= DRQ;    //IRON
            if ( pWD1793->IOWaiter > pWD1793->WAITTIME )
            {
                pWD1793->LostDataFlag=1;
                //WriteLog("WRITESECTOR TIMEOUT",0);
                WriteBytetoSector(pWD1793,0);
            }
            break;
            
        case READADDRESS:
            if ( pWD1793->IOWaiter > pWD1793->WAITTIME )
            {
                pWD1793->LostDataFlag=1;
                GetBytefromAddress (pWD1793,0);
            }
            break;
            
        case FORCEINTERUPT:
            break;
            
        case READTRACK:
            if ( pWD1793->IOWaiter > pWD1793->WAITTIME )
            {
                pWD1793->LostDataFlag=1;
                GetBytefromTrack(pWD1793,0);
            }
            break;
            
        case WRITETRACK:
            pWD1793->StatusReg |= DRQ;    //IRON
            if ( pWD1793->IOWaiter > pWD1793->WAITTIME )
            {
                pWD1793->LostDataFlag=1;
                WriteBytetoTrack(pWD1793,0);
            }
            break;
            
        case IDLE:
            break;
    }
}

/**********************************************************/

result_t wd1793EmuDevDestroy(emudevice_t * pEmuDevice)
{
    return XERROR_NONE;
}

/********************************************************************************/

#pragma mark -
#pragma mark --- emulator device IO ---

/*************************************************************/
/**
 */
unsigned char wd1793IORead(wd1793_t * pWD1793, unsigned char port)
{
    unsigned char temp;
    
    switch ( port )
    {
        case 0x48:
            temp = pWD1793->StatusReg;
            break;
            
        case 0x49:
            temp = pWD1793->TrackReg;
            break;
            
        case 0x4A:
            temp = pWD1793->SectorReg;
            break;
            
        case 0x4B:                        //Reading data from disk
            if ( pWD1793->CurrentCommand == IDLE )
            {
                temp = pWD1793->DataReg;
            }
            else
            {
                temp = pWD1793->GetBytefromDisk(pWD1793,0);
            }
            break;
            
        case 0x40:    //Control Register can't be read
            temp = 0;
            break;
            
        default:
            return 0;
            break;
    }
    
    return temp;
}

/**********************************************************/
/**
 */
void wd1793IOWrite(wd1793_t * pWD1793, unsigned char port, unsigned char data)
{
    switch (port)
    {
        case 0x48:    //Command Register
            DispatchCommand(pWD1793,data);
            break;
            
        case 0x49:
            pWD1793->TrackReg = data;
            break;
            
        case 0x4A:
            pWD1793->SectorReg = data;
            break;
            
        case 0x4B:    //Writing Data to disk
            if ( pWD1793->CurrentCommand == IDLE )
            {
                pWD1793->DataReg = data;
            }
            else
            {
                pWD1793->WriteBytetoDisk(pWD1793,data);
            }
            break;
            
        case 0x40:
            pWD1793->ControlReg = data;
            DecodeControlReg(pWD1793,pWD1793->ControlReg);
            break;
            
        default:
            //return 0;
            break;
    }
}

/********************************************************************************/

#pragma mark -
#pragma mark --- emulator device config ---

/********************************************************************************/
/**
 */
result_t wd1793EmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	wd1793_t *	pWD1793		= (wd1793_t *)pEmuDevice;
	int		    x;
	char		Temp[256];
	
	//ASSERT_WD1793(pWD1793);
	if ( pWD1793 != NULL )
	{	
		for (x=0; x<NUM_DRIVES; x++)
		{
			if ( pWD1793->Drive[x].pPathname != NULL )
			{
				sprintf(Temp,"Drive-%d",x);
				confSetPath(config,"Disks" ,Temp, pWD1793->Drive[x].pPathname,config->absolutePaths);
			}
		}

		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/
/**
 */
result_t wd1793EmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	wd1793_t *	pWD1793		= (wd1793_t *)pEmuDevice;
	int		    x;
	char		Temp[256];
	char *	    pPathname;
	
	ASSERT_WD1793(pWD1793);
	if ( pWD1793 != NULL )
	{	
		for (x=0; x<NUM_DRIVES; x++)
		{
			pPathname = NULL;
			sprintf(Temp,"Drive-%d",x);
			confGetPath(config, "Disks", Temp, &pPathname, config->absolutePaths);
			if ( pPathname != NULL )
			{
				MountDisk(pWD1793, pPathname, x);
                
                free(pPathname);
			}
		}
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/

#pragma mark -
#pragma mark --- pakInterface callbacks ---

/**********************************************************/

void wd1793Reset(wd1793_t * pWD1793)
{
    // TODO: update based on heartbeats per frame from the system
    //
    // timings - based on pings to pak
    pWD1793->WAITTIME = 5;
    pWD1793->STEPTIME = 15;    // 1 Millisecond = 15.78 Pings
    pWD1793->SETTLETIME = 10;    // CyclestoSettle
    pWD1793->INDEXTIME = (((int)(TOTAL_LINESPERSCREEN/2) * 60)/5);
    
    // reset CPU reference in case it has changed
    pWD1793->pCPU = NULL;
    
    pWD1793->GetBytefromDisk	= &GetBytefromSector;
	pWD1793->WriteBytetoDisk	= &WriteBytetoSector;
	
	pWD1793->StatusReg=READY;
	pWD1793->DataReg=0;
	pWD1793->TrackReg=0;
	pWD1793->SectorReg=0;
	pWD1793->ControlReg=0;
	pWD1793->Side=0;
	pWD1793->CurrentCommand=IDLE;
	pWD1793->CurrentDisk=NONE;
	pWD1793->LastDisk=NONE;
	pWD1793->MotorOn=0;
	pWD1793->InteruptEnable=0;
	pWD1793->HaltEnable=0;
	pWD1793->HeadLoad=0;
	pWD1793->TrackVerify=0;
	pWD1793->StepTimeMS= 0;
	pWD1793->SideCompare=0;
	pWD1793->SideCompareEnable=0;
	pWD1793->Delay15ms=0;
	pWD1793->StepDirection=1;
	pWD1793->IndexPulse = 0;
	pWD1793->MSectorFlag=0;
	pWD1793->LostDataFlag=0;
	pWD1793->ExecTimeWaiter=0;
	pWD1793->TransferBufferIndex=0;
	pWD1793->TransferBufferSize=0;
	pWD1793->IOWaiter=0;
	pWD1793->IndexCounter=0;
	pWD1793->DirtyDisk=1;
    
#if defined KEYBOARD_LEDS_SUPPORTED
    pWD1793->KBLeds=0;
	//pWD1793->InputBuffer;	  // Input buffer for DeviceIoControl
	//pWD1793->OutputBuffer;	  // Output buffer for DeviceIoControl
	pWD1793->DataLength = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
	//pWD1793->ReturnedLength; // Number of bytes returned in output buffer
	//pWD1793->hKbdDev;	
#endif
	
#if (defined RAW_DRIVE_ACCESS_SUPPORTED)
    //pWD1793->rwp;
    pWD1793->MSectorCount=0;
    pWD1793->UseLeds=0;
    pWD1793->RawReadBuf=NULL;
	//pWD1793->PhysicalDriveA;
	//pWD1793->PhysicalDriveB;
	//pWD1793->OldPhysicalDriveA;
	//pWD1793->OldPhysicalDriveB;
#endif
}

/**********************************************************/

void wd1793Init(wd1793_t * pWD1793)
{
	assert(pWD1793 != NULL);
	if ( pWD1793 != NULL )
	{
		memset(pWD1793,0,sizeof(wd1793_t));
		
		pWD1793->device.id			= EMU_DEVICE_ID;
		pWD1793->device.idModule	= VCC_WD1793_ID;
		strcpy(pWD1793->device.Name,"WD1793");
		
		pWD1793->device.pfnDestroy	= wd1793EmuDevDestroy;
		pWD1793->device.pfnSave		= wd1793EmuDevConfSave;
		pWD1793->device.pfnLoad		= wd1793EmuDevConfLoad;

		wd1793Reset(pWD1793);
	}
}

/**********************************************************/
