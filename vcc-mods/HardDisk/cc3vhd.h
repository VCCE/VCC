#ifndef _CC3VHD_H_
#define _CC3VHD_H_

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
/***************************************************************************/

/***************************************************************************/

#define EMUHDD_PORT 0x80

// TODO: what about cluster sizes > 1
#define DRIVESIZE	130	//in Mb
#define MAX_SECTOR	DRIVESIZE*1024*1024
#define SECTORSIZE	256

#define HD_OK		0
#define HD_PWRUP	-1
#define HD_INVLD	-2
#define HD_NODSK	4
#define HD_WP		5

#define SECTOR_READ		0
#define SECTOR_WRITE	1
#define DISK_FLUSH		2

/***************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

	int				MountHD(char *, int iDiskNum);
	void			UnmountHD(int iDiskNum);
	void			DiskStatus(char *);

	unsigned char	IdeRead(unsigned char Port);
	void			IdeWrite(unsigned char Data, unsigned char Port);

#ifdef __cplusplus
}
#endif

/***************************************************************************/

#endif // _CC3VHD_H_
