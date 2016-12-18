#pragma once

/****************************************************************************/
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

// define this to make the main config dialog modal
// vs. modeless where you can interact with the main window
// while the config dialog is open
//#define CONFIG_DIALOG_MODAL

/****************************************************************************/

#include "defines.h"

/****************************************************************************/

#define TH_RUNNING	0
#define TH_REQWAIT	1
#define TH_WAITING	2

/****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

	extern SystemState EmuState;

    uint8_t	SetRamSize(const uint8_t size);

	void			SetCPUMultiplyerFlag (unsigned char);
	void			SetTurboMode(unsigned char);
	unsigned char	SetCPUMultiplyer(unsigned char );
	
	unsigned char	SetSpeedThrottle(unsigned char);
	unsigned char	SetFrameSkip(unsigned char);
	unsigned char	SetCpuType( unsigned char);
	unsigned char	SetAutoStart(unsigned char);

	void			DoReboot(void);
	void			DoHardReset(SystemState *);
	void			LoadPack (void);

	void			vccSetResetPending(int resetType);

#ifdef __cplusplus
}
#endif

/****************************************************************************/
