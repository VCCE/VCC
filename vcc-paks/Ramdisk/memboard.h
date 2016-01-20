#ifndef __RAMDISK_H_
#define __RAMDISK_H_

/****************************************************************************/
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
/****************************************************************************/

#define BASE_PORT 0x40

/****************************************************************************/

extern unsigned char	g_Block;
extern unsigned short	g_Address;

extern char	DStatus[256];

/****************************************************************************/

int InitMemBoard(void);
int DestroyMemBoard(void);

int WritePort(unsigned char,unsigned char);
int WriteArray(unsigned char);
unsigned char ReadArray(void);

/****************************************************************************/

#endif // __RAMDISK_H_
