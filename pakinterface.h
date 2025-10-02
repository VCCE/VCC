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

#pragma once

void PakTimer();
unsigned char PackPortRead (unsigned char);
void PackPortWrite(unsigned char,unsigned char);
unsigned char PackMem8Read (unsigned short);
void PackMem8Write(unsigned short,unsigned char);
void GetModuleStatus( SystemState *);
int LoadCart();
unsigned short PackAudioSample();
void ResetBus();
int load_ext_rom(const char *);
void GetCurrentModule(char *);
int InsertModule (const char *);
void UpdateBusPointer();
void UnloadDll();
void UnloadPack();
void BeginCartMenu();
void CartMenuActivated(unsigned int);

// Error return codes from InsertModule
// FIXME: These should be turned into an enum
constexpr auto NOMODULE = 1;
constexpr auto NOTVCC = 2;

