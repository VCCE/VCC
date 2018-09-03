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

#ifndef _rom_h_
#define _rom_h_

/*****************************************************************************/

#include "file.h"

/********************************************************************/

typedef struct rom_t rom_t;
struct rom_t
{
	bool		bLoaded;
	uint8_t *	pData;
	size_t		szData;
	bool		bOurBuffer;
    
	size_t		BankedCartOffset;
} ;

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	result_t		romInit(rom_t * pROM);
	result_t		romLoad(rom_t * pROM, const char * pathname);
	result_t		romSet(rom_t * pROM, uint8_t * data, size_t size);
	result_t		romReset(rom_t * pROM);
	result_t		romDestroy(rom_t * pROM);
	
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif // _rom_h_

