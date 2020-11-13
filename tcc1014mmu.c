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

#include "windows.h"
#include "defines.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "tcc1014mmu.h"
#include "iobus.h"
//#include "cc3rom.h"
#include "config.h"
#include "tcc1014graphics.h"
#include "pakinterface.h"
#include "logger.h"
#include "hd6309.h"
#include "fileops.h"
static unsigned char *MemPages[1024];
static unsigned short MemPageOffsets[1024];
static unsigned char *memory=NULL;	//Emulated RAM
static unsigned char *InternalRomBuffer=NULL;
static unsigned char MmuTask=0;		// $FF91 bit 0
static unsigned char MmuEnabled=0;	// $FF90 bit 6
static unsigned char RamVectors=0;	// $FF90 bit 3
static unsigned char MmuState=0;	// Composite variable handles MmuTask and MmuEnabled
static unsigned char RomMap=0;		// $FF90 bit 1-0
static unsigned char MapType=0;		// $FFDE/FFDF toggle Map type 0 = ram/rom
static unsigned short MmuRegisters[4][8];	// $FFA0 - FFAF
static unsigned int MemConfig[4]={0x20000,0x80000,0x200000,0x800000};
static unsigned short RamMask[4]={15,63,255,1023};
static unsigned char StateSwitch[4]={8,56,56,56};
static unsigned char VectorMask[4]={15,63,63,63};
static unsigned char VectorMaska[4]={12,60,60,60};
static unsigned int VidMask[4]={0x1FFFF,0x7FFFF,0x1FFFFF,0x7FFFFF};
static unsigned char CurrentRamConfig=1;
static unsigned short MmuPrefix=0;

void UpdateMmuArray(void);
/*****************************************************************************************
* MmuInit Initilize and allocate memory for RAM Internal and External ROM Images.        *
* Copy Rom Images to buffer space and reset GIME MMU registers to 0                      *
* Returns NULL if any of the above fail.                                                 *
*****************************************************************************************/
unsigned char * MmuInit(unsigned char RamConfig)
{
	unsigned int RamSize=0;
	unsigned int Index1=0;
	RamSize=MemConfig[RamConfig];
	CurrentRamConfig=RamConfig;
	if (memory != NULL)
		free(memory);

	memory=(unsigned char *)malloc(RamSize);
	if (memory==NULL)
		return(NULL);
	for (Index1=0;Index1<RamSize;Index1++)
	{
		if (Index1 & 1)
			memory[Index1]=0;
		else 
			memory[Index1]=0xFF;
	}
	SetVidMask(VidMask[CurrentRamConfig]);
	if (InternalRomBuffer != NULL)
		free(InternalRomBuffer);
	InternalRomBuffer=NULL;
	InternalRomBuffer=(unsigned char *)malloc(0x8000);

	if (InternalRomBuffer == NULL)
		return(NULL);

	memset(InternalRomBuffer,0xFF,0x8000);
	CopyRom();
	MmuReset();
	return(memory);
}

void MmuReset(void)
{
	unsigned int Index1=0,Index2=0;
	MmuTask=0;
	MmuEnabled=0;
	RamVectors=0;
	MmuState=0;
	RomMap=0;
	MapType=0;
	MmuPrefix=0;
	for (Index1=0;Index1<8;Index1++)
		for (Index2=0;Index2<4;Index2++)
			MmuRegisters[Index2][Index1]=Index1+StateSwitch[CurrentRamConfig];

	for (Index1=0;Index1<1024;Index1++)
	{
		MemPages[Index1]=memory+( (Index1 & RamMask[CurrentRamConfig]) *0x2000);
		MemPageOffsets[Index1]=1;
	}
	SetRomMap(0);
	SetMapType(0);
	return;
}

void SetVectors(unsigned char data)
{
	RamVectors=!!data; //Bit 3 of $FF90 MC3
	return;
}

void SetMmuRegister(unsigned char Register,unsigned char data)
{	
	unsigned char BankRegister,Task;
	BankRegister = Register & 7;
	Task=!!(Register & 8);
	MmuRegisters[Task][BankRegister]= MmuPrefix |(data & RamMask[CurrentRamConfig]); //gime.c returns what was written so I can get away with this
	return;
}

void SetRomMap(unsigned char data)
{	
	RomMap=(data & 3);
	UpdateMmuArray();
	return;
}

void SetMapType(unsigned char type)
{
	MapType=type;
	UpdateMmuArray();
	return;
}

void Set_MmuTask(unsigned char task)
{
	MmuTask=task;
	MmuState= (!MmuEnabled)<<1 | MmuTask;
	return;
}

void Set_MmuEnabled (unsigned char usingmmu)
{
	MmuEnabled=usingmmu;
	MmuState= (!MmuEnabled)<<1 | MmuTask;
	return;
}
 
unsigned char * Getint_rom_pointer(void)
{
	return(InternalRomBuffer);
}

void CopyRom(void)
{
	char ExecPath[MAX_PATH];
	char COCO3ROMPath[MAX_PATH];
	char IniFilePath[MAX_PATH];

	GetIniFilePath(IniFilePath);
	GetPrivateProfileString("DefaultPaths", "COCO3ROMPath", "", COCO3ROMPath, MAX_PATH, IniFilePath);
	unsigned short temp=0;
	strcat(COCO3ROMPath, "\\coco3.rom");
	if (COCO3ROMPath != "") { temp = load_int_rom(COCO3ROMPath); } //Try loading from the user defined path first.
	if (temp) { OutputDebugString(" Found coco3.rom in COCO3ROMPath\n"); }

	if (temp == 0) { temp = load_int_rom(BasicRomName()); }		//Try to load the image
	if (temp == 0)
	{	// If we can't find it use default copy
		GetModuleFileName(NULL, ExecPath, MAX_PATH);
		PathRemoveFileSpec(ExecPath);
		strcat(ExecPath, "coco3.rom");
		temp = load_int_rom(ExecPath);
	}
	if (temp == 0)
	{
		MessageBox(0, "Missing file coco3.rom", "Error", 0);
		exit(0);
	}
//		for (temp=0;temp<=32767;temp++)
//			InternalRomBuffer[temp]=CC3Rom[temp];
	return;
}

int load_int_rom(TCHAR filename[MAX_PATH])
{
	unsigned short index=0;
	FILE *rom_handle;
	rom_handle=fopen(filename,"rb");
	if (rom_handle==NULL)
		return(0);
	while ((feof(rom_handle)==0) & (index<0x8000))
		InternalRomBuffer[index++]=fgetc(rom_handle);

	fclose(rom_handle);
	return(index);
}

// Coco3 MMU Code
unsigned char MemRead8( unsigned short address)
{
	if (address<0xFE00)
	{
		if (MemPageOffsets[MmuRegisters[MmuState][address>>13]]==1)
			return(MemPages[MmuRegisters[MmuState][address>>13]][address & 0x1FFF]);
		return( PackMem8Read( MemPageOffsets[MmuRegisters[MmuState][address>>13]] + (address & 0x1FFF) ));
	}
	if (address>0xFEFF)
		return (port_read(address));
	if (RamVectors)	//Address must be $FE00 - $FEFF
		return(memory[(0x2000*VectorMask[CurrentRamConfig])|(address & 0x1FFF)]); 
	if (MemPageOffsets[MmuRegisters[MmuState][address>>13]]==1)
		return(MemPages[MmuRegisters[MmuState][address>>13]][address & 0x1FFF]);
	return( PackMem8Read( MemPageOffsets[MmuRegisters[MmuState][address>>13]] + (address & 0x1FFF) ));
}

void MemWrite8(unsigned char data,unsigned short address)
{
//	char Message[256]="";
//	if ((address>=0xC000) & (address<=0xE000))
//	{
//		sprintf(Message,"Writing %i to ROM Address %x\n",data,address);
//		WriteLog(Message,TOCONS);
//	}
	if (address<0xFE00)
	{
		if (MapType | (MmuRegisters[MmuState][address>>13] <VectorMaska[CurrentRamConfig]) | (MmuRegisters[MmuState][address>>13] > VectorMask[CurrentRamConfig]))
			MemPages[MmuRegisters[MmuState][address>>13]][address & 0x1FFF]=data;
		return;
	}
	if (address>0xFEFF)
	{
		port_write(data,address);
		return;
	}
	if (RamVectors)	//Address must be $FE00 - $FEFF
		memory[(0x2000*VectorMask[CurrentRamConfig])|(address & 0x1FFF)]=data;
	else
	if (MapType | (MmuRegisters[MmuState][address>>13] <VectorMaska[CurrentRamConfig]) | (MmuRegisters[MmuState][address>>13] > VectorMask[CurrentRamConfig]))
		MemPages[MmuRegisters[MmuState][address>>13]][address & 0x1FFF]=data;
	return;
}

unsigned char __fastcall fMemRead8( unsigned short address)
{
	if (address<0xFE00)
	{
		if (MemPageOffsets[MmuRegisters[MmuState][address>>13]]==1)
			return(MemPages[MmuRegisters[MmuState][address>>13]][address & 0x1FFF]);
		return( PackMem8Read( MemPageOffsets[MmuRegisters[MmuState][address>>13]] + (address & 0x1FFF) ));
	}
	if (address>0xFEFF)
		return (port_read(address));
	if (RamVectors)	//Address must be $FE00 - $FEFF
		return(memory[(0x2000*VectorMask[CurrentRamConfig])|(address & 0x1FFF)]); 
	if (MemPageOffsets[MmuRegisters[MmuState][address>>13]]==1)
		return(MemPages[MmuRegisters[MmuState][address>>13]][address & 0x1FFF]);
	return( PackMem8Read( MemPageOffsets[MmuRegisters[MmuState][address>>13]] + (address & 0x1FFF) ));
}

void __fastcall fMemWrite8(unsigned char data,unsigned short address)
{
	if (address<0xFE00)
	{
		if (MapType | (MmuRegisters[MmuState][address>>13] <VectorMaska[CurrentRamConfig]) | (MmuRegisters[MmuState][address>>13] > VectorMask[CurrentRamConfig]))
			MemPages[MmuRegisters[MmuState][address>>13]][address & 0x1FFF]=data;
		return;
	}
	if (address>0xFEFF)
	{
		port_write(data,address);
		return;
	}
	if (RamVectors)	//Address must be $FE00 - $FEFF
		memory[(0x2000*VectorMask[CurrentRamConfig])|(address & 0x1FFF)]=data;
	else
	if (MapType | (MmuRegisters[MmuState][address>>13] <VectorMaska[CurrentRamConfig]) | (MmuRegisters[MmuState][address>>13] > VectorMask[CurrentRamConfig]))
		MemPages[MmuRegisters[MmuState][address>>13]][address & 0x1FFF]=data;
	return;
}
/*****************************************************************
* 16 bit memory handling routines                                *
*****************************************************************/

unsigned short MemRead16(unsigned short addr)
{
	return (MemRead8(addr)<<8 | MemRead8(addr+1));
}

void MemWrite16(unsigned short data,unsigned short addr)
{
	MemWrite8( data >>8,addr);
	MemWrite8( data & 0xFF,addr+1);
	return;
}

unsigned short GetMem(long address) {
	return(memory[address]);
}
void SetMem(long address, unsigned short data) {
	memory[address] = data;
}

void SetDistoRamBank(unsigned char data)
{

	switch (CurrentRamConfig)
	{
	case 0:	// 128K
		return;
		break;
	case 1:	//512K
		return;
		break;
	case 2:	//2048K
		SetVideoBank(data & 3);
		SetMmuPrefix(0);
		return;
		break;
	case 3:	//8192K	//No Can 3 
		SetVideoBank(data & 0x0F);
		SetMmuPrefix( (data & 0x30)>>4);
		return;
		break;
	}
	return;
}

void SetMmuPrefix(unsigned char data)
{
	MmuPrefix=(data & 3)<<8;
	return;
}

void UpdateMmuArray(void)
{
	if (MapType)
	{
		MemPages[VectorMask[CurrentRamConfig]-3]=memory+(0x2000*(VectorMask[CurrentRamConfig]-3));
		MemPages[VectorMask[CurrentRamConfig]-2]=memory+(0x2000*(VectorMask[CurrentRamConfig]-2));
		MemPages[VectorMask[CurrentRamConfig]-1]=memory+(0x2000*(VectorMask[CurrentRamConfig]-1));
		MemPages[VectorMask[CurrentRamConfig]]=memory+(0x2000*VectorMask[CurrentRamConfig]);

		MemPageOffsets[VectorMask[CurrentRamConfig]-3]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]-2]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]-1]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]]=1;
		return;
	}
	switch (RomMap)
	{
	case 0:
	case 1:	//16K Internal 16K External
		MemPages[VectorMask[CurrentRamConfig]-3]=InternalRomBuffer;
		MemPages[VectorMask[CurrentRamConfig]-2]=InternalRomBuffer+0x2000;
		MemPages[VectorMask[CurrentRamConfig]-1]=NULL;
		MemPages[VectorMask[CurrentRamConfig]]=NULL;

		MemPageOffsets[VectorMask[CurrentRamConfig]-3]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]-2]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]-1]=0;
		MemPageOffsets[VectorMask[CurrentRamConfig]]=0x2000;
		return;
	break;

	case 2:	// 32K Internal
		MemPages[VectorMask[CurrentRamConfig]-3]=InternalRomBuffer;
		MemPages[VectorMask[CurrentRamConfig]-2]=InternalRomBuffer+0x2000;
		MemPages[VectorMask[CurrentRamConfig]-1]=InternalRomBuffer+0x4000;
		MemPages[VectorMask[CurrentRamConfig]]=InternalRomBuffer+0x6000;

		MemPageOffsets[VectorMask[CurrentRamConfig]-3]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]-2]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]-1]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]]=1;
		return;
	break;

	case 3:	//32K External
		MemPages[VectorMask[CurrentRamConfig]-1]=NULL;
		MemPages[VectorMask[CurrentRamConfig]]=NULL;
		MemPages[VectorMask[CurrentRamConfig]-3]=NULL;
		MemPages[VectorMask[CurrentRamConfig]-2]=NULL;

		MemPageOffsets[VectorMask[CurrentRamConfig]-1]=0;
		MemPageOffsets[VectorMask[CurrentRamConfig]]=0x2000;
		MemPageOffsets[VectorMask[CurrentRamConfig]-3]=0x4000;
		MemPageOffsets[VectorMask[CurrentRamConfig]-2]=0x6000;
		return;
	break;
	}
	return;
}

