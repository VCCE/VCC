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

#ifndef _fd502_h_
#define _fd502_h_

/*******************************************************************************/

// direct access to drive support
//#define RAW_DRIVE_ACCESS_SUPPORTED

// keyboard LED use support
//#define KEYBOARD_LEDS_SUPPORTED

/*****************************************************************************/

#include "wd1793.h"
#include "distortc.h"

#include "pakinterface.h"

/*****************************************************************************/
/*
    Configuration save/load section names
 */
#define FD502_CONF_SECTION              "FD502"

#define FD502_CONF_SETTING_ROMPATH      "RomPath"
#define FD502_CONF_SETTING_LASTPATH     "LastPath"
#define FD502_CONF_SETTING_TURBODISK    "TurboDisk"

#define FD502_CONF_DISTO_RTC_ENABLED    "DistoRTC"
#define FD502_CONF_DISTO_RTC_ADDRESS    "DistoRTCAddress"

// SS vs DS?

#if defined RAW_DRIVE_ACCESS_SUPPORTED
//
#endif

#if defined KEYBOARD_LEDS_SUPPORTED
#   define FD502_CONF_SETTING_LEDS         "KeyboardLeds"
#endif

/*****************************************************************************/

#define EXTROMSIZE 16384

/*****************************************************************************/

#define FD502_COMMAND_NONE			0
#define FD502_COMMAND_CONFIG		1
#define FD502_COMMAND_SET_ROM       2
#define FD502_COMMAND_TURBO_DISK    3

#define FD502_COMMAND_DISTO_ENABLE  4
#define FD502_COMMAND_DISTO_ADDRESS 5   // (Range: 5 + 1)

#define FD502_COMMAND_DISK0_LOAD	10
#define FD502_COMMAND_DISK1_LOAD	11
#define FD502_COMMAND_DISK2_LOAD	12
#define FD502_COMMAND_DISK3_LOAD	13

#define FD502_COMMAND_DISK0_EJECT	20
#define FD502_COMMAND_DISK1_EJECT	21
#define FD502_COMMAND_DISK2_EJECT	22
#define FD502_COMMAND_DISK3_EJECT	23

/*****************************************************************************/

#define VCC_PAK_FD502_ID	    XFOURCC('c','f','d','i')

#define ASSERT_FD502(pFD502)	assert(pFD502 != NULL && "NULL pointer"); \
								assert(pFD502->pak.device.id == EMU_DEVICE_ID && "Invalid Emu device ID"); \
								assert(pFD502->pak.device.idModule == VCC_COCOPAK_ID && "Invalid module ID"); \
								assert(pFD502->pak.device.idDevice == VCC_PAK_FD502_ID && "Invalid device ID")

// TODO: move to private header
typedef struct fd502_t fd502_t;
struct fd502_t
{
	cocopak_t			pak;				// base
	
	wd1793_t			wd1793;
	
	cocopak_t *			pDistoRTC;
    
    char *              confRomPath;
    char *              confLastPath;
    bool                confTurboDisk;
    bool                confDistoRTCEnabled;
    int                 confDistoRTCAddress;
} ;

/*****************************************************************************/

#if (defined __cplusplus)
extern "C"
{
#endif
	
	XAPI_EXPORT cocopak_t * vccpakModuleCreate(void);
	
#if (defined __cplusplus)
}
#endif

/*****************************************************************************/

#endif
