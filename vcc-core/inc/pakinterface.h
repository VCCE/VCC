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

#ifndef _pakinterface_h_
#define _pakinterface_h_

/****************************************************************************/

#include "cpu.h"
#include "tcc1014.h"
#include "xDynLib.h"
#include "emuDevice.h"
#include "rom.h"

/****************************************************************************/
/*
	Module/Pak status flags
 */
#define HASCONFIG		(1 << 0)
#define HASIOWRITE		(1 << 1)
#define HASIOREAD		(1 << 2)
#define NEEDSCPUIRQ		(1 << 3)
#define DOESDMA			(1 << 4)
#define NEEDHEARTBEAT	(1 << 5)
#define ANALOGAUDIO		(1 << 6)
#define CSWRITE			(1 << 7)
#define CSREAD			(1 << 8)
#define RETURNSSTATUS	(1 << 9)
#define CARTRESET		(1 << 10)
//#define SAVESINI		(1 << 11)
#define ASSERTCART		(1 << 12)

/****************************************************************************/
/*
	Pak load results
 */
#define LOADPAK_SUCCESS		0
#define LOADPAK_NOMODULE	1
#define LOADPAK_NOTVCC		2

/****************************************************************************/
/*
 */
#define MODULE_NAME_LENGTH	256

/****************************************************************************/
/**
	Plug-in DLL/dlib module creation function name definition
 */
#define VCCPLUGIN_FUNCTION_MODULECREATE			"vccpakModuleCreate"

/****************************************************************************/
/*
 Function prototypes for plug-in API
 */

typedef struct cocopak_t cocopak_t;

typedef cocopak_t *		(*pakcreatefn_t)(void);                                 /**<< Pak API create object dynamicly linked function */
typedef void			(*pakmodulestatusfn_t)(cocopak_t * pPak, char *);   /**<< Pak API get status */
typedef void			(*pakheartbeatfn_t)(cocopak_t * pPak);              /**<< Pak API 'ping' */
typedef unsigned short	(*pakmoduleaudiosamplefn_t)(cocopak_t * pPak);      /**<< Pak API Get audio sample */
typedef void			(*pakconfigfn_t)(cocopak_t * pPak, unsigned char);  /**<< Pak API Config */
typedef void			(*pakmoduleresetfn_t)(cocopak_t * pPak);			/**<< Pak API Reset */    // TODO: move to emudevice_t ?

/****************************************************************************/
/**
    plug-in function pointers for the pak/cartridge port
*/
typedef struct vccpakapi_t vccpakapi_t;
/**
 */
struct vccpakapi_t
{
	//pakcreatefn_t					pfnPakCreate;		/**<< Pak object creation is acquired dynamically from dlib/DLL */
	
    /** TODO: hard reset flag to reset function? */
    // TODO: use emuDevReset / emuDevInit
    
    pakmoduleresetfn_t				pfnPakReset;        /**<< reset Pak - anything specific to this device */
    pakmoduleaudiosamplefn_t		pfnPakAudioSample;  /**<< retrieve audio sample from Pak */
	pakheartbeatfn_t				pfnPakHeartBeat;	/**<< Pak 'heart beat' */
    //pakmodulestatusfn_t			pfnPakStatus;		/**<< TODO: Pak status callback */
    pakconfigfn_t					pfnPakConfig;       /**<< TODO: Pak configuration UI callback */

	portreadfn_t					pfnPakPortRead;		/**<< Pak i/o port read, not registered so it can be selected by multipak interface */
	portwritefn_t					pfnPakPortWrite;	/**<< Pak i/o port write, not registered so it can be selected by multipak interface */
	memread8fn_t					pfnPakMemRead8;		/**<< Pak memory read */
	memwrite8fn_t					pfnPakMemWrite8;	/**<< Pak memory write */
};

/****************************************************************************/

#define VCC_COCOPAK_ID		XFOURCC('c','c','p','k')
#define VCC_COCOPAK_ROM_ID	XFOURCC('v','r','o','m')

#define ASSERT_COCOPAK(pInstance)	assert(pInstance != NULL);	\
									assert(pInstance->device.id == EMU_DEVICE_ID); \
									assert(pInstance->device.idModule == VCC_COCOPAK_ID)

#define ASSERT_COCOPAK_ROM(pInstance)	assert(pInstance != NULL);	\
										assert(pInstance->device.id == EMU_DEVICE_ID); \
										assert(pInstance->device.idModule == VCC_COCOPAK_ID); \
										assert(pInstance->device.idDevice == VCC_COCOPAK_ROM_ID)

/****************************************************************************/
/**
    CoCo Pak / device emulator object
 */
struct cocopak_t
{
	emudevice_t		device;                     /**<< emulator device base */
	
	int 			type;                       /**<< Pak type */
	
	vccpakapi_t		pakapi;                     /**< pakapi functions for this module */

	/* dynamic library specific variables */
	char *	        pPathname;					/**<< Path to module/dynamic library */
	unsigned short	ModuleParams;				/**<< Module parameters / capabilities */

	bool			bCartWanted;				/**<< this pak/interface wants to assert CART */
	bool			bCartEnabled;				/**<< individual carts can disable assert CART? */

	rom_t			rom;						/**<< loaded ROM (if any) */
	pxdynlib_t		hinstLib;					/**<< Pointer to dynamic library instance */
};

/****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	/*
		cocopak
	 */
    cocopak_t *		pakCreate(void);	// TODO: deprecate? use pakLoadPAK only?
	
	int 			pakLoadPAK(const char * pPathname, cocopak_t ** ppPak);
	
    //const char *	pakGetModuleName(cocopak_t * pPak);
    bool			pakCanConfig(cocopak_t * pPak, int iMenuItem);
    
	void			pakReset(cocopak_t * pPak);
	void			pakConfig(cocopak_t * pPak, int iMenuItem);
	unsigned short	pakAudioSample(cocopak_t * pPak);

	unsigned char	pakPortRead (emudevice_t * pEmuDevice, unsigned char);
	void			pakPortWrite(emudevice_t * pEmuDevice, unsigned char, unsigned char);
	unsigned char	pakMem8Read (emudevice_t * pEmuDevice, unsigned short);
	void			pakMem8Write(emudevice_t * pEmuDevice, unsigned char, unsigned char);

#ifdef __cplusplus
}
#endif
		
/****************************************************************************/

#endif
