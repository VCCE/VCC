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
/********************************************************************************/

#include "tcc1014registers.h"

#include "tcc1014mmu.h"
#include "tcc1014graphics.h"
#include "keyboard.h"
#include "cpu.h"
#include "Coco3.h"

#include "xDebug.h"

/********************************************************************************/

void gimeRegReset(gime_t * pGIME)
{
    ASSERT_GIME(pGIME);

	pGIME->HorzInterruptEnabled = false;
	pGIME->VertInterruptEnabled = false;
	pGIME->TimerInterruptEnabled = false;
}

/********************************************************************************/

bool gimeVertInterruptEnabled(gime_t * pGIME)
{
    ASSERT_GIME(pGIME);

	return (pGIME->VertInterruptEnabled  != 0);
}

/********************************************************************************/

bool gimeHorzInterruptEnabled(gime_t * pGIME)
{
    ASSERT_GIME(pGIME);

	return (pGIME->HorzInterruptEnabled != 0);
}

/*****************************************************************/
/*
 */
void gimeSetKeyboardInterruptState(gime_t * pGIME, bool state)
{
    ASSERT_GIME(pGIME);
    
    pGIME->KeyboardInterruptEnabled = !!state;
}


/********************************************************************************/

void gimeSetInit0(gime_t * pGIME, unsigned char data)
{
    ASSERT_GIME(pGIME);

    SetCompatMode(pGIME, ((data & 128) != 0) );
    
	mmuSetEnabled (pGIME, ((data & 64) != 0) );   // MMUEN
    
	mmuSetRomMap (pGIME, data & 3);			// MC0-MC1
	mmuSetVectors(pGIME, data & 8);			// MC3
    
	pGIME->EnhancedFIRQFlag = ((data & 16) != 0);
    pGIME->EnhancedIRQFlag  = ((data & 32) != 0);
}

/********************************************************************************/

void gimeSetInit1(gime_t * pGIME, unsigned char data)
{
    ASSERT_GIME(pGIME);

	mmuSetTask(pGIME, data & 1);			//TR
    
    // 0 = 63.695uS  (1/60*262)  1 scanline time
    // 1 = 279.265nS (1/ColorBurst)
    pGIME->TimerClockRate = ((data & 32) != 0);
}

/********************************************************************************/

void gimeSetHorzInterruptState(gime_t * pGIME, bool state)
{
    ASSERT_GIME(pGIME);

	pGIME->HorzInterruptEnabled = !!state;
}

/********************************************************************************/

void gimeSetVertInterruptState(gime_t * pGIME, bool state)
{
    ASSERT_GIME(pGIME);

	pGIME->VertInterruptEnabled = !!state;
}

/********************************************************************************/

void gimeSetTimerInterruptState(gime_t * pGIME, bool state)
{
    ASSERT_GIME(pGIME);

	pGIME->TimerInterruptEnabled = state;
}

/********************************************************************************/

void gimeSetGimeIRQStearing(gime_t * pGIME, unsigned char data) //92
{
	ASSERT_GIME(pGIME);
	
    bool enabled;
    
    enabled = (pGIME->GimeRegisters[0x92] & 2) | (pGIME->GimeRegisters[0x93] & 2);
    gimeSetKeyboardInterruptState(pGIME,enabled);
    
    enabled = (pGIME->GimeRegisters[0x92] & 8) | (pGIME->GimeRegisters[0x93] & 8);
    gimeSetVertInterruptState(pGIME,enabled);
    
    enabled = (pGIME->GimeRegisters[0x92] & 16) | (pGIME->GimeRegisters[0x93] & 16);
    gimeSetHorzInterruptState(pGIME,enabled);
    
    enabled = (pGIME->GimeRegisters[0x92] & 32) | (pGIME->GimeRegisters[0x93] & 32);
    gimeSetTimerInterruptState(pGIME,enabled);
}

void gimeSetGimeFIRQStearing(gime_t * pGIME, unsigned char data) //93
{
    ASSERT_GIME(pGIME);
	
    bool enabled;
    
    enabled = (pGIME->GimeRegisters[0x92] & 2) | (pGIME->GimeRegisters[0x93] & 2);
    gimeSetKeyboardInterruptState(pGIME,enabled);
    
	
    enabled = (pGIME->GimeRegisters[0x92] & 8) | (pGIME->GimeRegisters[0x93] & 8);
    gimeSetVertInterruptState(pGIME,enabled);
    
    enabled = (pGIME->GimeRegisters[0x92] & 16) | (pGIME->GimeRegisters[0x93] & 16);
    gimeSetHorzInterruptState(pGIME,enabled);
    
    enabled = (pGIME->GimeRegisters[0x92] & 32) | (pGIME->GimeRegisters[0x93] & 32);
    gimeSetTimerInterruptState(pGIME,enabled);
}

/********************************************************************************/

#if 0
The 12-bit timer can be loaded with any number from 0-4095. The timer resets and restarts counting down as soon as a number is written to $FF94. Writing to $FF95 does not restart the timer, but the value does save. Reading from either register does not restart the timer.
62
$FF96-$FF97 - Unused
Color Computer 1/2/3 Hardware Programming, Chris Lomont, v0.82

When the timer reaches zero, it automatically restarts and triggers an interrupt (if enabled). The timer also controls the rate of blinking text. Storing a zero to both registers stops the timer from operating. Lastly, the timer works slightly differently on the 1986 and 1987 versions of the GIME. Neither can actually run a clock count of 1. That is, if you store a 1 into the timer register, the 1986 GIME actually processes this as a '3' and the 1987 GIME processes it as a '2'. All other values stored are affected the same way: nnn+2 for 1986 GIME and nnn+1 for 1987 GIME.
(2) Must turn timer interrupt enable off/on again to reset timer IRQ/FIRQ.
(3) Storing a $00 at $FF94 seems to stop the timer. Also, apparently each time it passes thru
zero, the $FF92/93 bit is set without having to re-enable that Interrupt Request.
#endif

// TODO: only a write to $FF94 (MSB) re-starts the timer
void gimeSetTimerMSB(gime_t * pGIME, unsigned char data) //94
{
	unsigned short Temp;
	
    ASSERT_GIME(pGIME);

	Temp = ((pGIME->GimeRegisters[0x94] <<8)+ pGIME->GimeRegisters[0x95]) & 4095;

    // 1986 GIME (+2)
    pGIME->UnxlatedTickCounter = (Temp & 0xFFF) + 2;

    // reset timer
    pGIME->MasterTickCounter = pGIME->UnxlatedTickCounter;
}

void gimeSetTimerLSB(gime_t * pGIME, unsigned char data) //95
{
	unsigned short Temp;
	
    ASSERT_GIME(pGIME);
	
	Temp = ((pGIME->GimeRegisters[0x94] <<8)+ pGIME->GimeRegisters[0x95]) & 4095;
    pGIME->UnxlatedTickCounter = (Temp & 0xFFF);

    //pGIME->MasterTickCounter = pGIME->UnxlatedTickCounter;
}

void gimeWrite(emudevice_t * pEmuDevice, unsigned char port, unsigned char data)
{
	gime_t *		pGIME = (gime_t *)pEmuDevice;
	
    ASSERT_GIME(pGIME);
	
	pGIME->GimeRegisters[port] = data;

	switch (port)
	{
	case 0x90:
		gimeSetInit0(pGIME,data);
		break;

	case 0x91:
		gimeSetInit1(pGIME,data);
		break;

	case 0x92:
		gimeSetGimeIRQStearing(pGIME,data);
		break;

	case 0x93:
		gimeSetGimeFIRQStearing(pGIME,data);
		break;

	case 0x94:
		gimeSetTimerMSB(pGIME,data);
		break;
	case 0x95:
		gimeSetTimerLSB(pGIME,data);
		break;

	case 0x96:
        emuDevLog(&pGIME->mmu.device, "(unused) GIME register $FF96 write");
		break;
	case 0x97:
        emuDevLog(&pGIME->mmu.device, "(unused) GIME register $FF97 write");
		break;

	case 0x98:
		SetGimeVmode(pGIME,data);
		break;

	case 0x99:
		SetGimeVres(pGIME,data);
		break;

	case 0x9A:
		SetGimeBorderColor(pGIME,data);
		break;

	case 0x9B:
		mmuSetDistoRamBank(pGIME,data);
		break;

	case 0x9C:
        SetVertScrollRegister(pGIME,data);
    break;

	case 0x9D:
	case 0x9E:
        SetVerticalOffsetRegister(pGIME,((uint16_t)pGIME->GimeRegisters[0x9D]<<8) | ((uint16_t)pGIME->GimeRegisters[0x9E]&0xFF));
    break;

	case 0x9F:
		SetGimeHorzOffset(pGIME,data);
		break;

	case 0xA0:
	case 0xA1:
	case 0xA2:
	case 0xA3:
	case 0xA4:
	case 0xA5:
	case 0xA6:
	case 0xA7:
	case 0xA8:
	case 0xA9:
	case 0xAA:
	case 0xAB:
	case 0xAC:
	case 0xAD:
	case 0xAE:
	case 0xAF:
		mmuSetRegister(pGIME,port,data);
		break;

	case 0xB0:
	case 0xB1:
	case 0xB2:
	case 0xB3:
	case 0xB4:
	case 0xB5:
	case 0xB6:
	case 0xB7:
	case 0xB8:
	case 0xB9:
	case 0xBA:
	case 0xBB:
	case 0xBC:
	case 0xBD:
	case 0xBE:
	case 0xBF:
		SetGimePallet(pGIME,port-0xB0,data & 63);
		break;
	}
}

unsigned char gimeRead(emudevice_t * pEmuDevice, unsigned char port)
{
	gime_t *	pGIME = (gime_t *)pEmuDevice;
    int Temp = 0;
    
    ASSERT_GIME(pGIME);

	switch ( port )
	{
        case 0x92:
            Temp=pGIME->LastIrq;
            pGIME->LastIrq=0;
            return (Temp);
        break;

        case 0x93:
            Temp=pGIME->LastFirq;
            pGIME->LastFirq=0;
            return (Temp);
        break;

        case 0x94:
            return (pGIME->MasterTickCounter >> 8) & 0xFF;
        break;
            
        case 0x95:
            return (pGIME->MasterTickCounter >> 0) & 0xFF;
        break;

        default:
            return (pGIME->GimeRegisters[port]);
	}
}

/**
 */
void gimeAssertKeyboardInterrupt(gime_t * pGIME, cpu_t * pCPU) 
{
    ASSERT_GIME(pGIME);
    ASSERT_CPU(pCPU);

    if ( pGIME->KeyboardInterruptEnabled )
    {
        if ( ((pGIME->GimeRegisters[0x93] & irqKeyboard)) && (pGIME->EnhancedFIRQFlag))
        {
            cpuAssertInterrupt(pCPU,FIRQ,0);
            pGIME->LastFirq = pGIME->LastFirq | irqKeyboard;
        }
        else if ( ((pGIME->GimeRegisters[0x92] & irqKeyboard)) && (pGIME->EnhancedIRQFlag) )
        {
            cpuAssertInterrupt(pCPU,IRQ,0);
            pGIME->LastIrq = pGIME->LastIrq | irqKeyboard;
        }
    }
}

/**
 */
void gimeAssertVertInterrupt(gime_t * pGIME, cpu_t * pCPU)
{
    ASSERT_GIME(pGIME);
    ASSERT_CPU(pCPU);

    if ( gimeVertInterruptEnabled(pGIME) )
    {
        if ( ((pGIME->GimeRegisters[0x93] & irqVertical)) && (pGIME->EnhancedFIRQFlag) )
        {
            cpuAssertInterrupt(pCPU,FIRQ,0);
            pGIME->LastFirq = pGIME->LastFirq | irqVertical;
        }
        else if ( ((pGIME->GimeRegisters[0x92] & irqVertical)) && (pGIME->EnhancedIRQFlag) )
        {
            cpuAssertInterrupt(pCPU,IRQ,0);
            pGIME->LastIrq = pGIME->LastIrq | irqVertical;
        }
    }
}

/**
 */
void gimeAssertHorzInterrupt(gime_t * pGIME, cpu_t * pCPU)
{
    ASSERT_GIME(pGIME);
    ASSERT_CPU(pCPU);

    if ( gimeHorzInterruptEnabled(pGIME) )
    {
        if (((pGIME->GimeRegisters[0x93] & irqHorizontal)) && (pGIME->EnhancedFIRQFlag))
        {
            cpuAssertInterrupt(pCPU,FIRQ,0);
            pGIME->LastFirq = pGIME->LastFirq | irqHorizontal;
        }
        else if (((pGIME->GimeRegisters[0x92] & irqHorizontal)) && (pGIME->EnhancedIRQFlag))
        {
            cpuAssertInterrupt(pCPU,IRQ,0);
            pGIME->LastIrq = pGIME->LastIrq | irqHorizontal;
        }
    }
}

/**
 */
void gimeAssertTimerInterrupt(gime_t * pGIME, cpu_t * pCPU)
{
    ASSERT_GIME(pGIME);
	ASSERT_CPU(pCPU);

	if (((pGIME->GimeRegisters[0x93] & irqTimer)) && (pGIME->EnhancedFIRQFlag))
	{
		cpuAssertInterrupt(pCPU,FIRQ,0);
		pGIME->LastFirq = pGIME->LastFirq | irqTimer;
	}
	else if (((pGIME->GimeRegisters[0x92] & irqTimer)) && (pGIME->EnhancedIRQFlag))
    {
        cpuAssertInterrupt(pCPU,IRQ,0);
        pGIME->LastIrq = pGIME->LastIrq | irqTimer;
    }
}

//
// S.A.M.
//

/**
 */
unsigned char samRead(emudevice_t * pEmuDevice, unsigned char port) //SAM don't talk much :)
{
	gime_t *	pGIME = (gime_t *)pEmuDevice;
	
    ASSERT_GIME(pGIME);

	if (    (port>=0xF0) 
		  //& (port <=0xFF) // always true...
	   )
	{
        //IRQ vectors from rom
		return pGIME->rom.pData[0x3F00 + port];
	}
	
	return(0);
}

/**
 */
void samWrite(emudevice_t * pEmuDevice, unsigned char port, unsigned char data)
{
	unsigned char mask=0;
	unsigned char reg=0;
	gime_t *	pGIME = (gime_t *)pEmuDevice;
	coco3_t *		pCoco3;
	
    ASSERT_GIME(pGIME);
	pCoco3 = (coco3_t *)pGIME->mmu.device.pParent;
	ASSERT_CC3(pCoco3);
	
    //VDG Display offset Section
	if ((port >= 0xC6) & (port <= 0xD3))
	{
		port=port-0xC6;
		reg= ((port & 0x0E)>>1);
		mask= 1<<reg;
		pGIME->Dis_Offset= pGIME->Dis_Offset & (0xFF-mask); //Shut the bit off
		if (port & 1)
			pGIME->Dis_Offset = pGIME->Dis_Offset | mask;
		SetGimeVdgOffset(pGIME,pGIME->Dis_Offset);
	}

    //VDG Mode
	if ((port >= 0xC0) & (port <= 0xC5))
	{
		port=port-0xC0;
		reg= ((port & 0x0E)>>1);
		mask= 1<<reg;
		pGIME->VDG_Mode = pGIME->VDG_Mode & (0xFF-mask);
		if (port & 1)
			pGIME->VDG_Mode = pGIME->VDG_Mode | mask;
		SetGimeVdgMode(pGIME,pGIME->VDG_Mode);
	}

    // 0xFFDE(65502) / 0xFFDF(65503)
	if ( (port == 0xDE) | (port == 0xDF))
    {
		mmuSetMapType(pGIME,port&1);
    }
    
    // CPU double speed
    // 0xFFD7(65495) / 0xFFD9(65497)
	if ( (port == 0xD7) | (port == 0xD9) )
	{
        pGIME->CpuRate = 1;
	}

    // CPU Normal speed
    // 0xFFD6(65496) / 0xFFD8(65498)
	if ( (port == 0xD6) | (port == 0xD8) )
	{
        pGIME->CpuRate = 0;
	}
}

/**
 */
void mc6883_Reset(gime_t * pGIME)
{
    ASSERT_GIME(pGIME);

	pGIME->VDG_Mode = 0;
	pGIME->Dis_Offset = 0;
	pGIME->CpuRate = 0;
}

unsigned char mc6883_VDG_Offset(gime_t * pGIME)
{
    ASSERT_GIME(pGIME);

	return (pGIME->Dis_Offset);
}

unsigned char mc6883_VDG_Modes(gime_t * pGIME)
{
    ASSERT_GIME(pGIME);

	return (pGIME->VDG_Mode);
}

