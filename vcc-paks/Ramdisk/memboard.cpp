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
/*
	Disto RamPak emulation

	$FF40 - write only - lsb of 24 bit address
	$FF41 - write only - nsb of 24 bit address
	$FF42 - write only - msb of 24 bit address
	$FF43 - read/write - data register

	Read sector
		for each byte
			write each address byte
			read data byte
	
	Write sector
		for each byte
			write each address byte
			write data byte

*/
/****************************************************************************/

#include "memboard.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>

/****************************************************************************/

unsigned char			g_Block		= 0;	// 0-255
unsigned short			g_Address	= 0;	// 0-65535
static unsigned char *	RamBuffer[256];

char					DStatus[256] = "";
static int				statusUpdate = 0;

/****************************************************************************/
/**
	Initialize the memory Pak - allocate buffers
*/
int InitMemBoard(void)
{
	int x;

	g_Address	= 0;
	g_Block		= 0;

	// allocate 256 blocks of 64k each = 16MB
	for (x = 0; x<256; x++)
	{
		RamBuffer[x] = (unsigned char *)malloc(65536);
		if (RamBuffer[x] == NULL)
		{
			// out of memory
			return 1;
		}
		
		memset(RamBuffer[x], 0, 65536);
	}

	strcpy(DStatus, "");

	// success
	return 0;
}

/****************************************************************************/
/**
	Clean up - release memory
*/
int DestroyMemBoard(void)
{
	int x;

	g_Block		= 0xFF;
	g_Address	= 0xFFFF;
	for (x = 0; x<256; x++)
	{
		if (RamBuffer[x] != NULL)
		{
			free(RamBuffer[x]);
			RamBuffer[x] = NULL;
		}
	}

	return 0;
}

/****************************************************************************/
/**
	Write a byte to one of our i/o ports
*/
int WritePort(unsigned char Port, unsigned char Data)
{
	switch (Port)
	{
	case BASE_PORT + 0:
		g_Address = (g_Address & 0xFF00) | ((unsigned short)Data & 0xFF);
		break;

	case BASE_PORT + 1:
		g_Address = (g_Address & 0x00FF) | (((unsigned short)Data << 8) & 0xFF00);
		break;

	case BASE_PORT + 2:
		g_Block = (Data & 0xFF);
		break;
	}

	return 0;
}

/****************************************************************************/
/**
	Write a byte to the memory array
*/
int WriteArray(unsigned char Data)
{
	if (statusUpdate == 0)
	{
		sprintf(DStatus, "W %0000.4X", (g_Address >> 8) | ((unsigned int)g_Block << 8));
	}
	statusUpdate++;
	if (statusUpdate == 256)
	{
		statusUpdate = 0;
	}

	RamBuffer[g_Block][g_Address] = Data;

	return 0;
}

/****************************************************************************/
/**
	Read a byte from the memory array
*/
unsigned char ReadArray(void)
{
	if (statusUpdate == 0)
	{
		sprintf(DStatus, "R %0000.4X", ((g_Address >> 8)&0x00FF) | (((unsigned int)g_Block << 8)&0xFF00));
	}
	statusUpdate++;
	if (statusUpdate == 256)
	{
		statusUpdate = 0;
	}

	return RamBuffer[g_Block][g_Address];
}

/****************************************************************************/


