
//----------------------------------------------------------------------
// This file is part of VCC (Virtual Color Computer).
// Vcc is Copyright 2015 by Joseph Forgione
//
// VCC (Virtual Color Computer) is free software, you can redistribute it
// and/or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//----------------------------------------------------------------------

#define _DEBUG_
#ifdef _DEBUG_
#define _DLOG(...) PrintLogC(__VA_ARGS__)
#else
#define _DLOG(...)
#endif

bool LoadRom(char *);
void SDCWrite(unsigned char data,unsigned char port);
unsigned char SDCRead(unsigned char port);
void SDCInit(void);
void SDCReset(void);
void SetSDRoot(const char *);

void AssertInterupt(unsigned char,unsigned char);
void MemWrite(unsigned char,unsigned short);
unsigned char MemRead(unsigned short);
