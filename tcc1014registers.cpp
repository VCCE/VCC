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
static int InteruptTimer=0;
void SetInit0(unsigned char);
void SetInit1(unsigned char);
void SetTimerMSB();
void SetTimerLSB();
unsigned char GetInit0();

//
// Gime interrupt bits
//
constexpr auto GIME_INTR_TIMER = 1u << 5;
constexpr auto GIME_INTR_HSYNC = 1u << 4;
constexpr auto GIME_INTR_VSYNC = 1u << 3;
constexpr auto GIME_INTR_RS232 = 1u << 2;
constexpr auto GIME_INTR_KEYB = 1u << 1;
constexpr auto GIME_INTR_CART = 1u << 0;

//
// Gime interrupt status
//
static auto GimeFirqState = 0u;
static auto GimeIrqState = 0u;
static auto LastGimeFirq = 0u;
static auto LastGimeIrq = 0u;

//
// Gime irq/firq enabled to cpu?
//
bool GimeIrqToCpuEnabled() { return GimeRegisters[0x90] & 0x20; }
bool GimeFirqToCpuEnabled() { return GimeRegisters[0x90] & 0x10; }

//
// Set interrupt GIME_INTR* flag
//
void GimeSetInterrupt(unsigned int flag)
{
	// merge to internal state when enabled
	GimeIrqState |= (flag & GimeRegisters[0x92]) & 0x3F;
	GimeFirqState |= (flag & GimeRegisters[0x93]) & 0x3F;

	// update state for enabled interrupt to cpu
	auto prevIrq = LastGimeIrq;
	auto prevFirq = LastGimeFirq;
	LastGimeIrq = GimeIrqToCpuEnabled() ? GimeIrqState : 0;
	LastGimeFirq = GimeFirqToCpuEnabled() ? GimeFirqState : 0;

	// on state changed update irq line to cpu
	if (prevIrq != LastGimeIrq)
	{
		if (LastGimeIrq)
			CPUAssertInterupt(IS_GIME, INT_IRQ);
		else
			CPUDeAssertInterupt(IS_GIME, INT_IRQ);
	}

	// on state changed update firq line to cpu
	if (prevFirq != LastGimeFirq)
	{
		if (LastGimeFirq)
			CPUAssertInterupt(IS_GIME, INT_FIRQ);
		else
			CPUDeAssertInterupt(IS_GIME, INT_FIRQ);
	}
}

//
// Clear gime irq GIME_INTR* flag, returns previous state.
//
unsigned char GimeClearIrq(unsigned int nflag)
{
	auto prevIrqState = GimeIrqState;
	auto prevIrq = LastGimeIrq;
	GimeIrqState &= nflag;
	LastGimeIrq &= nflag;

	// if was previously set, clear line
	if (prevIrq && !LastGimeIrq)
		CPUDeAssertInterupt(IS_GIME, INT_IRQ);

	return (unsigned char)prevIrqState;
}

//
// Clear gime firq GIME_INTR* flag, returns previous state.
//
unsigned char GimeClearFirq(unsigned int nflag)
{
	auto prevFirqState = GimeFirqState;
	auto prevFirq = LastGimeFirq;
	GimeFirqState &= nflag;
	LastGimeFirq &= nflag;

	// if was previously set, clear line
	if (prevFirq && !LastGimeFirq)
		CPUDeAssertInterupt(IS_GIME, INT_FIRQ);

	return (unsigned char)prevFirqState;
}

//
// Reset gime registers to defaults
//
void GimeRegistersReset()
{
	memset(GimeRegisters, 0, sizeof(GimeRegisters));

	GimeFirqState = 0u;
	GimeIrqState = 0u;
	LastGimeFirq = 0u;
	LastGimeIrq = 0u;

	VDG_Mode = 0;
	Dis_Offset = 0;
	MPU_Rate = 0;
	VerticalOffsetRegister = 0;
	InteruptTimer = 0;
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
		GimeClearIrq(~data); // TODO: Verify this
		break;

	case 0x93:
		GimeClearFirq(~data); // TODO: Verify this
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
		gGimeGpu.SetGimeVmode(data);
		break;

	case 0x99:
		gGimeGpu.SetGimeVres(data);
		break;

	case 0x9A:
		gGimeGpu.SetGimeBoarderColor(data);
		break;

	case 0x9B:
		SetDistoRamBank(data);
		break;

	case 0x9C:
		break;

	case 0x9D:
	case 0x9E:
		gGimeGpu.SetVerticalOffsetRegister((GimeRegisters[0x9D]<<8) | GimeRegisters[0x9E]);
		break;

	case 0x9F:
		gGimeGpu.SetGimeHorzOffset(data);
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
		gGimeGpu.SetGimePalette(port-0xB0,data & 63);
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
			// note, clear all irq flags & interrupt line should be cleared if was
			// set otherwise extra interrupts will occur. this is noticable in robocop
			// the sound will repeat/stutter.
			// note, return internal flags to program, see rtaylor's timer dsk.
			return GimeClearIrq(0);
		case 0x93:
			// note, as above.
			return GimeClearFirq(0);
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
	gGimeGpu.SetCompatMode(!!(data & 128));
	Set_MmuEnabled (!!(data & 64)); //MMUEN
	SetRomMap ( data & 3);			//MC0-MC1
	SetVectors( data & 8);			//MC3
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

void SetTimerMSB() //94
{
	unsigned short Temp;
	Temp=((GimeRegisters[0x94] <<8)+ GimeRegisters[0x95]) & 4095;
	SetInteruptTimer(Temp);
}

void SetTimerLSB() //95
{
	// does not restart timer
}

void GimeAssertKeyboardInterupt() 
{
	GimeSetInterrupt(GIME_INTR_KEYB);
}

void GimeAssertVertInterupt()
{
	GimeSetInterrupt(GIME_INTR_VSYNC);
}

void GimeAssertHorzInterupt()
{
	GimeSetInterrupt(GIME_INTR_HSYNC);
}

void GimeAssertTimerInterupt()
{
	GimeSetInterrupt(GIME_INTR_TIMER);
}

void GimeAssertCartInterupt()
{
	GimeSetInterrupt(GIME_INTR_CART);
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
		gGimeGpu.SetGimeVdgOffset(Dis_Offset);
	}

	if ((port >=0xC0) & (port <=0xC5))	//VDG Mode
	{
		port=port-0xC0;
		reg= ((port & 0x0E)>>1);
		mask= 1<<reg;
		VDG_Mode = VDG_Mode & (0xFF-mask);
		if (port & 1)
			VDG_Mode = VDG_Mode | mask;
		gGimeGpu.SetGimeVdgMode(VDG_Mode);
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

