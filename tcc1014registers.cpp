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
// Returns the current timer counter 
//
unsigned short GimeTimerCounter()
{
	return ((GimeRegisters[0x94] << 8) + GimeRegisters[0x95]) & 0xFFF;
}

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
unsigned char GimeClearIrq()
{
	auto prevIrqState = GimeIrqState;
	auto prevIrq = LastGimeIrq;
	GimeIrqState = 0;
	LastGimeIrq = 0;

	// if was previously set, clear line
	if (prevIrq && !LastGimeIrq)
		CPUDeAssertInterupt(IS_GIME, INT_IRQ);

	return (unsigned char)prevIrqState;
}

//
// Clear gime firq GIME_INTR* flag, returns previous state.
//
unsigned char GimeClearFirq()
{
	auto prevFirqState = GimeFirqState;
	auto prevFirq = LastGimeFirq;
	GimeFirqState = 0;
	LastGimeFirq = 0;

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
}

void GimeWrite(unsigned char port,unsigned char data)
{
#if USE_NEWTIMING
	auto log = [](const char* fmt, auto ...args)
	{
		if (!EmuState.Debugger.IsTracingEnabled()) return;
		char tmp[100];
		sprintf(tmp, fmt, args...);
		EmuState.Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenReg, 0, 0, tmp);
	};
	auto changed = GimeRegisters[port] ^ data;
	auto ifZeroTimerInterrupt = [changed]()
	{
		// interrupt on timer being enabled (Arkanoid sound)
		if ((changed & GIME_INTR_TIMER) && GimeTimerCounter() == 0)
			GimeAssertTimerInterupt();
	};
	auto bit = [data](int n) { return (data >> n) & 1; };
	auto bits = [data](int n, int m) { return (data >> n) & ((1 << (m - n + 1)) - 1); };
#else
	auto log = [](auto...) {};
	auto changed = GimeRegisters[port] ^ data;
	auto bit = [](int) { return 0; };
	auto bits = [](int,int) { return 0; };

	// when timer is 0 and its enabled, it interrupts almost immediately
	auto ifZeroTimerInterrupt = [changed,data]()
	{
		if ((data & changed & GIME_INTR_TIMER) && GimeTimerCounter() == 0)
			GimeAssertTimerInterupt();
	};
#endif

	GimeRegisters[port] = data;

	switch (port)
	{
	case 0x90:
		log("INIT0: %02X CC1=%d MMU=%d IEN=%d FEN=%d", data, bit(7), bit(6), bit(5), bit(4));
		SetInit0(data);
		GimeSetInterrupt(GimeIrqState | GimeFirqState);
		break;

	case 0x91:
		log("INIT1: %02X TINS=%d Task=%d", data, bit(5), bit(0));
		SetInit1(data);
		break;

	case 0x92:
		log("IRQ: %02X", data);
		if (!data) GimeClearIrq(); // 3.3,3.8
		ifZeroTimerInterrupt();
		break;

	case 0x93:
		log("FIRQ: %02X", data);
		if (!data) GimeClearFirq(); // as above
		ifZeroTimerInterrupt();
		break;

	case 0x94:
		log("TMRH: %d", ((GimeRegisters[0x94] & 15) << 8) | GimeRegisters[0x95]);
		SetTimerMSB();
		break;

	case 0x95:
		log("TMRL: %d", ((GimeRegisters[0x94] & 15) << 8) | GimeRegisters[0x95]);
		SetTimerLSB();
		break;

	case 0x96:
		log("TURBO: %02X", data);
		SetTurboMode(data & 1);
		break;

	case 0x97:
		log("$97: %02X", data);
		break;

	case 0x98:
		log("VMODE: %02X BP=%d LPR=%d", data, bit(7), bits(0, 2));
		gGimeGpu.SetGimeVmode(data);
		break;

	case 0x99:
		log("VRES: LPF=%d HR=%d CR=%d", bits(5, 6), bits(2, 4), bits(0, 1));
		gGimeGpu.SetGimeVres(data);
		break;

	case 0x9A:
		log("BDRC: %d", data);
		gGimeGpu.SetGimeBoarderColor(data);
		break;

	case 0x9B:
		log("BANK: %02X", data);
		SetDistoRamBank(data);
		break;

	case 0x9C:
		log("VSC: %d", bits(0, 3));
		gGimeGpu.SetVerticalScroll(GimeRegisters[0x9C]);
		break;

	case 0x9D:
	case 0x9E:
		log("VOFF: %05X", ((GimeRegisters[0x9D] << 8) | GimeRegisters[0x9E]) * 8);
		gGimeGpu.SetVerticalOffsetRegister((GimeRegisters[0x9D] << 8) | GimeRegisters[0x9E]);
		break;

	case 0x9F:
		log("HOFF: %d HVEN=%d", bits(0, 6) * 2, bit(7));
		gGimeGpu.SetGimeHorzOffset(data);
		break;

	case 0xA0:case 0xA1:case 0xA2:case 0xA3:case 0xA4:case 0xA5:case 0xA6:case 0xA7:
	case 0xA8:case 0xA9:case 0xAA:case 0xAB:case 0xAC:case 0xAD:case 0xAE:case 0xAF:
		log("MMU: %X=%02X (%02X...)", port, data, data*2);
		SetMmuRegister(port, data);
		break;

	case 0xB0:case 0xB1:case 0xB2:case 0xB3:case 0xB4:case 0xB5:case 0xB6:case 0xB7:
	case 0xB8:case 0xB9:case 0xBA:case 0xBB:case 0xBC:case 0xBD:case 0xBE:case 0xBF:
		log("PAL: %d=%d", port - 0xB0, bits(0, 5));
		gGimeGpu.SetGimePalette(port - 0xB0, data & 63);
		break;
	}
	return;
}

//
// return gime registers (normally unreadable) for the debugger/memory window
// and without changing any state.
//
unsigned char SafeGimeRead(unsigned char port)
{
	// TODO: return last irq state
	if (port == 0x92) return GimeRegisters[port];
	if (port == 0x93) return GimeRegisters[port];
	return GimeRegisters[port];
}

unsigned char GimeRead(unsigned char port)
{
	auto lastBus = []()
	{
		VCC::CPUState MC6809GetState();
		auto cpuState = MC6809GetState();
		return cpuState.PC < 0xFF00 ? MemRead8(cpuState.PC) : 0;
	};

	// iobus sets port range 0x90 to 0xBF
	auto data = 0;
	switch (port)
	{
		case 0x92:
			// note, clear all irq flags & interrupt line should be cleared if was
			// set otherwise extra interrupts will occur. this is noticable in robocop
			// the sound will repeat/stutter.
			// note, return internal flags to program, see rtaylor's timer dsk.
			return GimeClearIrq();
		case 0x93:
			// note, as above.
			return GimeClearFirq();
		default:
			data = GimeRegisters[port];
			// mmu registers, 
			if (port >= 0xA0 && port <= 0xAF)
			{
				// - if 128k or 512k upper bits are not driven by gime
				if (EmuState.RamSize <= 1)
					return (lastBus() & 0xC0) | (data & 0x3F);

				// - else if 2048/8192k read all bits
				return data;
			}

			// palette registers, read lower bits
			if (port >= 0xB0 && port <= 0xBF)
				return (lastBus() & 0xC0) | (data & 0x3F);

			// other gime registers are unreadable
			return lastBus();
	}
}

void SetInit0(unsigned char data)
{
	gGimeGpu.SetCompatMode(!!(data & 128));
	Set_MmuEnabled (!!(data & 64)); //MMUEN
	SetRomMap(data & 3);			//MC0-MC1
	SetVectors(data & 8);			//MC3
}

void SetInit1(unsigned char data)
{
	Set_MmuTask(data & 1);			//TR
	SetTimerClockRate(data & 32);	//TINS
}

unsigned char GetInit0()
{
	unsigned char data=0;
	return data;
}

void SetTimerMSB() //94
{
	RestartInterruptTimer(GimeTimerCounter());
}

void SetTimerLSB() //95
{
	// does not restart timer but just update the cached time
	SetMasterTickCounter(GimeTimerCounter());
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

// notes:
// 3.3 - zero reg resets flags
// 3.8 - clear partial flags has no effect
//

