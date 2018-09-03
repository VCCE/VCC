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

#ifndef _peripherals_h_
#define _peripherals_h_

/*****************************************************************************/

#include "emuDevice.h"

/*****************************************************************************/

#define PERIPHERAL_COMMAND_NONE			0
#define PERIPHERAL_COMMAND_ADD			1

/*****************************************************************************/

#define EMU_DEVICE_NAME_PERIPHERALMGR	"Peripheral Mgr"

#define CONF_SECTION_PERIPHERALMGR		"Peripherals"

//#define CONF_SETTING_CPU				"CPU"

/*****************************************************************************/

#define VCC_PERIPHERALDEVICE_ID				XFOURCC('m','p','r','d')

#define ASSERT_PERIPHERALDEVICE(pDevice)	assert(pDevice != NULL);	\
											assert(pDevice->device.id == EMU_DEVICE_ID); \
											assert(pDevice->device.idModule == VCC_PERIPHERALDEVICE_ID)

/**
 */
typedef struct peripheral_t peripheral_t;
/**
 */
struct peripheral_t
{
	emudevice_t		device;
	
};

/*****************************************************************************/


#define VCC_PERIPHERALMGR_ID				XFOURCC('m','p','r','m')

#define ASSERT_PERIPHERALMGR(pDevice)	assert(pDevice != NULL);	\
										assert(pDevice->device.id == EMU_DEVICE_ID); \
										assert(pDevice->device.idModule == VCC_PERIPHERALMGR_ID)

/**
 */
typedef struct peripheralmanager_t peripheralmanager_t;
/**
 Emulator device for handling plug-in 'Add-on' or 'Mod' devices
 */
struct peripheralmanager_t
{
	emudevice_t		device;
	
};

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
    peripheralmanager_t *	prmInit(void);
	
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif
