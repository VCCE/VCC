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

#ifndef _cassette_h_
#define _cassette_h_

/*****************************************************************************/

#include "emuDevice.h"

/*****************************************************************************/

typedef enum tapemode_e
{
	kTapeMode_Stop		= 0,
	kTapeMode_Play		= 1,
	kTapeMode_Record	= 2,
	kTapeMode_Eject		= 3
} tapemode_e;

typedef enum tapetype_e
{
	kTapeType_CAS		= 1,
	kTapeType_WAV		= 0
} tapetype_e;

#define WRITEBUFFERSIZE	0x1FFFF

/*****************************************************************************/

#define EMU_DEVICE_NAME_CASSETTE	"Cassette"

#define CONF_SECTION_CASSETTE		"Cassette"

#define CONF_SETTING_TAPEPATH		"TapePath"

/*****************************************************************************/

#define VCC_CASSETTE_ID	XFOURCC('m','c','a','s')

/*****************************************************************************/

typedef struct cassette_t cassette_t;
struct cassette_t
{
	emudevice_t		device;

	/*
		config / persistence
	 */
	char *	        pPathname;

	/*
		run time
	 */
	unsigned char	MotorState;
	unsigned char	TapeMode;
	unsigned char	WriteProtect;
	
	size_t	        TapeOffset;
	size_t	        TotalSize;
	unsigned char *	CasBuffer;
	unsigned char	_FileType;
	
	unsigned long	BytesMoved;
	
	unsigned char	Quiet;
	
	// Write Stuff
	int				LastTrans;
	unsigned char	Mask;
	unsigned char	cByte;
	unsigned char	LastSample;

	unsigned int	TempIndex;
	unsigned char	TempBuffer[8192];
};

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
    cassette_t *	casCreate(void);
	
	// emulator functions
#if 0
	void			casSetTapeCounter(cassette_t * pCassette, uint32_t);
#endif
	void			casMotor(cassette_t * pCassette, unsigned char);
	void			casLoadCassetteBuffer(cassette_t * pCassette, unsigned char *);
	void			casFlushCassetteBuffer(cassette_t * pCassette, unsigned char *, uint32_t);

	// UI functions
	int				casMountTape(cassette_t * pCassette, const char * pPathname);
	void			casSetTapeMode(cassette_t * pCassette, unsigned char);
	size_t          casGetTapeCounter(cassette_t * pCassette);
	
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif
