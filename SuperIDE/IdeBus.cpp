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

#include "logger.h"
#include "idebus.h"
#include <stdio.h>
#include <windows.h>

static unsigned int Lba=0,LastLba;
char Message[256]="";	//DEBUG
static unsigned char XferBuffer[512]="";

static char *BigBuffer=nullptr;
static unsigned int BufferIndex=0;
static unsigned int BufferLenth=0;
static unsigned char CurrentCommand=0;
void ExecuteCommand(void);
void ByteSwap (char *);
static IDEINTERFACE Registers;
static HANDLE hDiskFile[2];
static unsigned char DiskSelect=0,LbaEnabled=0;
unsigned long BytesMoved=0;
unsigned char BusyCounter=BUSYWAIT;
HANDLE OpenDisk(char *,unsigned char);
static unsigned short IDBlock[2][256];
static unsigned char Mounted=0,ScanCount=0;
static char CurrStatus[32]="IDE:Idle ";
static char FileNames[2][MAX_PATH]={"",""};
void IdeInit()
{

	hDiskFile[MASTER]=INVALID_HANDLE_VALUE;
	hDiskFile[SLAVE]=INVALID_HANDLE_VALUE;
	Registers.Command=0;
	Registers.Cylinderlsb=0;
	Registers.Cylindermsb=0;
	Registers.Data=0;
	Registers.Error[MASTER]=0;
	Registers.Error[SLAVE]=0;
	Registers.Head=0;
	Registers.SecCount=0;
	Registers.SecNumber=0;
	Registers.Status[MASTER]=0;	
	Registers.Status[SLAVE]=0;
	Lba=0;
	BufferIndex=0;
	CurrentCommand=0;
	BufferLenth=0;
	memset(XferBuffer,0,512);
	return;
}

void IdeRegWrite(unsigned char Reg,unsigned short Data)
{
	unsigned char SData=(unsigned char)Data&0xFF;
	BOOL bResult=0;

	switch (Reg)
	{
		case 0x0:
			if (!CurrentCommand)
				return;
			Registers.Data=Data;
			XferBuffer[BufferIndex]= Registers.Data & 0xFF;
			XferBuffer[BufferIndex+1]=(Registers.Data>>8)&0xFF;
			BufferIndex+=2;
			if (BufferIndex>=(BufferLenth))
			{
				if ((CurrentCommand==0x30) | (CurrentCommand==0x31))
				{
					SetFilePointer(hDiskFile[DiskSelect],Lba*512,0,FILE_BEGIN);
					WriteFile(hDiskFile[DiskSelect],XferBuffer,512,&BytesMoved,nullptr);
				}
				BufferIndex=0;
				BufferLenth=0;
				CurrentCommand=0;
				Registers.Status[DiskSelect]=RDY;
				Registers.Error[DiskSelect]=0;
			}
			break;
		case 0x1:
		//	Registers.Error=SData; //Write Precomp on write not used
			break;
		case 0x2:
			Registers.SecCount=SData;
			break;
		case 0x3:
			Registers.SecNumber=SData;
			break;
		case 0x4:
			Registers.Cylinderlsb=SData;
			break;
		case 0x5:
			Registers.Cylindermsb=SData;
			break;
		case 0x6:
			Registers.Head=SData;
			break;
		case 0x7:
			Registers.Command=SData;
			ExecuteCommand();
			break;

		default:
			break;
		}	//End port switch	
		Lba=((Registers.Head&15)<<24)+(Registers.Cylindermsb<<16)+(Registers.Cylinderlsb<<8)+Registers.SecNumber;
		DiskSelect=(Registers.Head >>4)&1;
		LbaEnabled=(Registers.Head >>6)&1;
		//LBa = [ (Cylinder * No of Heads + Heads) * Sectors/Track ] + (Sector-1) //CHS translation
}

unsigned short IdeRegRead(unsigned char Reg)
{
	unsigned short RetVal=0;

	switch (Reg)
	{
		case 0x0:
			if (!CurrentCommand)
				return(0);
			Registers.Data=XferBuffer[BufferIndex] + (XferBuffer[BufferIndex+1]<<8);
			BufferIndex+=2;
			if (BufferIndex>=BufferLenth)
			{
				BufferIndex=0;
				BufferLenth=0;
				CurrentCommand=0;
				Registers.Status[DiskSelect]=RDY;
				Registers.Error[DiskSelect]=0;
			}
			RetVal=Registers.Data;
			break;
		case 0x1:
			RetVal=Registers.Error[DiskSelect];
			break;
		case 0x2:
			RetVal=Registers.SecCount;
			break;
		case 0x3:
			RetVal=Registers.SecNumber;
			break;
		case 0x4:
			RetVal=Registers.Cylinderlsb;
			break;
		case 0x5:
			RetVal=Registers.Cylindermsb;
			break;
		case 0x6:
			RetVal=Registers.Head;
			break;
		case 0x7:
			if (BusyCounter)
			{
				BusyCounter--;
				return(BUSY);
			}
			RetVal=Registers.Status[DiskSelect];
			break;

		default:
			RetVal=0;
			break;
		}	//End port switch
	return(RetVal);
}

void ExecuteCommand(void)
{
	CurrentCommand=Registers.Command;
	char Temp=0;
	switch (Registers.Command)
	{
			
		case 0x90:	//Diagnostics
			for (Temp=0;Temp<=1;Temp++)
			{
				Registers.Error[Temp]=0;
				Registers.Status[Temp]=0;
				if (hDiskFile[Temp]!=INVALID_HANDLE_VALUE)
				{
					Registers.Error[Temp]=0x01;
					Registers.Status[Temp]=0x50;
				}
			}
			break;

		case 0xEC:	//Indentify Drive
			memcpy(XferBuffer,IDBlock[DiskSelect],512);
			BufferLenth=512;
			BufferIndex=0;
			Registers.Status[DiskSelect]=DRQ|RDY;
			BusyCounter=BUSYWAIT;
			break;

		case 0x20:	//Read Sectors /w Retry
		case 0x21:	//Read Sectors /wo Retry
			sprintf(CurrStatus,"IDE: Rd Sec %000000.6X",Lba);
			BusyCounter=BUSYWAIT;
			BufferLenth=512;
			BufferIndex=0;
			Registers.Status[DiskSelect]=DRQ|RDY;
			SetFilePointer(hDiskFile[DiskSelect],Lba*512,0,FILE_BEGIN);
			memset(XferBuffer,0,512);
			ReadFile(hDiskFile[DiskSelect],XferBuffer,512,&BytesMoved,nullptr);
			LastLba=Lba;

			break;

		case 0x30:	//Write Sectors /w Retry
		case 0x31:	//Write Sectors /wo Retry
			sprintf(CurrStatus,"IDE: Wr Sec %000000.6X",Lba);
			BusyCounter=BUSYWAIT;
			BufferLenth=512;
			BufferIndex=0;
			Registers.Status[DiskSelect]=DRQ|RDY;
			memset(XferBuffer,0,512);
			break;
		case 0x50:	//Format Track

			break;
		case 0x10:	//All Recalibrate
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x18:
		case 0x19:
		case 0x1A:
		case 0x1B:
		case 0x1C:
		case 0x1D:
		case 0x1E:
		case 0x1F:
			break;

		default:
			break;
	}
}

void ByteSwap (char *String)
{
	int Index=0,Index2=0;
	int Lenth=strlen(String);
	char *NewString=(char*)malloc(Lenth+1);
	memset(NewString, 0, Lenth + 1);

	for (Index=0;Index<(Lenth+1);Index++)
		if (String[Index^1])
			NewString[Index2++]=String[Index^1];
	memcpy(String,NewString,Lenth);
	free(NewString);
	return;
}

HANDLE OpenDisk(char *ImageFile,unsigned char DiskNum)
{
	HANDLE hTemp=nullptr;
	unsigned long FileSize=0;
	unsigned long LbaSectors=0;
	char Model[41]="VCC VIRTUAL IDE DISK";
	char Firmware[9]="0.99BETA";
	char SerialNumber[21]="SERIAL NUMBER HERE";


	strncpy(SerialNumber,ImageFile,20);

	hTemp=CreateFile( ImageFile,GENERIC_READ | GENERIC_WRITE,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if (hTemp==INVALID_HANDLE_VALUE)
		return(hTemp);

	Registers.Status[DiskNum]=RDY;
	Registers.Error[DiskNum]=0;
	FileSize=SetFilePointer(hTemp,0,0,FILE_END);
	LbaSectors=FileSize>>9;
	//Build Disk ID return Buffer
	ByteSwap(Model);
	ByteSwap(Firmware);
	ByteSwap(SerialNumber);
	memcpy (&IDBlock[DiskNum][27],Model,strlen(Model));
	memcpy (&IDBlock[DiskNum][23],Firmware,strlen(Firmware));
	memcpy (&IDBlock[DiskNum][10],SerialNumber,strlen(SerialNumber));
	IDBlock[DiskNum][01]=(unsigned short)(LbaSectors/0x1000);	//Logical Cylinders
	IDBlock[DiskNum][03]=0x0010;	//Logical Heads
	IDBlock[DiskNum][04]=0x2000;	//Logical Bytes per Track
	IDBlock[DiskNum][05]=512;		//Logical Bytes per Sector
	IDBlock[DiskNum][06]=0x0100;	//Logical Sectors per Track
	IDBlock[DiskNum][20]=1;			//Controller Type
	IDBlock[DiskNum][21]=1;			//# of 512Byte Buffers
	IDBlock[DiskNum][49]=0x0200;	//Bit 9=1 LBA Mode supported
	IDBlock[DiskNum][51]=0x0A00;	//PIO Cycle Time Bits 15-8
	IDBlock[DiskNum][54]=(unsigned short)(LbaSectors/0x1000);	//Cylinders
	IDBlock[DiskNum][55]=0x0010;	//Number of Heads
	IDBlock[DiskNum][56]=0x0100;	//Sectors per Track
	IDBlock[DiskNum][61]=(unsigned short)(LbaSectors>>16);		//LBA Sectors
	IDBlock[DiskNum][60]=(unsigned short)(LbaSectors & 0xFFFF);
	return(hTemp);
}

void DiskStatus(char *Temp)
{
	strcpy(Temp,CurrStatus);
	ScanCount++;
	if (Lba != LastLba)
	{
		ScanCount=0;
		LastLba=Lba;
	}
	if (ScanCount > 63)
	{
		ScanCount=0;
		if (Mounted==1)
			strcpy(CurrStatus,"IDE:Idle");
		else
			strcpy(CurrStatus,"IDe:No Image!");
	}
	return;
}
unsigned char MountDisk(char *FileName,unsigned char DiskNumber)
{
	if (DiskNumber>1)
		return(FALSE);
	hDiskFile[DiskNumber]=OpenDisk( FileName, DiskNumber);
	if (hDiskFile[DiskNumber]==INVALID_HANDLE_VALUE)
		return(FALSE);
	strcpy(FileNames[DiskNumber],FileName);
	Mounted=1;
	return(TRUE);
}

unsigned char DropDisk(unsigned char DiskNumber)
{
	CloseHandle(hDiskFile[DiskNumber]);
	hDiskFile[DiskNumber]=INVALID_HANDLE_VALUE;
	strcpy(FileNames[DiskNumber],"");
	Mounted=0;
	return(TRUE);
}

void QueryDisk(unsigned char DiskNumber,char *Name)
{
	strcpy(Name,FileNames[DiskNumber]);
	return;
}