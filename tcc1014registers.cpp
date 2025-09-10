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

#include <Windows.h>
#include "defines.h"
#include "tcc1014mmu.h"
#include "tcc1014registers.h"
#include "tcc1014graphics.h"
#include "coco3.h"
#include "keyboard.h"
#include "Vcc.h"


static unsigned char VDG_Mode=0;
static unsigned char Dis_Offset=0;
static unsigned char MPU_Rate=0;
static unsigned char *rom;
static unsigned char GimeRegisters[256];
static unsigned short VerticalOffsetRegister=0;
static unsigned char EnhancedFIRQFlag=0,EnhancedIRQFlag=0;
static int InteruptTimer=0;
void SetInit0(unsigned char);
void SetInit1(unsigned char);
void SetGimeIRQStearing();
void SetGimeFIRQStearing();
void SetTimerMSB();
void SetTimerLSB();
unsigned char GetInit0();
static unsigned char IRQStearing[8]={0,0,0,0,0,0,0,0};
static unsigned char FIRQStearing[8]={0,0,0,0,0,0,0,0};
static unsigned char LastIrq = 0, LastFirq = 0;

static unsigned char KeyboardInteruptEnabled = 0;

unsigned char GimeGetKeyboardInteruptState()
{
	return KeyboardInteruptEnabled;
}

void GimeSetKeyboardInteruptState(unsigned char State)
{
	KeyboardInteruptEnabled = !!State;
}

void GimeWrite(unsigned char port,unsigned char data)
{
	GimeRegisters[port]=data;

	switch (port)
	{
	case 0x90:
		SetInit0(data);
		break;

	case 0x91:
		SetInit1(data);
		break;

	case 0x92:
		SetGimeIRQStearing();
		break;

	case 0x93:
		SetGimeFIRQStearing();
		break;

	case 0x94:
		SetTimerMSB();
		break;

	case 0x95:
		SetTimerLSB();
		break;

	case 0x96:
		SetTurboMode(data & 1);
		break;
	case 0x97:
		break;

	case 0x98:
		SetGimeVmode(data);
		break;

	case 0x99:
		SetGimeVres(data);
		break;

	case 0x9A:
		SetGimeBoarderColor(data);
		break;

	case 0x9B:
		SetDistoRamBank(data);
		break;

	case 0x9C:
		break;

	case 0x9D:
	case 0x9E:
		SetVerticalOffsetRegister ((GimeRegisters[0x9D]<<8) | GimeRegisters[0x9E]);
		break;

	case 0x9F:
		SetGimeHorzOffset(data);
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
		SetMmuRegister(port,data);
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
		SetGimePallet(port-0xB0,data & 63);
		break;
	}
	return;
}

unsigned char GimeRead(unsigned char port)
{
	// iobus sets port range 0x90 to 0xBF
	auto data = 0;
	switch (port)
	{
	case 0x92:
		data=LastIrq;
		LastIrq=0;
		CPUDeAssertInterupt(IS_GIME, INT_IRQ);
		return data;
	case 0x93:
		data=LastFirq;
		LastFirq=0;
		CPUDeAssertInterupt(IS_GIME, INT_FIRQ);
		return data;
	default:
		if (port >= 0xA0) {
			data = GimeRegisters[port];
		    if (port >= 0xB0) data &= 0x3F;
			return data;
	    } else {
			return 0x1B;
		}
	}
}

void SetInit0(unsigned char data)
{
	SetCompatMode ( !!(data & 128));
	Set_MmuEnabled (!!(data & 64)); //MMUEN
	SetRomMap ( data & 3);			//MC0-MC1
	SetVectors( data & 8);			//MC3
	EnhancedFIRQFlag=(data & 16)>>4;
	EnhancedIRQFlag=(data & 32)>>5;
	return;
}

void SetInit1(unsigned char data)
{
	Set_MmuTask(data & 1);			//TR
	SetTimerClockRate (data & 32);	//TINS
	return;
}

unsigned char GetInit0()
{
	unsigned char data=0;
	return data;
}

void SetGimeIRQStearing() //92
{
	// FIXME: GimeIRQStearing for CART (bit 0)

	if ( (GimeRegisters[0x92] & 2) | (GimeRegisters[0x93] & 2) )
		GimeSetKeyboardInteruptState(1);
	else
		GimeSetKeyboardInteruptState(0);

	if ( (GimeRegisters[0x92] & 8) | (GimeRegisters[0x93] & 8) )
		SetVertInteruptState(1); 
	else
		SetVertInteruptState(0);

	if ( (GimeRegisters[0x92] & 16) | (GimeRegisters[0x93] & 16) )
		SetHorzInteruptState(1);
	else
		SetHorzInteruptState(0);

	if ( (GimeRegisters[0x92] & 32) | (GimeRegisters[0x93] & 32) )
		SetTimerInteruptState(1);
	else
		SetTimerInteruptState(0);
	return;
}

void SetGimeFIRQStearing() //93
{
	// FIXME: GimeFIRQStearing for CART (bit 0)

	if ( (GimeRegisters[0x92] & 2) | (GimeRegisters[0x93] & 2) )
		GimeSetKeyboardInteruptState(1);
	else
		GimeSetKeyboardInteruptState(0);

	if ( (GimeRegisters[0x92] & 8) | (GimeRegisters[0x93] & 8) )
		SetVertInteruptState(1);
	else
		SetVertInteruptState(0);

	if ( (GimeRegisters[0x92] & 16) | (GimeRegisters[0x93] & 16) )
		SetHorzInteruptState(1);
	else
		SetHorzInteruptState(0);
	// Moon Patrol Demo Using Timer for FIRQ Side Scroll 
	if ( (GimeRegisters[0x92] & 32) | (GimeRegisters[0x93] & 32) )
		SetTimerInteruptState(1);
	else
		SetTimerInteruptState(0);

	return;
}

void SetTimerMSB() //94
{
	unsigned short Temp;
	Temp=((GimeRegisters[0x94] <<8)+ GimeRegisters[0x95]) & 4095;
	SetInteruptTimer(Temp);
	return;	
}

void SetTimerLSB() //95
{
	unsigned short Temp;
	Temp=((GimeRegisters[0x94] <<8)+ GimeRegisters[0x95]) & 4095;
	SetInteruptTimer(Temp);
	return;
}

void GimeAssertKeyboardInterupt() 
{
	if ((GimeRegisters[0x93] & 2) && EnhancedFIRQFlag == 1)
	{
		CPUAssertInterupt(IS_GIME, INT_FIRQ);
		LastFirq = LastFirq | 2;
	}
	else if ((GimeRegisters[0x92] & 2) && EnhancedIRQFlag == 1)
	{
		CPUAssertInterupt(IS_GIME, INT_IRQ);
		LastIrq = LastIrq | 2;
	}
}

void GimeAssertVertInterupt()
{
	if ((GimeRegisters[0x93] & 8) && EnhancedFIRQFlag == 1)
	{
		CPUAssertInterupt(IS_GIME, INT_FIRQ); //FIRQ
		LastFirq = LastFirq | 8;
	}
	else if ((GimeRegisters[0x92] & 8) && EnhancedIRQFlag == 1)
	{
		CPUAssertInterupt(IS_GIME, INT_IRQ); //IRQ moon patrol demo using this
		LastIrq = LastIrq | 8;
	}
}

void GimeAssertHorzInterupt()
{
	if ((GimeRegisters[0x93] & 16) && EnhancedFIRQFlag == 1)
	{
		CPUAssertInterupt(IS_GIME, INT_FIRQ);
		LastFirq = LastFirq | 16;
	}
	else if ((GimeRegisters[0x92] & 16) && EnhancedIRQFlag == 1)
	{
		CPUAssertInterupt(IS_GIME, INT_IRQ);
		LastIrq = LastIrq | 16;
	}
}

// Timer [F]IRQ bit gets set even if interrupt is not enabled.
// TODO: What about other gime interrupts? Are they simular?
void GimeAssertTimerInterupt()
{
	if (GimeRegisters[0x93] & 32) 
	{
		LastFirq = LastFirq | 32;
		if (EnhancedFIRQFlag == 1) 
			CPUAssertInterupt(IS_GIME, INT_FIRQ);
	}
	else if (GimeRegisters[0x92] & 32) 
	{
		LastIrq = LastIrq | 32;
		if (EnhancedIRQFlag == 1) 
			CPUAssertInterupt(IS_GIME, INT_IRQ);
	}
	return;
}

// CART
void GimeAssertCartInterupt()
{
	if (GimeRegisters[0x93] & 1)
	{
		LastFirq = LastFirq | 1;
		if (EnhancedFIRQFlag == 1)
			CPUAssertInterupt(IS_GIME, INT_FIRQ);
	}
	else if (GimeRegisters[0x92] & 1)
	{
		LastIrq = LastIrq | 1;
		if (EnhancedIRQFlag == 1)
			CPUAssertInterupt(IS_GIME, INT_IRQ);
	}
}

unsigned char sam_read(unsigned char port) //SAM don't talk much :)
{
	
	if ( (port>=0xF0) & (port <=0xFF)) //IRQ vectors from rom
		return( rom[0x7F00 + port]);

	return 0;
}
void sam_write(unsigned char port)
{
	unsigned char mask=0;
	unsigned char reg=0;

	if ((port >=0xC6) & (port <=0xD3))	//VDG Display offset Section
	{
		port=port-0xC6;
		reg= ((port & 0x0E)>>1);
		mask= 1<<reg;
		Dis_Offset= Dis_Offset & (0xFF-mask); //Shut the bit off
		if (port & 1)
			Dis_Offset= Dis_Offset | mask;
		SetGimeVdgOffset(Dis_Offset);
	}

	if ((port >=0xC0) & (port <=0xC5))	//VDG Mode
	{
		port=port-0xC0;
		reg= ((port & 0x0E)>>1);
		mask= 1<<reg;
		VDG_Mode = VDG_Mode & (0xFF-mask);
		if (port & 1)
			VDG_Mode = VDG_Mode | mask;
		SetGimeVdgMode(VDG_Mode);
	}

	if ( (port==0xDE) | (port ==0xDF))
		SetMapType(port&1);

	// RAM high speed poke
	if (port==0xD9)
		SetCPUMultiplyerFlag(1);

	if (port==0xD8)
		SetCPUMultiplyerFlag(0);

//
// todo: ROM high speed poke, currently unsupported by vcc
// should only be fast in rom not ram so can't just use
// SetCPUMultiplyerFlag. 
//
//	if (port==0xD7)
//		SetCPUMultiplyerFlag (1);
//
//	if (port==0xD6)
//		SetCPUMultiplyerFlag (0);


	return;
}


void mc6883_reset()
{
	VDG_Mode=0;
	Dis_Offset=0;
	MPU_Rate=0;
	rom=Getint_rom_pointer();
	return;
}

unsigned char VDG_Offset()
{
	return Dis_Offset;
}

unsigned char VDG_Modes()
{
	return VDG_Mode;
}

