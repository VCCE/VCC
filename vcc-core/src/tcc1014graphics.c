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
 */
/****************************************************************************/

#include "Vcc.h"

#include "tcc1014graphics.h"
#include "tcc1014registers.h"
#include "tcc1014mmu.h"
#include "coco3.h"

#include <math.h>

/****************************************************************************/
/**
    Send notification that display values have changed
 */
void _displayUpdate(gime_t * pGIME)
{
    deviceevent_t deviceEvent;
    deviceEvent.event.type = eEventDevice;
    deviceEvent.source = &pGIME->mmu.device;
    deviceEvent.target = VCC_COCO3_ID;
    deviceEvent.param = COCO3_DEVEVENT_SCREEN_REFRESH;
    deviceEvent.param2 = 0;
    emuDevSendEventUp(&pGIME->mmu.device, &deviceEvent.event);
}

/****************************************************************************/

//void SetBlinkState(gime_t * pGIME, int Tmp)
//{
//    assert(pGIME != NULL);
//    
//    pGIME->BlinkState = Tmp;
//}

// COCO 2 modes

void SetGimeVdgOffset(gime_t * pGIME, unsigned char Offset)
{
	assert(pGIME != NULL);
	
	if ( pGIME->CC2Offset != Offset)
	{
		pGIME->CC2Offset=Offset;
    
        _displayUpdate(pGIME);
	}
}

void SetGimeVdgMode(gime_t * pGIME, unsigned char VdgMode) // 3 bits from SAM Registers
{
	assert(pGIME != NULL);
	
	if (pGIME->CC2VDGMode != VdgMode)
	{
		pGIME->CC2VDGMode=VdgMode;
        
		_displayUpdate(pGIME);
        
        pGIME->preLatchRow = 0;
	}
}

void SetGimeVdgMode2(gime_t * pGIME, unsigned char Vdgmode2) // 5 bits from PIA Register
{
	assert(pGIME != NULL);
	
	if (pGIME->CC2VDGPiaMode != Vdgmode2)
	{
		pGIME->CC2VDGPiaMode=Vdgmode2;
        
		_displayUpdate(pGIME);
	}
}

// COCO 3 modes

void SetVerticalOffsetRegister(gime_t * pGIME, unsigned short Register)
{
	assert(pGIME != NULL);
	
	if (pGIME->VerticalOffsetRegister != Register)
	{
		pGIME->VerticalOffsetRegister = Register;

		_displayUpdate(pGIME);
	}
}

// TODO: this is saved but is not implemented
void SetVertScrollRegister(gime_t * pGIME, unsigned char data)
{
    assert(pGIME != NULL);
    
    if (pGIME->VerticalScrollRegister != data)
    {
        pGIME->VerticalScrollRegister = data;
        
        _displayUpdate(pGIME);
    }
}

void SetCompatMode(gime_t * pGIME, unsigned char Register)
{
	assert(pGIME != NULL);
	
	if (pGIME->CompatMode != Register)
	{
		pGIME->CompatMode=Register;
        
		_displayUpdate(pGIME);
	}
}

void SetGimeVmode(gime_t * pGIME, unsigned char vmode)
{
	assert(pGIME != NULL);
	
	if (pGIME->CC3Vmode != vmode)
	{
		pGIME->CC3Vmode=vmode;
        
		_displayUpdate(pGIME);
	}
}

void SetGimeVres(gime_t * pGIME, unsigned char vres)
{
	assert(pGIME != NULL);
	
	if (pGIME->CC3Vres != vres)
	{
		pGIME->CC3Vres=vres;
        
		_displayUpdate(pGIME);
	}
}

void SetGimeHorzOffset(gime_t * pGIME, unsigned char data)
{
	assert(pGIME != NULL);
	
	if (pGIME->HorzOffsetReg != data)
	{
		pGIME->Hoffset=(data<<1);
		pGIME->HorzOffsetReg=data;
        
		_displayUpdate(pGIME);
	}
}

void SetGimeBorderColor(gime_t * pGIME, unsigned char data)
{
	assert(pGIME != NULL);

	if (pGIME->CC3BorderColor != (data & 63) )
	{
		pGIME->CC3BorderColor= data & 63;
        
		_displayUpdate(pGIME);
	}
}

void SetVideoBank(gime_t * pGIME, unsigned char data)
{
	assert(pGIME != NULL);
	
	pGIME->DistoOffset= data * (512*1024);
    
	_displayUpdate(pGIME);
}

void SetGimePallet(gime_t * pGIME, unsigned char pallete, unsigned char color)
{
    assert(pGIME != NULL);
    
    pGIME->Pallete[pallete] = (color & 63);
    
    _displayUpdate(pGIME);
}

/*********************************************************************************************************************/

