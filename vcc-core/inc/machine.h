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

#ifndef _machine_h_
#define _machine_h_

/****************************************************************************/

#include "cpu.h"

/****************************************************************************/

#define EMU_MACHINE_ID		XFOURCC('e','m','c','h')

/****************************************************************************/

//typedef struct machine_t machine_t;
struct machine_t
{
    emudevice_t     device;

    cpu_t *			pCPU;					/**< CPU emulation object */
};

/****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif



#ifdef __cplusplus
}
#endif

/****************************************************************************/

#endif // _machine_h_

