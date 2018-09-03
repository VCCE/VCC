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

#ifndef _tcc1014registers_h_
#define _tcc1014registers_h_

/*****************************************************************************/

#include "cpu.h"
#include "tcc1014.h"
#include "emuDevice.h"

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	void			gimeRegReset(gime_t * pGIME);

	bool			gimeVertInterruptEnabled(gime_t * pGIME);
	bool			gimeHorzInterruptEnabled(gime_t * pGIME);
	
	void			gimeWrite(emudevice_t * pEmuDevice, unsigned char,unsigned char);
	unsigned char	gimeRead(emudevice_t * pEmuDevice, unsigned char);
	
	void			gimeAssertKeyboardInterrupt(gime_t * pGIME, cpu_t * pCPU);
	void			gimeAssertHorzInterrupt(gime_t * pGIME, cpu_t * pCPU);
	void			gimeAssertVertInterrupt(gime_t * pGIME, cpu_t * pCPU);
	void			gimeAssertTimerInterrupt(gime_t * pGIME, cpu_t * pCPU);
	
    // TODO: move this out?  refactor?
	void			mc6883_Reset(gime_t * pGIME);
	unsigned char	mc6883_VDG_Offset(gime_t * pGIME);
	unsigned char	mc6883_VDG_Modes(gime_t * pGIME);

	unsigned char	samRead(emudevice_t * pEmuDevice, unsigned char);
	void			samWrite(emudevice_t * pEmuDevice, unsigned char port, unsigned char data);
	
#ifdef __cplusplus
}
#endif
		
/*****************************************************************************/

#endif
