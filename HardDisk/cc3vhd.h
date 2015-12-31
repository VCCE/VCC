#ifndef __CC3VHD_H__
#define __CC3VHD_H__
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
void UnmountHD(void);
int MountHD(char [256]);
unsigned char IdeRead(unsigned char);
void IdeWrite (unsigned char, unsigned char);
void DiskStatus(char *);
#define	HEAD 0
#define SLAVE 1
#define STANDALONE 2


#define DRIVESIZE 130	//in Mb
#define MAX_SECTOR DRIVESIZE*1024*1024
#define SECTORSIZE 256

#define HD_OK		0
#define HD_PWRUP	-1
#define HD_INVLD	-2
#define HD_NODSK	4
#define HD_WP		5

#define SECTOR_READ		0
#define SECTOR_WRITE	1
#define DISK_FLUSH		2

#endif
