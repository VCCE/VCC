// gimex.cpp -- see gimex.h.
//
// This file is part of VCC (Virtual Color Computer). VCC is free software:
// you can redistribute it and/or modify it under the terms of the GNU General
// Public License as published by the Free Software Foundation, either version 3
// of the License, or (at your option) any later version.

#include <Windows.h>
#include "defines.h"
#include "gimex.h"
#include "tcc1014mmu.h"
#include "tcc1014graphics.h"
#include "Vcc.h"

namespace
{
	// firmware version reported at $FFEF: major in bits 7-6, minor in bits 5-0 (1.03)
	constexpr unsigned char kVersionMajor = 1;
	constexpr unsigned char kVersionMinor = 3;
	constexpr unsigned char kVersion = (kVersionMajor << 6) | kVersionMinor;

	// GIME-X addresses up to 2 MB of physical RAM
	constexpr unsigned int kAddressMask = 0x1FFFFF;

	constexpr unsigned char kCtrlWriteInc = 0x01;
	constexpr unsigned char kCtrlReadInc = 0x02;

	bool sEnabled = false;
	unsigned int sDest = 0;            // $FFE1-$FFE3
	unsigned int sSource = 0;          // $FFE4-$FFE6
	unsigned char sControl = 0;        // $FFE7
	unsigned char sPalette256Index = 0; // $FFE0, last 256-colour entry written

	// physical RAM size, mirrored like the MMU does for smaller configs
	unsigned int PhysicalMask()
	{
		static const unsigned int sizes[4] = { 0x20000, 0x80000, 0x200000, 0x800000 };
		unsigned int size = sizes[EmuState.RamSize & 3];
		return (size - 1) & kAddressMask;
	}

	void SetHigh(unsigned int& addr, unsigned char data) { addr = (addr & 0x00FFFF) | ((data & 0x1F) << 16); }
	void SetMid(unsigned int& addr, unsigned char data)  { addr = (addr & 0x1F00FF) | (data << 8); }
	void SetLow(unsigned int& addr, unsigned char data)  { addr = (addr & 0x1FFF00) | data; }

	// Each access to $FFE8 or $FFE9 transfers one byte at the current pointer and
	// then post-increments it if enabled. A 16 bit STD/LDD therefore moves two
	// consecutive bytes (high at addr, low at addr+1) when auto-increment is on.
	// TODO: verify non-incrementing 16 bit behaviour against real GIME-X hardware.
	void DataWrite(unsigned char data)
	{
		SetMem(sDest & PhysicalMask(), data);
		if (sControl & kCtrlWriteInc)
			sDest = (sDest + 1) & kAddressMask;
	}

	unsigned char DataRead()
	{
		auto data = static_cast<unsigned char>(GetMem(sSource & PhysicalMask()));
		if (sControl & kCtrlReadInc)
			sSource = (sSource + 1) & kAddressMask;
		return data;
	}
}

void SetGimeXEnabled(bool enabled)
{
	sEnabled = enabled;
}

bool GimeXEnabled()
{
	return sEnabled;
}

void GimeXReset()
{
	sDest = 0;
	sSource = 0;
	sControl = 0;
	sPalette256Index = 0;
	SetGimeXTurbo(false);
}

bool GimeXOwnsPort(unsigned char port)
{
	if (!sEnabled) return false;
	return (port >= 0xE0 && port <= 0xE9) || port == 0xEF;
}

void GimeXWrite(unsigned char port, unsigned char data)
{
	switch (port)
	{
	case 0xE0:
		sPalette256Index = data;
		gGimeGpu.SetPalette256FromPalette0(data);
		break;
	case 0xE1: SetHigh(sDest, data); break;
	case 0xE2: SetMid(sDest, data); break;
	case 0xE3: SetLow(sDest, data); break;
	case 0xE4: SetHigh(sSource, data); break;
	case 0xE5: SetMid(sSource, data); break;
	case 0xE6: SetLow(sSource, data); break;
	case 0xE7: sControl = data & (kCtrlWriteInc | kCtrlReadInc); break;
	case 0xE8:
	case 0xE9: DataWrite(data); break;
	default: break; // $FFEF version is read only
	}
}

unsigned char GimeXSafeRead(unsigned char port)
{
	switch (port)
	{
	case 0xE0: return sPalette256Index;
	case 0xE1: return (sDest >> 16) & 0x1F;
	case 0xE2: return (sDest >> 8) & 0xFF;
	case 0xE3: return sDest & 0xFF;
	case 0xE4: return (sSource >> 16) & 0x1F;
	case 0xE5: return (sSource >> 8) & 0xFF;
	case 0xE6: return sSource & 0xFF;
	case 0xE7: return sControl;
	case 0xE8:
	case 0xE9: return static_cast<unsigned char>(GetMem(sSource & PhysicalMask()));
	case 0xEF: return kVersion;
	default: return 0xFF;
	}
}

unsigned char GimeXRead(unsigned char port)
{
	if (port == 0xE8 || port == 0xE9)
		return DataRead();
	return GimeXSafeRead(port);
}

void SetGimeXTurbo(bool turbo)
{
	turbo = turbo && sEnabled;
	if (EmuState.GimeXTurboFlag == turbo) return;
	EmuState.GimeXTurboFlag = turbo;
	SetupClock();
}
