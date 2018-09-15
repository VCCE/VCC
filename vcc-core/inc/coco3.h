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

#ifndef _coco3_h_
#define _coco3_h_

/*****************************************************************************/

#include "machine.h"
#include "cpu.h"
#include "tcc1014.h"

#include "cassette.h"
#include "mc6821.h"
#include "audio.h"
#include "screen.h"
#include "keyboard.h"
#include "joystick.h"

#include "pakinterface.h"

#include "file.h"

/*****************************************************************************/
/*
	CoCo3 interface ports for peripherals (see peripheral manager)
 */
#define COCO_PORT_CASSETTE			(1 << 0)
#define COCO_PORT_SERIAL			(1 << 1)
#define COCO_PORT_JOYSTICK1			(1 << 2)
#define COCO_PORT_JOYSTICK2			(1 << 3)

/*****************************************************************************/
/*
	Coco device commands
 */

#define COCO3_COMMAND_NONE              0
#define COCO3_COMMAND_THROTTLE          1
#define COCO3_COMMAND_CPUPATH_SET       2
#define COCO3_COMMAND_CPUPATH_CLEAR     3
#define COCO3_COMMAND_ROMPATH_SET       4
#define COCO3_COMMAND_ROMPATH_CLEAR     5

#define COCO3_COMMAND_CPU               10  // (range 10 to 10+cputypes-1)

#define COCO3_COMMAND_RAM               20  // (range 20 to 20+ramconfig_e-1)

#define COCO3_COMMAND_OVERCLOCK         100 // (range 100 to 100+OVERCLOCK_MAX-1)

/*****************************************************************************/
/*
    CoCo3 persistent config
 */

#define EMU_DEVICE_NAME_COCO3		"CoCo 3"

#define CONF_SECTION_SYSTEM			"SYSTEM"

#define CONF_SETTING_CPU			"CPU"            /**< CPU type (built in - 0=6809, 1=6309 - if CONF_SETTING_CPUPATH not set) */
#define CONF_SETTING_CPUPATH		"CPUPath"        /**< CPU Path (if external) */
#define CONF_SETTING_OVERCLOCK		"Overclock"      /**< CPU Overclock multiplier (1-n) */
#define CONF_SETTING_THROTTLE		"Throttle"       /**< Frame rate throttle enable */
#define CONF_SETTING_RAM			"RAM"            /**< RAM size (0 = 128k, 1=512k, 2=1024k, 3=2048k, 4=8192k) */
#define CONF_SETTING_ROMPATH		"ROMPath"        /**< external ROM path */


#define OVERCLOCK_MAX 200   // TODO: set this to something appropriate

/*****************************************************************************/

#define COCO3_DEVEVENT_SCREEN_REFRESH   1

/*****************************************************************************/
/**
	forward declarations
 */
typedef struct coco3_t coco3_t;

/*****************************************************************************/
/**
	Callback definitions
 */
typedef void (*audioeventfn_t)(coco3_t *);

/*****************************************************************************/

#define VCC_COCO3_ID		XFOURCC('e','c','c','3')

#if (defined DEBUG)
#define ASSERT_CC3(pCoco3)	assert(pCoco3 != NULL);	\
							assert(pCoco3->machine.device.id == EMU_DEVICE_ID); \
							assert(pCoco3->machine.device.idModule == EMU_MACHINE_ID); \
                            assert(pCoco3->machine.device.idDevice == VCC_COCO3_ID)
#else
#define ASSERT_CC3(pCoco3)
#endif

#define CAST_CC3(p)          (coco3_t *)p;

typedef enum cputype_e
{
    eCPUType_MC6809 = 0,
    eCPUType_HD6309,
    eCPUType_Custom
} cputype_e;

typedef struct coco3settings_t
{
    // coco3/machine config settings
    int                 cpuType;                /**< 6809 or 6309 - TODO: change to load plug-in CPU dlib */
    int                 ramSize;                /**< RAM size (0 = 128k, 1=512k, 2=1024k, 3=2048k, 4=8192k) See: ramconfig_e */
    int                 cpuOverClock;           /**< CPU Over clock rate - 1-n */
    int                 frameThrottle;          /**< Frame rate throttle - 0 or 1 */
    char *              cpuPath;                /**< Path to CPU dynamic library - defaults to internal CPU modules if not set */
    char *              externalBasicROMPath;   /**< path to custom extended basic ROM */
} coco3settings_t;

/**
 */
struct coco3_t
{
    machine_t           machine;
    
    coco3settings_t     conf;                   /**< Saved settings */
    coco3settings_t     run;                    /**< Current run time settings */
    
	/*
		handy direct pointers to required devices for quick reference
		can also all be found via emudevice_t device tree
        in machine_t above
	*/
	gime_t *			pGIME;					/**< GIME chip emulation object (Graphics, Interrupts, Memory) */
	mc6821_t *			pMC6821;				/**< MC6821 PIA (X 2) */
    keyboard_t *        pKeyboard;              /**< keyboard */
    screen_t *          pScreen;                /**< Screen/Display - split into video + monitor? */

    // i/o bus
    cocopak_t *         pPak;                    /**< expansion bus plug-in Pak Interface */
    
    // peripherals
    cassette_t *		pCassette;				/**< Cassette peripheral */
    joystick_t *        pJoystick;              /**< joystick */

    /* audio related - TODO: this should be part of audio object */
    audio_t *           pAudio;                 /**< CoCo 3 audio emulation object */
    unsigned short      SoundRate;
    int                 SndEnable;
    unsigned char       SoundOutputMode;
    int                 AudioIndex;
    unsigned int *      pAudioBuffer;
    unsigned char *     pCassBuffer;
    audioeventfn_t      AudioEvent;
    
    // TODO: add peripherals
    // TODO: add add-ons
    
    /*
        emulation loop - TODO: document
	 */
	double				CycleDrift;
};

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
    coco3_t *		cc3Create(void);

	result_t		cc3EmuDevCommand(emudevice_t * pEmuDev, int iCommand, int iParam);

	bool			cc3PowerCycle(coco3_t * pCoco3);
	bool			cc3Reset(coco3_t * pCoco3);
	
	void			cc3RenderFrame(coco3_t * pCoco3);
	void			cc3CopyGrayFrame(coco3_t * pCoco3);

	unsigned char	cc3SetSndOutMode(coco3_t * pCoco3, unsigned char);
	unsigned short	cc3SetAudioRate(coco3_t * pCoco3, unsigned short);
	
    int             cc3ConfGetOverClock(coco3_t * pCoco3);
    void            cc3ConfSetOverClock(coco3_t * pCoco3, int multiplier);

#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif
