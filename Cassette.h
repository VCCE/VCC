#ifndef __CASSETTE_H__
#define __CASSETTE_H__
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

constexpr auto STOP		= 0u;
constexpr auto PLAY		= 1u;
constexpr auto REC		= 2u;
constexpr auto EJECT	= 3u;
constexpr auto CAS		= 1u;
constexpr auto WAV		= 0u;

constexpr auto CAS_WRITEBUFFERSIZE = 0x40000u;
constexpr auto CAS_TAPEREADAHEAD = 1000u; // decoded batch size
constexpr auto CAS_SILENCE = 128u;
constexpr auto CAS_TAPEAUDIORATE = 44100u;

unsigned int GetTapeCounter(void);
unsigned int LoadTape(void);
void SetTapeCounter(unsigned int, bool force = false);
void SetTapeMode(unsigned char);
void Motor(unsigned char);
void LoadCassetteBuffer(unsigned char *, unsigned int* CassBufferSize);
void FlushCassetteBuffer(const unsigned char *,unsigned int *);
void GetTapeName(char *);
void UpdateTapeStatus(char* status, int max);
uint8_t CassInBitStream();

extern unsigned char TapeFastLoad;
unsigned int GetTapeRate();
unsigned char GetMotorState();

#endif
