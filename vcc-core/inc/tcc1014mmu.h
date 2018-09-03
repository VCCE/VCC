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

#ifndef _tcc1014mmu_h_
#define _tcc1014mmu_h_

/*****************************************************************************/

#include "tcc1014.h"
#include "file.h"

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
    // should be emuDevice callbacks
	void					mmuInit(gime_t * pGIME, ramconfig_e RamConfig, const char * pPathname);
	void					mmuReset(gime_t * pGIME);

	unsigned char *			mmuGetRAMBuffer(gime_t * pGIME);
	
	void					mmuSetMapType(gime_t * pGIME, unsigned char);
	void					mmuSetTask(gime_t * pGIME, unsigned char);
	void					mmuSetRegister(gime_t * pGIME, unsigned char,unsigned char);
	void					mmuSetEnabled(gime_t * pGIME, unsigned char );
	void					mmuSetRomMap(gime_t * pGIME, unsigned char );
	void					mmuSetVectors(gime_t * pGIME, unsigned char);
	void					mmuSetDistoRamBank(gime_t * pGIME, unsigned char);
	void					mmuSetPrefix(gime_t * pGIME, unsigned char);

	// TODO: move to CPU?
	void					portInit(gime_t * pGIME);
	void					portRegisterRead(gime_t * pGIME, portreadfn_t pfnPortRead, emudevice_t * pEmuDevice, int iStart, int iEnd);
	void					portRegisterWrite(gime_t * pGIME, portwritefn_t pfnPortWrite, emudevice_t * pEmuDevice, int iStart, int iEnd);
	void					portRegisterDefaultRead(gime_t * pGIME, portreadfn_t pfnPortRead, emudevice_t * pEmuDevice);
	void					portRegisterDefaultWrite(gime_t * pGIME, portwritefn_t pfnPortWrite, emudevice_t * pEmuDevice);
	unsigned char			portRead(gime_t * pGIME, int addr);
	void					portWrite(gime_t * pGIME, unsigned char data, int addr);
	
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif

