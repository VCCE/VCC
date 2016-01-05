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

#define STOP	0
#define PLAY	1
#define REC		2
#define EJECT	3
#define CAS	1
#define WAV 0
#define WRITEBUFFERSIZE	0x1FFFF

unsigned int GetTapeCounter(void);
unsigned int LoadTape(void);
void SetTapeCounter(unsigned int);
void SetTapeMode(unsigned char);
void Motor(unsigned char);
void LoadCassetteBuffer(unsigned char *);
void FlushCassetteBuffer(unsigned char *,unsigned int);
void GetTapeName(char *);

#endif
