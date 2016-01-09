#ifndef __PAKINTERFACE_H__
#define __PAKINTERFACE_H__

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

#include "defines.h"
#include "vccPakAPI.h"

/****************************************************************************/

#ifndef __cplusplus
extern "C"
{
#endif

	void vccPakTimer(void);
	unsigned char vccPakPortRead(unsigned char);
	void vccPakPortWrite(unsigned char, unsigned char);
	unsigned char vccPakMem8Read(unsigned short);
	void vccPakMem8Write(unsigned char, unsigned char);

	void GetModuleStatus(SystemState *);
	int LoadCart(void);
	//void ConfigThisModule (void);
	unsigned short PackAudioSample(void);
	void ResetBus(void);

	int load_ext_rom(char *);
	void GetCurrentModule(char *);
	int InsertModule(char *);
	void UpdateBusPointer(void);
	void UnloadDll(void);
	void UnloadPack(void);
	void DynamicMenuActivated(unsigned char);
	void RefreshDynamicMenu(void);

#ifndef __cplusplus
}
#endif

/****************************************************************************/

//
// Defines the start and end IDs for the dynamic menus
//
#define ID_SDYNAMENU 5000	
#define ID_EDYNAMENU 5100

/****************************************************************************/

#endif // __PAKINTERFACE_H__

