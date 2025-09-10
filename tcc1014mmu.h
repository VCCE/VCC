#ifndef __TCC1014MMU_H__
#define __TCC1014MMU_H__
#include <array>


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
#include "Debugger.h"

namespace VCC
{

	struct MMUState
	{
		bool Enabled;
		int ActiveTask;
		int RamVectors;
		int RomMap;

		std::array<int, 8>    Task0;
		std::array<int, 8>    Task1;
	};

}

VCC::MMUState GetMMUState();
void GetMMUPage(size_t page, std::array<unsigned char, 8192>& outBuffer);

void MemWrite8(unsigned char,unsigned short );
void MemWrite16(unsigned short,unsigned short );

unsigned short MemRead16(unsigned short);
unsigned char MemRead8(unsigned short);
unsigned char SafeMemRead8(unsigned short);
unsigned char * MmuInit(unsigned char);
unsigned char *	Getint_rom_pointer();
unsigned short GetMem(unsigned long);
void SetMem(unsigned long, unsigned short);
bool MemCheckWrite(unsigned short address);

void __fastcall fMemWrite8(unsigned char,unsigned short );
unsigned char __fastcall fMemRead8(unsigned short);

void SetMapType(unsigned char);
void LoadRom();
void Set_MmuTask(unsigned char);
void SetMmuRegister(unsigned char,unsigned char);
void Set_MmuEnabled (unsigned char );
void SetRomMap(unsigned char );
void SetVectors(unsigned char);
void MmuReset();
void SetDistoRamBank(unsigned char);
void SetMmuPrefix(unsigned char);
unsigned char * Get_mem_pointer();

// FIXME: These need to be turned into an enum and the signature of functions
// that use them updated.
#define _128K	0	
#define _512K	1
#define _2M		2

#endif
