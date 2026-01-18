//======================================================================
// This file is part of VCC (Virtual Color Computer).
// Vcc is Copyright 2015 by Joseph Forgione
//
// VCC (Virtual Color Computer) is free software, you can redistribute
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//======================================================================

// Vcc.h callable functions exposed to the rest of vcc core

#pragma once

void SetCPUMultiplyerFlag (unsigned char);     //tcc1014registers.cpp
void SetTurboMode(unsigned char);              //tcc1014registers.cpp
unsigned char SetCPUMultiplyer(unsigned char); //tcc1014registers.cpp, config.cpp
unsigned char SetRamSize(unsigned char);       //config.cpp
unsigned char SetSpeedThrottle(unsigned char); //config.cpp, keyboard.cpp
unsigned char SetFrameSkip(unsigned char);     //config.cpp
unsigned char SetCpuType( unsigned char);      //config.cpp
unsigned char SetAutoStart(unsigned char);     //config.cpp
void DoReboot();                               //config.cpp

