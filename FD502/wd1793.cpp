////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute itand/or
//	modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your
//	option) any later version.
//	
//	VCC (Virtual Color Computer) is distributed in the hope that it will be
//	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
//	Public License for more details.
//	
//	You should have received a copy of the GNU General Public License along with
//	VCC (Virtual Color Computer). If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////
#include "wd1793.h"
#include "wd1793defs.h"
#include "defines.h"
#include "fd502.h"
#include "fdrawcmd.h"	// http://simonowen.com/fdrawcmd/
#include <winioctl.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
/****************Fuction Protos for this Module************/

unsigned char GetBytefromSector (::vcc::bus::expansion_port_bus& bus);
unsigned char GetBytefromAddress(::vcc::bus::expansion_port_bus& bus);
unsigned char GetBytefromTrack  (::vcc::bus::expansion_port_bus& bus);
unsigned char (*GetBytefromDisk)(::vcc::bus::expansion_port_bus&)=&GetBytefromSector;

unsigned char WriteBytetoSector (::vcc::bus::expansion_port_bus& bus, unsigned char);
unsigned char WriteBytetoTrack  (::vcc::bus::expansion_port_bus& bus, unsigned char);
unsigned char (*WriteBytetoDisk)(::vcc::bus::expansion_port_bus&, unsigned char)=&WriteBytetoSector;

unsigned char MountDisk(const char *filename,unsigned char);
void DispatchCommand(::vcc::bus::expansion_port_bus& bus, unsigned char);
void DecodeControlReg(unsigned char);
void SetType1Flags(unsigned char);
void SetType2Flags(unsigned char);
void SetType3Flags(unsigned char);

long ReadSector (unsigned char,unsigned char,unsigned char,unsigned char *);
long WriteSector(unsigned char,unsigned char,unsigned char,const unsigned char *,long);
long ReadTrack  (unsigned char,unsigned char,unsigned char,unsigned char *);
long WriteTrack (unsigned char,unsigned char,unsigned char,const unsigned char *);

unsigned short ccitt_crc16(unsigned short crc, const unsigned char *, unsigned short );
long GetSectorInfo (SectorInfo *,const unsigned char *);
void CommandDone(::vcc::bus::expansion_port_bus& bus);
extern unsigned char PhysicalDriveA,PhysicalDriveB;
bool FormatTrack (HANDLE , BYTE , BYTE,BYTE );
bool CmdFormat (HANDLE , PFD_FORMAT_PARAMS , ULONG );
/**********************************************************/
static unsigned char StepTimesMS[4]={6,12,20,30};
static unsigned short BytesperSector[4]={128,256,512,1024};
static unsigned char TransferBuffer[16384]="";
static DiskInfo Drive[5];
static unsigned char StatusReg=READY;
static unsigned char DataReg=0;
static unsigned char TrackReg=0;
static unsigned char SectorReg=0;
static unsigned char ControlReg=0;
static unsigned char Side=0;
static unsigned char CurrentCommand=IDLE,CurrentDisk=NONE,LastDisk=NONE;
static unsigned char MotorOn=0;
static unsigned char KBLeds=0;
static unsigned char InteruptEnable=0;
static unsigned char HaltEnable=0;
static unsigned char HeadLoad=0;
static unsigned char TrackVerify=0;
static unsigned char StepTimeMS= 0;
static unsigned char SideCompare=0;
static unsigned char SideCompareEnable=0;
static unsigned char Delay15ms=0;
static unsigned char StepDirection=1;
static unsigned char IndexPulse;
static unsigned char TurboMode=0;
static unsigned char CyclestoSettle=SETTLETIME;
static unsigned char CyclesperStep=STEPTIME;
static unsigned char MSectorFlag=0; 
static unsigned char LostDataFlag=0;
static int ExecTimeWaiter=0;
static long TransferBufferIndex=0;
static long TransferBufferSize=0;
static unsigned int IOWaiter=0;
static unsigned int IndexCounter=0;
//static unsigned char UseLeds=0;
DWORD dwRet;
HANDLE OpenFloppy (int );
static PVOID RawReadBuf=nullptr;

bool SetDataRate (HANDLE , BYTE );
static FD_READ_WRITE_PARAMS rwp;
static bool DirtyDisk=true;
char ImageFormat[5][4]={"JVC","VDK","DMK","OS9","RAW"};


std::string get_mounted_disk_filename(::std::size_t drive_index)
{
	return Drive[drive_index].ImageName;
}

/*************************************************************/
unsigned char disk_io_read(::vcc::bus::expansion_port_bus& bus, unsigned char port)
{
	unsigned char temp;

	switch (port)
	{
		case 0x48:
			temp=StatusReg;
			break;

		case 0x49:
			temp=TrackReg;
			break;

		case 0x4A:
			temp=SectorReg;
			break;

		case 0x4B:						//Reading data from disk
			if (CurrentCommand==IDLE)
				temp=DataReg;
			else
				temp=GetBytefromDisk(bus);
			break;

		case 0x40:	//Control Register can't be read
			temp=0;
			break;
		default:
			return 0;
	}	//END Switch port
	return temp;
}

void disk_io_write(::vcc::bus::expansion_port_bus& bus, unsigned char data,unsigned char port)
{
	switch (port)
	{
	case	0x48:	//Command Register
		DispatchCommand(bus, data);
	break;

	case	0x49:
		TrackReg=data;
	break;

	case	0x4A:
		SectorReg=data;
	break;

	case	0x4B:	//Writing Data to disk
		if (CurrentCommand==IDLE)
			DataReg=data;
		else
			WriteBytetoDisk(bus, data);
	break;
			
	case	0x40:
		ControlReg=data;
		DecodeControlReg(ControlReg);
	break;
	}	//END Switch port
	return;
}
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
void DecodeControlReg(unsigned char Tmp)
{

	KBLeds=0;
	MotorOn=0;
	CurrentDisk=NONE;
	Side=0;
	InteruptEnable=0;
	HaltEnable=0;

	if (Tmp & CTRL_DRIVE0)
	{
		CurrentDisk=0;
	}
	if (Tmp & CTRL_DRIVE1)
	{
		CurrentDisk=1;
	}
	if (Tmp & CTRL_DRIVE2)
	{
		CurrentDisk=2;
	}
	if (Tmp & SIDESELECT)	//DRIVE3 Select in "all single sided" systems
		Side=1;
	if (Tmp & CTRL_MOTOR_EN)
	{
		MotorOn=1;
		if ( Drive[CurrentDisk].ImageType == RAW)
			DeviceIoControl(Drive[CurrentDisk].FileHandle, IOCTL_FD_MOTOR_ON, nullptr, 0, nullptr, 0, &dwRet, nullptr);
	}

	if ( (Side==1) & (CurrentDisk==NONE) )
	{
		CurrentDisk=3;
		Side=0;
	}
	if ( !(Tmp & CTRL_MOTOR_EN))	//Turning off the drive makes the disk dirty
		DirtyDisk=true;

	if (LastDisk != CurrentDisk)	//If we switch from reading one Physical disk to another we need to invalidate the cache
		DirtyDisk=true;
	LastDisk=CurrentDisk;
	if (Tmp & CTRL_DENSITY)	//Strange, Density and Interupt enable flag
		InteruptEnable=1;
	if (Tmp & CTRL_HALT_FLAG)
		HaltEnable=1;

	return;
}

int mount_disk_image(const char *filename,unsigned char drive)
{
	unsigned int Temp=0;
	Temp=MountDisk(filename,drive);
	// FIXME-CHET: This needs to be added back in
	//BuildCartridgeMenu();
	return(!Temp);
}

void unmount_disk_image(unsigned char drive)
{
	if (Drive[drive].FileHandle !=nullptr)
		CloseHandle(Drive[drive].FileHandle);
	Drive[drive].FileHandle=nullptr;
	Drive[drive].ImageType=0;
	strcpy(Drive[drive].ImageName,"");
	DirtyDisk=true;
	if (drive==(PhysicalDriveA-1))
		PhysicalDriveA=0;
	if (drive==(PhysicalDriveB-1))
		PhysicalDriveB=0;
	return;
}

void DiskStatus(char* text_buffer, size_t buffer_size)
{
	if (MotorOn==1) 
		sprintf(text_buffer,"FD-502:Drv:%1.1i %s Trk:%2.2i Sec:%2.2i Hd:%1.1i",CurrentDisk,ImageFormat[Drive[CurrentDisk].ImageType],Drive[CurrentDisk].HeadPosition,SectorReg,Side);
	else
		sprintf(text_buffer,"FD-502:Idle");
}

unsigned char MountDisk(const char *FileName,unsigned char disk)
{
	unsigned long BytesRead=0;
	unsigned char HeaderBlock[HEADERBUFFERSIZE]="";
	long TotalSectors=0;
	unsigned char TmpSides=0,TmpSectors=0,TmpMod=0;

	if (Drive[disk].FileHandle !=nullptr)
		unmount_disk_image(disk);
	//Image Geometry Defaults
	Drive[disk].FirstSector=1;
	Drive[disk].Sides=1;
	Drive[disk].Sectors=18;
	Drive[disk].SectorSize=1;
	Drive[disk].WriteProtect=0;
	Drive[disk].RawDrive=0;

	if (!strcmp(FileName,"*Floppy A:"))
		Drive[disk].RawDrive=1;
	if (!strcmp(FileName,"*Floppy B:"))
		Drive[disk].RawDrive=2;

	if (Drive[disk].RawDrive==0)
	{
		Drive[disk].FileHandle = CreateFile( FileName,GENERIC_READ | GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
		if (Drive[disk].FileHandle==INVALID_HANDLE_VALUE)
		{	//Can't open read/write might be read only
			Drive[disk].FileHandle = CreateFile(FileName,GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
			Drive[disk].WriteProtect=0xFF;
		}
		if (Drive[disk].FileHandle==INVALID_HANDLE_VALUE)
			return 1; //Give up cant mount it
		strcpy(Drive[disk].ImageName,FileName);
		Drive[disk].FileSize= GetFileSize(Drive[disk].FileHandle,nullptr);
		Drive[disk].HeaderSize = Drive[disk].FileSize % 256;
		SetFilePointer(Drive[disk].FileHandle,0,nullptr,FILE_BEGIN);
		ReadFile(Drive[disk].FileHandle,HeaderBlock,HEADERBUFFERSIZE,&BytesRead,nullptr);
	}
	else
		Drive[disk].HeaderSize=0xFF;

	switch (Drive[disk].HeaderSize)
	{
	case 4:
			Drive[disk].FirstSector=HeaderBlock[3];
	case 3:
			Drive[disk].SectorSize=HeaderBlock[2];
	case 2:
			Drive[disk].Sectors = HeaderBlock[0];
			Drive[disk].Sides = HeaderBlock[1];
	
	case 0:	
		//OS9 Image checks
		TotalSectors = (HeaderBlock[0]<<16) + (HeaderBlock[1]<<8) + (HeaderBlock[2]);
		TmpSides = (HeaderBlock[16] & 1)+1;
		TmpSectors = HeaderBlock[3];
		TmpMod=1;
		if ( (TmpSides*TmpSectors)!=0)
			TmpMod = TotalSectors%(TmpSides*TmpSectors);	
		if ((TmpSectors ==18) & (TmpMod==0) & (Drive[disk].HeaderSize ==0))	//Sanity Check 
		{
			Drive[disk].ImageType=OS9;
			Drive[disk].Sides = TmpSides;
			Drive[disk].Sectors = TmpSectors;
		}
		else
			Drive[disk].ImageType=JVC; 
		Drive[disk].TrackSize = Drive[disk].Sectors * BytesperSector[Drive[disk].SectorSize];
	break;

	case 12:
		Drive[disk].ImageType=VDK;	//VDK
		Drive[disk].Sides = HeaderBlock[9];
		Drive[disk].TrackSize = Drive[disk].Sectors * BytesperSector[Drive[disk].SectorSize];
		break;

	case 16:
		Drive[disk].ImageType=DMK;	//DMK 
		if (Drive[disk].WriteProtect != 0xFF) 
			Drive[disk].WriteProtect = HeaderBlock[0];
		Drive[disk].TrackSize = (HeaderBlock[3]<<8 | HeaderBlock[2]);
		Drive[disk].Sides = 2-((HeaderBlock[4] & 16 )>>4);
	break;

	case 0xFF:
		if (Drive[disk].RawDrive)
		{
			if (Drive[disk].FileHandle !=nullptr)
				unmount_disk_image(disk);
			Drive[disk].ImageType=RAW;
			Drive[disk].Sides=2;

			Drive[disk].FileHandle = OpenFloppy(Drive[disk].RawDrive-1);
			if (Drive[disk].FileHandle == nullptr)
				return 1;
			strcpy(Drive[disk].ImageName,FileName);
			if (Drive[disk].RawDrive==1)
				PhysicalDriveA=disk+1;
			if (Drive[disk].RawDrive==2)
				PhysicalDriveB=disk+1;
		}
	break;
	default:
		return 1;
	}
	strncpy(Drive[disk].ImageTypeName,ImageFormat[Drive[disk].ImageType],4);
	return 0; //Return 0 on success
}

/********************************************************************************************
* ReadSector returns only the valid sector data with out the low level housekeeping info	*																											 *
* Should work for all formats																*
********************************************************************************************/
long ReadSector (unsigned char Side,	//0 or 1
				 unsigned char Track,	//0 to 255 "REAL" values are 1 to 80
				 unsigned char Sector,	//1 to 18 could be 0 to 17
				 unsigned char *ReturnBuffer)
{
	unsigned long BytesRead=0,Result=0;
	long FileOffset=0;
	unsigned char TempBuffer[16384];
	SectorInfo CurrentSector;
//Needed for RAW access
	DWORD dwRet;
	FD_SEEK_PARAMS sp;
	unsigned char Ret=0;
	const unsigned char *pva=nullptr;
//************************

	if (Drive[CurrentDisk].FileHandle==nullptr)
		return 0;

	switch (Drive[CurrentDisk].ImageType)
	{
		case JVC:
		case VDK:
		case OS9:
			if (((Side+1) > Drive[CurrentDisk].Sides) | (TrackReg != Drive[CurrentDisk].HeadPosition))
				return 0;

			FileOffset= Drive[CurrentDisk].HeaderSize + ( (Track * Drive[CurrentDisk].Sides * Drive[CurrentDisk].TrackSize)+ (Side * Drive[CurrentDisk].TrackSize) + (BytesperSector[Drive[CurrentDisk].SectorSize] * (Sector - Drive[CurrentDisk].FirstSector) ) ) ;
			SetFilePointer(Drive[CurrentDisk].FileHandle,FileOffset,nullptr,FILE_BEGIN);
			ReadFile(Drive[CurrentDisk].FileHandle,ReturnBuffer,BytesperSector[Drive[CurrentDisk].SectorSize],&BytesRead,nullptr);
			if (BytesRead != BytesperSector[Drive[CurrentDisk].SectorSize]) //Fake the read for short images
			{
				memset(ReturnBuffer,0xFF,BytesperSector[Drive[CurrentDisk].SectorSize]); 
				BytesRead=BytesperSector[Drive[CurrentDisk].SectorSize];
			}
			return(BytesperSector[Drive[CurrentDisk].SectorSize]);

		case DMK:	
			FileOffset= Drive[CurrentDisk].HeaderSize + ( (Track * Drive[CurrentDisk].Sides * Drive[CurrentDisk].TrackSize)+ (Side * Drive[CurrentDisk].TrackSize));
			Result=SetFilePointer(Drive[CurrentDisk].FileHandle,FileOffset,nullptr,FILE_BEGIN);

			//Need to read the entire track due to the emulation of interleave on these images
			ReadFile(Drive[CurrentDisk].FileHandle,TempBuffer,Drive[CurrentDisk].TrackSize,&BytesRead,nullptr);
			if (BytesRead!=Drive[CurrentDisk].TrackSize) //DMK can't be short the image is corupt
				return 0;
			CurrentSector.Sector=Sector;
			if (GetSectorInfo (&CurrentSector,TempBuffer)==0)
				return 0;
			memcpy(ReturnBuffer,&TempBuffer[CurrentSector.DAM],CurrentSector.Lenth);
			return(CurrentSector.Lenth);

		case RAW:
			pva=(unsigned char *)RawReadBuf;
			if (TrackReg != Drive[CurrentDisk].HeadPosition)
				return 0;
			//Read the entire track and cache it. Speeds up disk reads 
			if (DirtyDisk | (rwp.phead != Side) | (rwp.cyl != Drive[CurrentDisk].HeadPosition ) )
			{
				rwp.flags = FD_OPTION_MFM;
				rwp.phead = Side;
				rwp.cyl = Drive[CurrentDisk].HeadPosition;
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
				DeviceIoControl(Drive[CurrentDisk].FileHandle , IOCTL_FDCMD_SEEK, &sp, sizeof(sp), nullptr, 0, &dwRet, nullptr);
				Ret=DeviceIoControl(Drive[CurrentDisk].FileHandle , IOCTL_FDCMD_READ_DATA, &rwp, sizeof(rwp), RawReadBuf,4608, &dwRet, nullptr);
				if (dwRet != 4608)
					return 0;
				DirtyDisk=false;
			}

			memcpy(ReturnBuffer,&pva[(Sector-1)*256],256);
			return 256;
	}
	return 0;
}

long WriteSector (	unsigned char Side,		//0 or 1
					unsigned char Track,	//0 to 255 "REAL" values are 1 to 80
					unsigned char Sector,	//1 to 18 could be 0 to 17
				    const unsigned char *WriteBuffer, //)
					long BytestoWrite)
{
	unsigned long BytesWritten=0,Result=0,BytesRead=0;
	unsigned int FileOffset=0;
	unsigned char TempBuffer[16384];
	unsigned short Crc=0xABCD;
//Needed for RAW access
	DWORD dwRet;
	FD_SEEK_PARAMS sp;
	unsigned char Ret=0;
	const unsigned char *pva=nullptr;
	SectorInfo CurrentSector;
	if ( (Drive[CurrentDisk].FileHandle==nullptr) | ((Side+1) > Drive[CurrentDisk].Sides) )
		return 0;

	switch (Drive[CurrentDisk].ImageType)
	{
		case JVC:
		case VDK:
		case OS9:
			FileOffset= Drive[CurrentDisk].HeaderSize + ( (Track * Drive[CurrentDisk].Sides * Drive[CurrentDisk].TrackSize)+ (Side * Drive[CurrentDisk].TrackSize) + (BytesperSector[Drive[CurrentDisk].SectorSize] * (Sector - Drive[CurrentDisk].FirstSector) ) ) ;
			Result=SetFilePointer(Drive[CurrentDisk].FileHandle,FileOffset,nullptr,FILE_BEGIN);
			WriteFile(Drive[CurrentDisk].FileHandle,WriteBuffer,BytesperSector[Drive[CurrentDisk].SectorSize],&BytesWritten,nullptr);
			return BytesWritten;

		case DMK:	//DMK 
			FileOffset= Drive[CurrentDisk].HeaderSize + ( (Track * Drive[CurrentDisk].Sides * Drive[CurrentDisk].TrackSize)+ (Side * Drive[CurrentDisk].TrackSize));
			Result=SetFilePointer(Drive[CurrentDisk].FileHandle,FileOffset,nullptr,FILE_BEGIN);
			//Need to read the entire track due to the emulation of interleave on these images			
			ReadFile(Drive[CurrentDisk].FileHandle,TempBuffer,Drive[CurrentDisk].TrackSize,&BytesRead,nullptr);
			if (BytesRead!=Drive[CurrentDisk].TrackSize)
				return 0;

			CurrentSector.Sector=Sector;
			if (GetSectorInfo(&CurrentSector,TempBuffer)==0)
				return 0;
			
			memcpy(&TempBuffer[CurrentSector.DAM],WriteBuffer,BytestoWrite);
			Crc=ccitt_crc16(0xE295,&TempBuffer[CurrentSector.DAM] , CurrentSector.Lenth); //0xcdb4
			TempBuffer[CurrentSector.DAM + CurrentSector.Lenth  ] =Crc>>8;
			TempBuffer[CurrentSector.DAM + CurrentSector.Lenth +1 ] =(Crc & 0xFF);
			FileOffset= Drive[CurrentDisk].HeaderSize + ( (Track * Drive[CurrentDisk].Sides * Drive[CurrentDisk].TrackSize)+ (Side * Drive[CurrentDisk].TrackSize));
			Result=SetFilePointer(Drive[CurrentDisk].FileHandle,FileOffset,nullptr,FILE_BEGIN);
			WriteFile(Drive[CurrentDisk].FileHandle,TempBuffer,Drive[CurrentDisk].TrackSize,&BytesWritten,nullptr);
			return(CurrentSector.Lenth);

		case RAW:
			DirtyDisk=true;
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
			DeviceIoControl(Drive[CurrentDisk].FileHandle , IOCTL_FDCMD_SEEK, &Track, sizeof(Track), nullptr, 0, &dwRet, nullptr);
			memcpy(RawReadBuf,WriteBuffer,256);
			Ret=DeviceIoControl(Drive[CurrentDisk].FileHandle , IOCTL_FDCMD_WRITE_DATA, &rwp, sizeof(rwp), RawReadBuf,18*(128<<rwp.size), &dwRet, nullptr);
			return dwRet;
	}
	return 0;
}

long WriteTrack (	unsigned char Side,		//0 or 1
					unsigned char Track,	//0 to 255 "REAL" values are 1 to 80
					unsigned char /*Dummy*/,	//Sector Value unused
					const unsigned char *WriteBuffer)
{
	unsigned char xTrack=0,xSide=0,xSector=0,xLenth=0;
	unsigned short BufferIndex=0,WriteIndex=0,IdamIndex=0;
	unsigned long FileOffset=0,Result=0,BytesWritten=0;
	unsigned char *TempBuffer=nullptr;
	unsigned char TempChar=0;
	unsigned short Crc=0,DataBlockPointer=0,Seed=0;	

	if (Drive[CurrentDisk].FileHandle==nullptr)
		return 0;
		
	switch (Drive[CurrentDisk].ImageType)
	{
		case JVC:
		case VDK:
		case OS9:
			for (BufferIndex=0;BufferIndex<6272;BufferIndex++)
			{
				if ( WriteBuffer[BufferIndex]== 0xFE) //Look for ID.A.M.
				{
					xTrack=WriteBuffer[BufferIndex+1];
					xSide=WriteBuffer[BufferIndex+2];
					xSector=WriteBuffer[BufferIndex+3];
					xLenth=WriteBuffer[BufferIndex+4];
					BufferIndex+=6;
				}

				if ( WriteBuffer[BufferIndex]== 0xFB)	//Look for D.A.M.
				{
					BufferIndex++;
					FileOffset= Drive[CurrentDisk].HeaderSize + ( (Track * Drive[CurrentDisk].Sides * Drive[CurrentDisk].TrackSize)+ (Side * Drive[CurrentDisk].TrackSize) + (BytesperSector[Drive[CurrentDisk].SectorSize] * (xSector - 1) ) ) ;
					Result=SetFilePointer(Drive[CurrentDisk].FileHandle,FileOffset,nullptr,FILE_BEGIN);
					WriteFile(Drive[CurrentDisk].FileHandle,&WriteBuffer[BufferIndex],BytesperSector[Drive[CurrentDisk].SectorSize],&BytesWritten,nullptr);				
					BufferIndex+=BytesperSector[Drive[CurrentDisk].SectorSize];
				}
			}
		break;

		case DMK:
			BufferIndex=128;	//Index to cooked buffer
			WriteIndex=0;		// Index to raw Buffer

			TempBuffer=(unsigned char *)malloc(Drive[CurrentDisk].TrackSize);
			memset(TempBuffer,0,Drive[CurrentDisk].TrackSize);
			while (BufferIndex < Drive[CurrentDisk].TrackSize)
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

			FileOffset= Drive[CurrentDisk].HeaderSize + ( (Track * Drive[CurrentDisk].Sides * Drive[CurrentDisk].TrackSize)+ (Side * Drive[CurrentDisk].TrackSize)  ) ;
			Result=SetFilePointer(Drive[CurrentDisk].FileHandle,FileOffset,nullptr,FILE_BEGIN);
			WriteFile(Drive[CurrentDisk].FileHandle,TempBuffer,Drive[CurrentDisk].TrackSize,&BytesWritten,nullptr);				
			free(TempBuffer);
		break;


		case RAW:
			DirtyDisk=true;
			DeviceIoControl(Drive[CurrentDisk].FileHandle , IOCTL_FDCMD_SEEK, &Track, sizeof(Track), nullptr, 0, &dwRet, nullptr);
			return(FormatTrack (Drive[CurrentDisk].FileHandle , Track , Side, WriteBuffer[100] )); //KLUDGE!
	}
	
	return BytesWritten;
}

long ReadTrack (	unsigned char Side,		//0 or 1
					unsigned char Track,	//0 to 255 "REAL" values are 1 to 80
					unsigned char /*Dummy*/,	//Sector Value unused
					unsigned char *WriteBuffer)
{
	unsigned long BytesRead=0,Result=0;
	long FileOffset=0;

	if (Drive[CurrentDisk].FileHandle==nullptr)
		return 0;

	switch (Drive[CurrentDisk].ImageType)
	{
		case JVC:
		case VDK:
		case OS9:
			if ((Side+1) > Drive[CurrentDisk].Sides)
				return 0;
			//STUB Write Me
			return 0;

		case DMK:	
			FileOffset= Drive[CurrentDisk].HeaderSize + ( (Track * Drive[CurrentDisk].Sides * Drive[CurrentDisk].TrackSize)+ (Side * Drive[CurrentDisk].TrackSize)+128);
			Result=SetFilePointer(Drive[CurrentDisk].FileHandle,FileOffset,nullptr,FILE_BEGIN);
			ReadFile(Drive[CurrentDisk].FileHandle,WriteBuffer,(Drive[CurrentDisk].TrackSize-128),&BytesRead,nullptr);
			if (BytesRead != ((unsigned)Drive[CurrentDisk].TrackSize-128) )
				return 0;
		break;
	}
	return BytesRead;
}

//This gets called at the end of every scan line so the controller has acurate timing.
void PingFdc(::vcc::bus::expansion_port_bus& bus)
{
	static char wobble=0;
	if (MotorOn==0)
		return;
	IndexCounter++;
	if (IndexCounter> ((INDEXTIME-30)+wobble))
		IndexPulse=1;
	if (IndexCounter > (INDEXTIME+wobble)) 
	{
		IndexPulse=0;
		IndexCounter=0;
		wobble=rand();
		wobble/=4;
	}

	if ( ((CurrentCommand<=7) | (CurrentCommand==IDLE)) & (Drive[CurrentDisk].FileHandle!=nullptr) )
	{
		if (IndexPulse  )
			StatusReg|=INDEXPULSE;
		else
			StatusReg &=(~INDEXPULSE);
	}
	if (CurrentCommand==IDLE)
		return;
	ExecTimeWaiter--;
	if (ExecTimeWaiter >0)  //Click Click Click
		return;
	
	ExecTimeWaiter=1;
	IOWaiter++;

	switch (CurrentCommand)
	{
		case RESTORE:	
			if (Drive[CurrentDisk].HeadPosition!=0)
			{
				Drive[CurrentDisk].HeadPosition-=1;
				ExecTimeWaiter=(CyclesperStep * StepTimeMS);
				if (Drive[CurrentDisk].ImageType==RAW)
					DeviceIoControl(Drive[CurrentDisk].FileHandle , IOCTL_FDCMD_SEEK, nullptr, 1, nullptr, 0, &dwRet, nullptr);
			}
			else
			{
				TrackReg=0;
				StatusReg=READY;
				StatusReg|=TRACK_ZERO;
				Drive[CurrentDisk].HeadPosition=0;
				if (Drive[CurrentDisk].WriteProtect)
					StatusReg|=WRITEPROTECT;
				CommandDone(bus);
			}
		break;

		case SEEK:	
			if (Drive[CurrentDisk].HeadPosition!=DataReg)
			{
				if (StepDirection==1)
					Drive[CurrentDisk].HeadPosition+=1;
				else
					Drive[CurrentDisk].HeadPosition-=1;
				ExecTimeWaiter=(CyclesperStep * StepTimeMS);
			}
			else
			{			
				Drive[CurrentDisk].HeadPosition=TrackReg;
				StatusReg=READY;
				if (Drive[CurrentDisk].HeadPosition==0)
					StatusReg|=TRACK_ZERO;
				if (Drive[CurrentDisk].WriteProtect)
					StatusReg|=WRITEPROTECT;
				CommandDone(bus);
			}
			if (Drive[CurrentDisk].ImageType==RAW)
				DeviceIoControl(Drive[CurrentDisk].FileHandle , IOCTL_FDCMD_SEEK, &TrackReg, sizeof(TrackReg), nullptr, 0, &dwRet, nullptr);
		break;

		case STEP:
		case STEPUPD:	
			if (StepDirection==1)
					Drive[CurrentDisk].HeadPosition+=1;
			else
			{
				if (Drive[CurrentDisk].HeadPosition >0)
					Drive[CurrentDisk].HeadPosition-=1;
			}

			if (CurrentCommand & 1)
			{
				if (StepDirection==1)
					TrackReg+=1;
				else
					TrackReg-=1;
			}
			if ( Drive[CurrentDisk].HeadPosition == 0)
				StatusReg=TRACK_ZERO;
			if (Drive[CurrentDisk].WriteProtect)
				StatusReg|=WRITEPROTECT;
			StatusReg=READY;
			CommandDone(bus);
		break;

		case STEPIN:
		case STEPINUPD:	
			StepDirection=1;
			Drive[CurrentDisk].HeadPosition+=1;
			if (CurrentCommand & 1)
				TrackReg+=1;
			if (Drive[CurrentDisk].WriteProtect)
				StatusReg|=WRITEPROTECT;
			StatusReg=READY;
			CommandDone(bus);
		break;

		case STEPOUT:
		case STEPOUTUPD:	
			StepDirection=0;
			if ( Drive[CurrentDisk].HeadPosition > 0 )
				Drive[CurrentDisk].HeadPosition-=1;
			if (CurrentCommand & 1)
				TrackReg-=1;
			if ( Drive[CurrentDisk].HeadPosition == 0)
				StatusReg=TRACK_ZERO;
			if (Drive[CurrentDisk].WriteProtect)
				StatusReg|=WRITEPROTECT;
			StatusReg=READY;
			CommandDone(bus);
		break;

		case READSECTOR:
		case READSECTORM:	
			if (IOWaiter>WAITTIME)
			{
				LostDataFlag=1;
				GetBytefromSector(bus);
			}
		break;

		case WRITESECTOR:
		case WRITESECTORM: 
			StatusReg |=DRQ;	//IRON
			if (IOWaiter>WAITTIME)
			{
				LostDataFlag=1;
//				WriteLog("WRITESECTOR TIMEOUT",0);
				WriteBytetoSector(bus, 0);
			}
		break;

		case READADDRESS: 
			if (IOWaiter>WAITTIME)
			{
				LostDataFlag=1;
				GetBytefromAddress (bus);	
			}
		break;

		case FORCEINTERUPT: 
		break;

		case READTRACK:
			if (IOWaiter>WAITTIME)
			{
				LostDataFlag=1;
				GetBytefromTrack(bus);
			}
		break;

		case WRITETRACK:
			StatusReg|=DRQ;	//IRON
			if (IOWaiter>WAITTIME)
			{
				LostDataFlag=1;
				WriteBytetoTrack(bus, 0);
			}
		break;

		case IDLE:
		break;
	}
	return;
}

void DispatchCommand(::vcc::bus::expansion_port_bus& bus, unsigned char Tmp)
{
	unsigned char Command= (Tmp >>4);
	if ( (CurrentCommand !=IDLE) & (Command != 13) ) 
		return;
	CurrentCommand=Command;
	switch (Command)
	{
		case RESTORE:	
			SetType1Flags(Tmp);
			ExecTimeWaiter= CyclestoSettle + (CyclesperStep * StepTimeMS);
			StatusReg= BUSY;
//			WriteLog("RESTORE",0);
			break;

		case SEEK:	
			SetType1Flags(Tmp);
			ExecTimeWaiter= CyclestoSettle + (CyclesperStep * StepTimeMS);
			TrackReg=DataReg;
			StatusReg= BUSY;
			if ( Drive[CurrentDisk].HeadPosition > DataReg)
				StepDirection=0;
			else
				StepDirection=1;
//			WriteLog("SEEK",0);
			break;

		case STEP:
		case STEPUPD:	
			SetType1Flags(Tmp);
			ExecTimeWaiter= CyclestoSettle + (CyclesperStep * StepTimeMS);
			StatusReg= BUSY;
	//		WriteLog("STEP",0);
			break;

		case STEPIN:
		case STEPINUPD:	
			SetType1Flags(Tmp);
			ExecTimeWaiter= CyclestoSettle + (CyclesperStep * StepTimeMS);
			StatusReg= BUSY;
//			WriteLog("STEPIN",0);
			break;

		case STEPOUT:
		case STEPOUTUPD:	
			SetType1Flags(Tmp);
			ExecTimeWaiter= CyclestoSettle + (CyclesperStep * StepTimeMS);
			StatusReg= BUSY;
//			WriteLog("STEPOUT",0);
			break;

		case READSECTOR:
		case READSECTORM:
			SetType2Flags(Tmp);
			TransferBufferIndex=0;
			StatusReg= BUSY | DRQ;
			GetBytefromDisk=&GetBytefromSector;
			ExecTimeWaiter= 1;
			IOWaiter=0;
			MSectorFlag=Command & 1;
//			WriteLog("READSECTOR",0);
			break;

		case WRITESECTOR:
		case WRITESECTORM: 
//			WriteLog("Getting WriteSector Command",0);
			SetType2Flags(Tmp);
			TransferBufferIndex=0;
			StatusReg= BUSY; //IRON
			WriteBytetoDisk=&WriteBytetoSector;
			ExecTimeWaiter=5;
			IOWaiter=0;
			MSectorFlag=Command & 1;
//			WriteLog("WRITESECTOR",0);
			break;

		case READADDRESS: 
//			MessageBox(0,"Hitting Get Address","Ok",0);
			SetType3Flags(Tmp);
			TransferBufferIndex=0;
			StatusReg= BUSY | DRQ;
			GetBytefromDisk=&GetBytefromAddress;
			ExecTimeWaiter=1;
			IOWaiter=0;

			break;

		case FORCEINTERUPT: 
			CurrentCommand=IDLE;
			TransferBufferSize=0;
			StatusReg=READY;
			ExecTimeWaiter=1;
			if ((Tmp & 15) != 0)
				bus.assert_nmi_interrupt_line();
//			WriteLog("FORCEINTERUPT",0);
			break;

		case READTRACK:
//			WriteLog("Hitting ReadTrack",0);
			SetType3Flags(Tmp);
			TransferBufferIndex=0;
			StatusReg= BUSY | DRQ;
			GetBytefromDisk=&GetBytefromTrack;
			ExecTimeWaiter=1;
			IOWaiter=0;
			break;

		case WRITETRACK:
//			WriteLog("Getting WriteTrack Command",0);
			SetType3Flags(Tmp);
			TransferBufferIndex=0;
			StatusReg= BUSY ;		//IRON
			WriteBytetoDisk=&WriteBytetoTrack;
			ExecTimeWaiter=1;
			ExecTimeWaiter=5;		//IRON
			IOWaiter=0;
			break;
	}
	return;
}

unsigned char GetBytefromSector (::vcc::bus::expansion_port_bus& bus)
{
	unsigned char RetVal=0;

	if (TransferBufferSize == 0)
	{
		TransferBufferSize = ReadSector(Side,Drive[CurrentDisk].HeadPosition,SectorReg,TransferBuffer);
//		WriteLog("BEGIN Readsector",0);
	}

	if (TransferBufferSize==0)// IRON| (TrackReg != Drive[CurrentDrive].HeadPosition) ) //| (SectorReg > Drive[CurrentDrive].Sectors)
	{
		CommandDone(bus);
		StatusReg=RECNOTFOUND;
		return 0;
	}

	if (TransferBufferIndex < TransferBufferSize)
	{
		RetVal=TransferBuffer[TransferBufferIndex++];
		StatusReg= BUSY | DRQ;
		IOWaiter=0;
//		WriteLog("Transfering 1 Byte",0);
	}
	else
	{
//		WriteLog("READSECTOR DONE",0);
		StatusReg=READY;
		CommandDone(bus);
		if (LostDataFlag==1)
		{
			StatusReg=LOSTDATA;
			LostDataFlag=0;
		}
		SectorReg++;
	}
	return RetVal;
}

unsigned char GetBytefromAddress (::vcc::bus::expansion_port_bus& bus)
{
	unsigned char RetVal=0;
	unsigned short Crc=0;

	if (TransferBufferSize == 0)
	{
		switch (Drive[CurrentDisk].ImageType)
		{
			case JVC:
			case VDK:
			case OS9:
				TransferBuffer[0]= Drive[CurrentDisk].HeadPosition;
				TransferBuffer[1]= Drive[CurrentDisk].Sides;
				TransferBuffer[2]= IndexCounter/176;
				TransferBuffer[3]= Drive[CurrentDisk].SectorSize;
				TransferBuffer[4]= (Crc >> 8);	
				TransferBuffer[5]= (Crc & 0xFF);
				TransferBufferSize=6;
			break;

			case DMK:
				TransferBuffer[0]= Drive[CurrentDisk].HeadPosition; //CurrentSector.Track; not right need to get from image
				TransferBuffer[1]= Drive[CurrentDisk].Sides;
				TransferBuffer[2]= IndexCounter/176;
				TransferBuffer[3]= IndexCounter/176; //Drive[CurrentDrive].SectorSize;
				Crc = 0;//CurrentSector.CRC;
				TransferBuffer[4]= (Crc >> 8);	
				TransferBuffer[5]= (Crc & 0xFF);
				TransferBufferSize=6;				
			break;
		} //END Switch
	}

	if (   Drive[CurrentDisk].FileHandle==nullptr  )
	{
		StatusReg=RECNOTFOUND;
		CommandDone(bus);
		return 0;
	}

	if (TransferBufferIndex < TransferBufferSize)
	{
		RetVal=TransferBuffer[TransferBufferIndex++];
		StatusReg= BUSY | DRQ;
		IOWaiter=0;
	}
	else
	{
		StatusReg=READY;
		CommandDone(bus);
		if (LostDataFlag==1)
		{
			StatusReg=LOSTDATA;
			LostDataFlag=0;
		}
	}
	return RetVal;
}

unsigned char GetBytefromTrack (::vcc::bus::expansion_port_bus& bus)
{
	unsigned char RetVal=0;

	if (TransferBufferSize == 0)
	{
		TransferBufferSize = ReadTrack(Side,Drive[CurrentDisk].HeadPosition,SectorReg,TransferBuffer);
//		WriteLog("BEGIN READTRACK",0);
	}

	if (TransferBufferSize==0)//iron | (TrackReg != Drive[CurrentDrive].HeadPosition) ) //| (SectorReg > Drive[CurrentDrive].Sectors)
	{
		CommandDone(bus);
		StatusReg=RECNOTFOUND;
		return 0;
	}

	if (TransferBufferIndex < TransferBufferSize)
	{
		RetVal=TransferBuffer[TransferBufferIndex++];
		StatusReg= BUSY | DRQ;
		IOWaiter=0;
	}
	else
	{
//		WriteLog("READTRACK DONE",0);
		StatusReg=READY;
		CommandDone(bus);
		if (LostDataFlag==1)
		{
			StatusReg=LOSTDATA;
			LostDataFlag=0;
		}
		SectorReg++;
	}
	return RetVal;
}

unsigned char WriteBytetoSector (::vcc::bus::expansion_port_bus& bus, unsigned char Tmp)
{
	unsigned long BytesRead=0,Result=0;
	long FileOffset=0,RetVal=0;
	unsigned char TempBuffer[16384];
	SectorInfo CurrentSector;

	if (TransferBufferSize==0)
	{
//		WriteLog("Begining WriteSector data collection Command",0);
		switch (Drive[CurrentDisk].ImageType)
		{
			case JVC:
			case VDK:
			case OS9:
				TransferBufferSize=BytesperSector[Drive[CurrentDisk].SectorSize] ; 
				if ((TransferBufferSize==0)  | (TrackReg != Drive[CurrentDisk].HeadPosition) | (SectorReg > Drive[CurrentDisk].Sectors) )
				{
					StatusReg=RECNOTFOUND;
					CommandDone(bus);
					return 0;
				}
			break;

			case DMK:
				FileOffset= Drive[CurrentDisk].HeaderSize + ( (Drive[CurrentDisk].HeadPosition * Drive[CurrentDisk].Sides * Drive[CurrentDisk].TrackSize)+ (Side * Drive[CurrentDisk].TrackSize));
				Result=SetFilePointer(Drive[CurrentDisk].FileHandle,FileOffset,nullptr,FILE_BEGIN);
				ReadFile(Drive[CurrentDisk].FileHandle,TempBuffer,Drive[CurrentDisk].TrackSize,&BytesRead,nullptr);
				CurrentSector.Sector=SectorReg;
				GetSectorInfo(&CurrentSector,TempBuffer);
				if ( (CurrentSector.DAM == 0) | (BytesRead != Drive[CurrentDisk].TrackSize) )
				{
					StatusReg=RECNOTFOUND;
					CommandDone(bus);
					return 0;
				}
				TransferBufferSize = CurrentSector.Lenth;
			break;

			case RAW:
				TransferBufferSize = 256;
			break;
		}	//END Switch
	}	//END if

	if (TransferBufferIndex>=TransferBufferSize) 
	{
		RetVal=WriteSector(Side,Drive[CurrentDisk].HeadPosition,SectorReg,TransferBuffer,TransferBufferSize);
		StatusReg=READY;
		if (( RetVal==0) | (LostDataFlag==1) )
			StatusReg=LOSTDATA;
		if (Drive[CurrentDisk].WriteProtect != 0)
			StatusReg=WRITEPROTECT | RECNOTFOUND;
		CommandDone(bus);	
		LostDataFlag=0;
		SectorReg++;
	}
	else
	{
		TransferBuffer[TransferBufferIndex++]=Tmp;
		StatusReg= BUSY | DRQ;
		IOWaiter=0;
	}
	return 0;
}
unsigned char WriteBytetoTrack (::vcc::bus::expansion_port_bus& bus, unsigned char Tmp)
{
	long RetVal=0;
	if (TransferBufferSize==0)
		TransferBufferSize=6272 ;


	if (TransferBufferIndex>=TransferBufferSize) 
	{
		RetVal=WriteTrack(Side,Drive[CurrentDisk].HeadPosition,SectorReg,TransferBuffer);

		StatusReg=READY;
		if (( RetVal==0) | (LostDataFlag==1) )
			StatusReg=LOSTDATA;
		if (Drive[CurrentDisk].WriteProtect != 0)
			StatusReg=WRITEPROTECT | RECNOTFOUND;
		CommandDone(bus);
		LostDataFlag=0;
	}
	else
	{
		TransferBuffer[TransferBufferIndex++]=Tmp;
		StatusReg= BUSY | DRQ;
		IOWaiter=0;
	}
	return 0;
}

void SetType1Flags(unsigned char Tmp)
{
	Tmp&=15;
	HeadLoad= (Tmp>>3)&1;
	TrackVerify= (Tmp>>2)&1;
	StepTimeMS = StepTimesMS[Tmp & 3];
	return;
}

void SetType2Flags(unsigned char Tmp)
{
	Tmp&=15;
	SideCompare= (Tmp>>3)&1;
	SideCompareEnable=(Tmp>>1)&1;
	Delay15ms=(Tmp>>2)&1;
	return;
}

void SetType3Flags(unsigned char Tmp)
{
	Delay15ms=(Tmp>>2)&1;
	return;
}

unsigned char SetTurboDisk( unsigned char Tmp)
{
	if (Tmp!=QUERY)
	{
		TurboMode=Tmp&1;
		switch (TurboMode)
		{
		case 0:
			CyclestoSettle=SETTLETIME;
			CyclesperStep=STEPTIME;
			break;
		case 1:
			CyclestoSettle=1;
			CyclesperStep=0;
			break;
		}
	}
	return TurboMode;
}

long GetSectorInfo (SectorInfo *Sector,const unsigned char *TempBuffer)
{
	unsigned short Temp1=0,Temp2=0;
	unsigned char Density=0;
	long IdamIndex=0;

	do		//Scan IDAM table for correct sector
	{
		Temp1= (TempBuffer[IdamIndex+1]<<8) | TempBuffer[IdamIndex];	//Reverse Bytes and Get the pointer 
		Density= Temp1>>15;												//Density flag
		Temp1=  (0x3FFF & Temp1) % Drive[CurrentDisk].TrackSize;				//Mask and keep from overflowing
		IdamIndex+=2;
	}
	while ( (IdamIndex<128) & (Temp1 !=0) & (Sector->Sector != TempBuffer[Temp1+3]) );
		
	if ((Temp1 ==0) | (IdamIndex>128) )
		return 0;		//Can't Find the Specified Sector on this track

	//At this point Temp1 should be pointing to the IDAM $FE for the track in question
	Sector->Track = TempBuffer[Temp1+1];
	Sector->Side = TempBuffer[Temp1+2];
	Sector->Sector = TempBuffer[Temp1+3];
	Sector->Lenth = BytesperSector[ TempBuffer[Temp1+4] & 3]; //128 256 512 1024
	Sector->CRC = (TempBuffer[Temp1+5]<<8) | TempBuffer[Temp1+6] ;
	Sector->Density = Density;
	if ( (Sector->Track != TrackReg) | (Sector->Sector != SectorReg) )
		return 0; 
	
	Temp1+=7;				//Ignore the CRC
	Temp2=Temp1+43;			//Scan for D.A.M.
	while ( (TempBuffer[Temp1] !=0xFB) & (Temp1<Temp2) )
		Temp1++;
	if (Temp1==Temp2)		//Can't find data address mark
		return 0;
	Sector->DAM = Temp1+1;
	Sector->CRC = ccitt_crc16(0xE295, &TempBuffer[Sector->DAM], Sector->Lenth);
	return(Sector->DAM);	//Return Pointer to Start of sector data

}

void CommandDone(::vcc::bus::expansion_port_bus& bus)
{
	if (InteruptEnable)
		bus.assert_nmi_interrupt_line();
	TransferBufferSize=0;
	CurrentCommand=IDLE;
}

//Stolen from MESS
/*
   Compute CCITT CRC-16 using the correct bit order for floppy disks.
   CRC code courtesy of Tim Mann.
*/

/* Accelerator table to compute the CRC eight bits at a time */
static const unsigned short ccitt_crc16_table[256] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

unsigned short ccitt_crc16(unsigned short crc, const unsigned char *buffer, unsigned short buffer_len)
{
	unsigned short i;
	for (i = 0; i < buffer_len; i++)
		crc = (crc << 8) ^ ccitt_crc16_table[(crc >> 8) ^ buffer[i]];
	return crc;
}

//Stolen from fdrawcmd.sys Demo Disk Utility by Simon Owen <simon@simonowen.com>

DWORD GetDriverVersion ()
{
    DWORD dwVersion = 0;
    HANDLE h = CreateFile("\\\\.\\fdrawcmd", GENERIC_READ|GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

    if (h != INVALID_HANDLE_VALUE)
    {
        DeviceIoControl(h, IOCTL_FDRAWCMD_GET_VERSION, nullptr, 0, &dwVersion, sizeof(dwVersion), &dwRet, nullptr);
        CloseHandle(h);
    }
    return dwVersion;
}


HANDLE OpenFloppy (int nDrive_)
{
    char szDevice[32],szTemp[128]="";
	HANDLE h=nullptr;
    wsprintf(szDevice, "\\\\.\\fdraw%u", nDrive_);
    h = CreateFile(szDevice, GENERIC_READ|GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (h == INVALID_HANDLE_VALUE)
	{
		sprintf(szTemp,"Unable to open RAW device %s",szDevice);
		MessageBox(nullptr,szTemp,"Ok",0);
	}
    return (h != INVALID_HANDLE_VALUE && SetDataRate(h, DISK_DATARATE)) ? h : nullptr;
}

bool SetDataRate (HANDLE h_, BYTE bDataRate_)
{
    return !!DeviceIoControl(h_,IOCTL_FD_SET_DATA_RATE,&bDataRate_,sizeof(bDataRate_),nullptr,0,&dwRet,nullptr);
}

bool FormatTrack (HANDLE h_, BYTE cyl_, BYTE head_, BYTE Fill)
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
        ph->cyl = cyl_;
        ph->head = head_;
        ph->sector = SECTOR_BASE + ((s + cyl_*(pfp->sectors - TRACK_SKEW)) % pfp->sectors);
        ph->size = pfp->size;
    }

    return CmdFormat(h_, pfp, (PBYTE)ph - abFormat);
}

bool CmdFormat (HANDLE h_, PFD_FORMAT_PARAMS pfp_, ULONG ulSize_)
{
    return !!DeviceIoControl(h_, IOCTL_FDCMD_FORMAT_TRACK, pfp_, ulSize_, nullptr, 0, &dwRet, nullptr);
}

unsigned short InitController ()
{
	long RawDriverVersion=0;
	RawDriverVersion=GetDriverVersion ();

	if (RawReadBuf==nullptr)
		RawReadBuf = VirtualAlloc(nullptr, 4608, MEM_COMMIT, PAGE_READWRITE);
	if (RawReadBuf==nullptr)
		return 0;

	return 1;
}
