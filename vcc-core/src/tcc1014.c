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

/********************************************************************************/
/*
 *  tcc1014.c
 */
/********************************************************************************/

#include "tcc1014.h"

#include "coco3.h"

#include "xDebug.h"

/********************************************************************************/

#pragma mark -
#pragma mark --- prototypes ---

/********************************************************************************/

void gimeRegReset(gime_t * pGIME);
void mmuSetMapType(gime_t * pGIME, unsigned char type);

/********************************************************************************/

#pragma mark -
#pragma mark --- emulator device callbacks ---

/********************************************************************/

result_t gimeEmuDevDestroy(emudevice_t * pEmuDevice)
{
	gime_t * pGIME;
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	
	pGIME = (gime_t *)pEmuDevice;
	
	ASSERT_GIME(pGIME);

	if ( pGIME != NULL )
	{
		/*
		 graphics clean-up
		 */
		
		/*
		 mmu clean up
		 */
		if ( pGIME->memory != NULL )
		{
			free(pGIME->memory);
			pGIME->memory = NULL;
		}
        
		emuDevRemoveChild(pGIME->mmu.device.pParent,&pGIME->mmu.device);
		
		free(pGIME);
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/

result_t gimeEmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	gime_t *	pGIME;
	
	pGIME = (gime_t *)pEmuDevice;
	
	ASSERT_GIME(pGIME);
	
	if ( pGIME != NULL )
	{
		// do nothing for now
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/

result_t gimeEmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	gime_t *	pGIME;
	
	pGIME = (gime_t *)pEmuDevice;
	
	ASSERT_GIME(pGIME);
	
	if ( pGIME != NULL )
	{
		// do nothing for now
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/

#pragma mark -
#pragma mark --- emulator device callbacks ---

/********************************************************************/

gime_t * gimeCreate()
{
	gime_t *	pGIME;
	
	pGIME = calloc(1,sizeof(gime_t));
	if ( pGIME != NULL )
	{
		pGIME->mmu.device.id		= EMU_DEVICE_ID;
        pGIME->mmu.device.idModule	= EMU_MMU_ID;
		pGIME->mmu.device.idDevice	= VCC_GIME_ID;
        
		strcpy(pGIME->mmu.device.Name,"GIME");
		
		pGIME->mmu.device.pfnDestroy	= gimeEmuDevDestroy;
		pGIME->mmu.device.pfnSave		= gimeEmuDevConfSave;
		pGIME->mmu.device.pfnLoad		= gimeEmuDevConfLoad;
		
		emuDevRegisterDevice(&pGIME->mmu.device);
		
		/*
			graphics
		*/
		pGIME->iVidMask=0x1FFFF;        
		pGIME->ExtendedText=1;
	}
	
	return pGIME;
}

int gimeGetCpuRate(gime_t * pGIME)
{
    ASSERT_GIME(pGIME);
    
    return pGIME->CpuRate;
}

/********************************************************************/
/* 
 */
void gimeReset(gime_t * pGIME)
{
	ASSERT_GIME(pGIME);
	
	pGIME->LowerCase=0;
	pGIME->InvertAll=0;
	pGIME->ExtendedText=1;
	pGIME->HorzOffsetReg=0;
	pGIME->DistoOffset=0;
	pGIME->Hoffset=0;
	pGIME->VerticalOffsetRegister=0;
    pGIME->VerticalScrollRegister = 0;

    pGIME->CC3Vmode=0;
    pGIME->CC3Vres=0;
    pGIME->CC2Offset=0;

    pGIME->ExtendedText = 1;

    pGIME->TimerClockRate           = 0;
    pGIME->MasterTickCounter        = 0;
    pGIME->UnxlatedTickCounter      = 0;
    
    mmuSetMapType(pGIME,0);
	
	gimeRegReset(pGIME);
}

/********************************************************************/
