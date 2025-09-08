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

unsigned short MemRead16(short unsigned int);
unsigned char MemRead8(short unsigned int);
unsigned char SafeMemRead8(short unsigned int);
unsigned char * MmuInit(unsigned char);
unsigned char *	Getint_rom_pointer(void);
unsigned char * Getext_rom_pointer(void);
unsigned short GetMem(unsigned long);
void SetMem(unsigned long, unsigned short);
bool MemCheckWrite(unsigned short address);

void __fastcall fMemWrite8(unsigned char,unsigned short );
unsigned char __fastcall fMemRead8(short unsigned int);

void SetMapType(unsigned char);
void LoadRom(void);
void Set_MmuTask(unsigned char);
void SetMmuRegister(unsigned char,unsigned char);
void Set_MmuEnabled (unsigned char );
void SetRomMap(unsigned char );
void SetVectors(unsigned char);
void MmuReset(void);
void SetDistoRamBank(unsigned char);
void SetMmuPrefix(unsigned char);
void SetCartMMU (unsigned char);
unsigned char * Get_mem_pointer(void);

constexpr auto _128K	= 0u;
constexpr auto _512K	= 1u;
constexpr auto _2M		= 2u;

#endif
