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

#include "Windows.h"
#include "defines.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "tcc1014mmu.h"
#include "iobus.h"
#include "config.h"
#include "tcc1014graphics.h"
#include "pakinterface.h"
#include "vcc/utils/logger.h"
#include "hd6309.h"
#include "vcc/utils/FileOps.h"


static unsigned char *MemPages[1024];
static unsigned short MemPageOffsets[1024];
static unsigned char *memory=nullptr;	//Emulated RAM
static unsigned char *InternalRomBuffer=nullptr;
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
static unsigned int RamSize=0;
std::atomic_bool mem_initializing;

void UpdateMmuArray();

/*****************************************************************************************
* MmuInit Initilize and allocate memory for RAM Internal and External ROM Images.        *
* Copy Rom Images to buffer space and reset GIME MMU registers to 0                      *
* Returns nullptr if any of the above fail.                                                 *
*****************************************************************************************/
unsigned char * MmuInit(unsigned char RamConfig)
{
	// Simple flag to reduce possibility of conflict with SafeMemRead8()
	// which debugger uses to access CPU ram. Without it the Memory Display
	// window will occasionally crash VCC when MmuInit runs due to F9 toggle
	// or MemSize config change. Issue does not seem to justify a section lock
	mem_initializing = true;

	unsigned int Index1=0;
	RamSize=MemConfig[RamConfig];
	CurrentRamConfig=RamConfig;
	if (memory != nullptr)
		free(memory);

	memory=(unsigned char *)malloc(RamSize);
	if (memory==nullptr) {
		mem_initializing = false;
		RamSize = 0;
		return nullptr;
	}

	for (Index1=0;Index1<RamSize;Index1++)
	{
		if (Index1 & 1)
			memory[Index1]=0;
		else 
			memory[Index1]=0xFF;
	}
	SetVidMask(VidMask[CurrentRamConfig]);
	if (InternalRomBuffer != nullptr)
		free(InternalRomBuffer);
	InternalRomBuffer=nullptr;
	InternalRomBuffer=(unsigned char *)malloc(0x8000);

	if (InternalRomBuffer == nullptr) {
		mem_initializing = false;
		return nullptr;
	}

	memset(InternalRomBuffer,0xFF,0x8000);
	LoadRom();
	MmuReset();
	mem_initializing = false;
	return memory;
}

void MmuReset()
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

VCC::MMUState GetMMUState()
{
	VCC::MMUState state;

	state.ActiveTask = MmuTask;
	state.Enabled = MmuEnabled != 0;

	for (auto i = 0U; i < state.Task0.size(); ++i)
	{
		state.Task0[i] = MmuRegisters[0][i];
		state.Task1[i] = MmuRegisters[1][i];
	}

	state.RamVectors = RamVectors;
	state.RomMap = RomMap;

	return state;
}

void GetMMUPage(size_t page, std::array<unsigned char, 8192>& outBuffer)
{
	auto offset = page * 8192;
	std::copy(memory + offset, memory + offset + outBuffer.size(), outBuffer.begin());
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
 
unsigned char * Getint_rom_pointer()
{
	return InternalRomBuffer;
}

// LoadRom() loads Coco3.rom. It is called by MmuInit() here
// and by SoftReset() in Vcc.c. If LoadRom() fails VCC can not run.
void LoadRom()
{
	char RomPath[MAX_PATH]={};
	unsigned short index=0;
	FILE *hFile;

	GetExtRomPath(RomPath);
	if (*RomPath == '\0') {
		GetModuleFileName(nullptr, RomPath, MAX_PATH);
		PathRemoveFileSpec(RomPath);
		strncat(RomPath, "coco3.rom", MAX_PATH);
	}

	if ((hFile = fopen(RomPath,"rb")) != nullptr) {
		while ((feof(hFile)==0) & (index<0x8000)) {
			InternalRomBuffer[index++] = fgetc(hFile);
		}
		fclose(hFile);
	}
	if ((hFile == nullptr) | (index == 0)) {
		MessageBox(nullptr,
				"coco3.rom load failed\n"
				"Close this then\n"
				"check ROM path.\n",
				"Error", MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND);
	}
	return;
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

// Debugger does not want to do port reads that change state
unsigned char SafeMemRead8(unsigned short address)
{
	// Do nothing if memory is in initializing state
	if (mem_initializing) return 0;
	// Filter port reads that are not GIME or SAM
	if ((address > 0xFEFF) && (address < 0xFF90)) return memory[address];
	// Otherwise use normal MMU MemRead8
	return MemRead8(address);
}

void MemWrite8(unsigned char data,unsigned short address)
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
	{
		memory[(0x2000 * VectorMask[CurrentRamConfig]) | (address & 0x1FFF)] = data;
	}
	else if (MapType | (MmuRegisters[MmuState][address >> 13] < VectorMaska[CurrentRamConfig]) | (MmuRegisters[MmuState][address >> 13] > VectorMask[CurrentRamConfig]))
	{
		MemPages[MmuRegisters[MmuState][address >> 13]][address & 0x1FFF] = data;
	}

	return;
}

// Returns TRUE if address is writable RAM (Not Cart, Not a Port)
bool MemCheckWrite(unsigned short address)
{
	if (address<0xFE00)
	{
		if (MapType | (MmuRegisters[MmuState][address>>13] <VectorMaska[CurrentRamConfig]) | (MmuRegisters[MmuState][address>>13] > VectorMask[CurrentRamConfig]))
			return true;
	}
	return false;
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
	{
		memory[(0x2000 * VectorMask[CurrentRamConfig]) | (address & 0x1FFF)] = data;
	}
	else
	{
		if (MapType | (MmuRegisters[MmuState][address >> 13] < VectorMaska[CurrentRamConfig]) | (MmuRegisters[MmuState][address >> 13] > VectorMask[CurrentRamConfig]))
		{
			MemPages[MmuRegisters[MmuState][address >> 13]][address & 0x1FFF] = data;
		}
	}
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

unsigned short GetMem(unsigned long address) {
	if (mem_initializing) return 0;  // To prevent access exceptions
	if (address < RamSize)
		return(memory[address]);
	else
		return 0xFF;
}
void SetMem(unsigned long address, unsigned short data) {
	if (address < RamSize)
		memory[address] = (unsigned char) data;
}

unsigned char * Get_mem_pointer()
{
	return memory;
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

void UpdateMmuArray()
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
		MemPages[VectorMask[CurrentRamConfig]-1]=nullptr;
		MemPages[VectorMask[CurrentRamConfig]]=nullptr;

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
		MemPages[VectorMask[CurrentRamConfig]-1]=nullptr;
		MemPages[VectorMask[CurrentRamConfig]]=nullptr;
		MemPages[VectorMask[CurrentRamConfig]-3]=nullptr;
		MemPages[VectorMask[CurrentRamConfig]-2]=nullptr;

		MemPageOffsets[VectorMask[CurrentRamConfig]-1]=0;
		MemPageOffsets[VectorMask[CurrentRamConfig]]=0x2000;
		MemPageOffsets[VectorMask[CurrentRamConfig]-3]=0x4000;
		MemPageOffsets[VectorMask[CurrentRamConfig]-2]=0x6000;
		return;
	break;
	}
	return;
}

