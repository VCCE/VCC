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

/********************************************************************/
/*
	Simple object for handling a ROM
 
	TODO: address mask used when reading ROM has potential to be incorrect
 */
/********************************************************************/

#include "rom.h"

#include "xDebug.h"

#include <stdio.h>

/********************************************************************/
/**
 */
result_t romInit(rom_t * pROM)
{
	assert(pROM != NULL);
	
	memset(pROM,0,sizeof(rom_t));
	
	return XERROR_NONE;
}

/********************************************************************/
/**
 */
result_t romLoad(rom_t * pROM, const char * pathname)
{
	result_t	errResult = XERROR_INVALID_PARAMETER;
	
    assert(pROM!= NULL);

    if (    pROM != NULL
         && pathname != NULL
        )
    {
        errResult = fileLoad(pathname, (void **)&pROM->pData, &pROM->szData);
        if ( errResult == XERROR_NONE )
        {
            pROM->BankedCartOffset	= 0;
            pROM->bLoaded			= TRUE;
            pROM->bOurBuffer		= TRUE;
            pROM->bLoaded			= TRUE;
        }
    }
    
	return errResult;
}

/********************************************************************/
/**
 */
result_t romSet(rom_t * pROM, uint8_t * pData, size_t szData)
{
	assert(pROM!= NULL);
	
	romDestroy(pROM);
	
	pROM->pData			= pData;
	pROM->szData		= szData;
	pROM->bOurBuffer	= FALSE;
	pROM->bLoaded		= FALSE;
	
	return XERROR_NONE;
}

/********************************************************************/
/**
 */
result_t romReset(rom_t * pROM)
{
	assert(pROM!= NULL);
	
	pROM->BankedCartOffset = 0;
	
	return XERROR_NONE;
}

/********************************************************************/
/**
 */
result_t romDestroy(rom_t * pROM)
{
	assert(pROM!= NULL);
	
	pROM->bLoaded = FALSE;
	if ( pROM->pData != NULL )
	{
		if ( pROM->bOurBuffer )
		{
			free(pROM->pData);
		}
		
		pROM->pData = NULL;
	}
	
	pROM->szData			= 0;
	pROM->BankedCartOffset	= 0;
	pROM->bLoaded			= FALSE;
	pROM->bOurBuffer		= FALSE;
	
	return XERROR_NONE;
}

/********************************************************************/
