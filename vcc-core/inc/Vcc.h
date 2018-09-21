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

#ifndef _vcc_h_
#define _vcc_h_

/**************************************************/

#include "coco3.h"
#include "throttle.h"
#include "addons.h"
#include "peripherals.h"
#include "joystick.h"
#include "keyboard.h"

#include "file.h"
#include "config.h"

#include "xDebug.h"
#include "xSystem.h"

/**************************************************/
/**
	Emulator thread states
*/
typedef enum emuthreadstate_e
{
	emuThreadOff	= 0,
	emuThreadRunning,
	emuThreadPause,
	emuThreadStop,
	emuThreadQuit
} emuthreadstate_e;

/**************************************************/
/*
	Configuration save/load section names
*/
#define VCC_CONF_SECTION			"VCC"

#define VCC_CONF_SETTING_PAK		"Pak"

#define CARTRIDGE_MENU				"Cartridge"

/**************************************************/

typedef struct vccsettings_t
{
    char *      pModulePath;      /**< Pathname to CART - should be in Coco3 */
} vccsettings_t;

/**************************************************/
/*
	Emulator commands for performing actions/commands
 */

typedef enum vcc_commands_e
{
    VCC_COMMAND_NONE = 0,
    VCC_COMMAND_RUN,
    VCC_COMMAND_STOP,
    VCC_COMMAND_POWERCYCLE,
    VCC_COMMAND_RESET,
    VCC_COMMAND_SETCART,
    VCC_COMMAND_POWER,
    VCC_COMMAND_POWEROFF,
    VCC_COMMAND_POWERON,
    
    VCC_COMMAND_LOADCART,       // move to CoCo3?
    VCC_COMMAND_EJECTCART
} vcc_commands_e;

/**************************************************/

#define VCC_CORE_ID	XFOURCC('v','c','c','x')

#define ASSERT_VCC(pInstance)	assert(pInstance != NULL);	\
								assert(pInstance->root.device.id == EMU_DEVICE_ID); \
								assert(pInstance->root.device.idModule == VCC_CORE_ID)

/**************************************************/
/**
	VCC instance object definition
 */
typedef struct vccinstance_t vccinstance_t;

typedef void (*vcccallback_t)(vccinstance_t * pInstance);

struct vccinstance_t
{
	emurootdevice_t			root;

    filelist_t *            cpuFileList;
    filelist_t *            pakFileList;
    filelist_t *            romFileList;

	vcccallback_t			pfnUIUpdate;				// Update UI callback
    vcccallback_t           pfnScreenShot;              // Save screen shot

	/*
        configuration / persistence
	 */
	config_t *			    config;

    vccsettings_t           run;
    vccsettings_t           conf;
    
	/* 
		pointer to the main coco3 object
		also in links in emudevice_t (device)
	*/
    // TODO: should just be machine_t
	coco3_t *				pCoco3;
    // should be part of the machine?
	addonmanager_t *		pAddOnMgr;
	peripheralmanager_t *	pPeripheralMgr;
	
	/*
		emulator status / state
	 */
    handle_t			    hEmuThread;					// emulator thread
	handle_t				hEmuMutex;			        // emu thread mutex
	emuthreadstate_e		EmuThreadState;				// emulation thread state
	vcc_commands_e 			CommandPending;				// emu reset pending flag

	throttle_t				FrameThrottle;				// frame throttle object
	
	char					cStatusText[256];			// device status text
	xtime_t					tmStatus;					// device status timer
    xtime_t                 tmFrame;                    // device status timer
} ;

/**************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	XAPI vccinstance_t *	vccCreate(const char * pcszName);

	// user interface
	XAPI bool			vccCommandValidate(vccinstance_t * pInstance, int32_t iCommand, int32_t * piState);
	XAPI result_t		vccCommand(vccinstance_t * pInstance, int32_t iCommand);
	XAPI result_t		vccUpdateStatus(vccinstance_t * pInstance);
	XAPI void			vccUpdateUI(vccinstance_t * pInstance);
    XAPI void           vccScreenShot(emurootdevice_t * rootDevice);

    XAPI void           vccSetCommandPending(vccinstance_t * pInstance, vcc_commands_e iCommand);

	// emulator control
	XAPI void			vccEmuLock(vccinstance_t * pInstance);
	XAPI void			vccEmuUnlock(vccinstance_t * pInstance);
	XAPI result_t		vccLoadPak(vccinstance_t * pInstance, const char * pPathname);
	XAPI result_t		vccEmuDevCommand(emudevice_t * pEmuDev, int iCommand, int iParam);
	
	// config
	XAPI result_t		vccLoad(vccinstance_t * pInstance, const char * pcPathname);
	XAPI result_t		vccSave(vccinstance_t * pInstance, const char * pathname, const char * refPath);
	XAPI result_t		vccConfigApply(vccinstance_t * pInstance);

#ifdef __cplusplus
}
#endif

/**************************************************/

#endif
