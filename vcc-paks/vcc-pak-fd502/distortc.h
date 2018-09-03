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

#ifndef _distortc_h_
#define _distortc_h_

/*****************************************************************************/

#include "pakinterface.h"

/*****************************************************************************/

#define VCC_COCOPAK_DISTORTC_ID	XFOURCC('d','r','t','c')

#define ASSERT_DISTORTC(pDistoRTC)	assert(pDistoRTC != NULL && "NULL pointer"); \
									assert(pDistoRTC->pak.device.id == EMU_DEVICE_ID && "Invalid Emu device ID"); \
									assert(pDistoRTC->pak.device.idModule == VCC_COCOPAK_ID && "Invalid module ID"); \
									assert(pDistoRTC->pak.device.idDevice == VCC_COCOPAK_DISTORTC_ID && "Invalid device ID")

// TODO: move to private header
typedef struct distortc_t distortc_t;
struct distortc_t
{
    // TODO: does this need to be a pak?
	cocopak_t		pak;				// base

    int             baseAddress;        // 16 bit address - eg. $FF50
	unsigned char	time_register;
};

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
#if (defined _LIB)
	XAPI_EXPORT cocopak_t * vccpakModuleCreate();
#else
    
	cocopak_t *				distoRTCCreate(void);
	void					distoRTCReset(cocopak_t * pPak);
	//void					distoRTCDestroy(cocopak_t * pPak);

	void					distoRTCPortWrite(emudevice_t * pEmuDevice, unsigned char Port, unsigned char Data);
	unsigned char			distoRTCPortRead(emudevice_t * pEmuDevice, unsigned char Port);
#endif			

#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif
