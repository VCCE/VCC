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

#ifndef _addons_h_
#define _addons_h_

/*****************************************************************************/

#include "emuDevice.h"

/*****************************************************************************/

#define ADDONMGR_COMMAND_NONE			0
#define ADDONMGR_COMMAND_ADD			1

/*****************************************************************************/

#define EMU_DEVICE_NAME_ADDONMGR	"Add-on Mgr"

#define CONF_SECTION_ADDONMGR		"ADDONS"

//#define CONF_SETTING_CPU			"CPU"

/*****************************************************************************/

#define VCC_ADDONDEVICE_ID			XFOURCC('m','a','o','d')

#define ASSERT_ADDONDEVICE(pDevice)	assert(pDevice != NULL);	\
									assert(pDevice->device.id == EMU_DEVICE_ID); \
									assert(pDevice->device.idModule == VCC_ADDONDEVICE_ID)

/**
 */
typedef struct addon_t addon_t;
/**
 */
struct addon_t
{
	emudevice_t		device;
	
};

/*****************************************************************************/


#define VCC_ADDONMGR_ID				XFOURCC('m','a','o','m')

#define ASSERT_ADDONMGR(pDevice)	assert(pDevice != NULL);	\
									assert(pDevice->device.id == EMU_DEVICE_ID); \
									assert(pDevice->device.idModule == VCC_ADDONMGR_ID)

/**
 */
typedef struct addonmanager_t addonmanager_t;
/**
	Emulator device for handling plug-in 'Add-on' or 'Mod' devices
 */
struct addonmanager_t
{
	emudevice_t		device;

};

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
    addonmanager_t *	aomInit(void);
	
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif
