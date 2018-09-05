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

#ifndef _emuDevice_h_
#define _emuDevice_h_

/*****************************************************************************/

#include "xTypes.h"
#include "event.h"
#include "config.h"
#include "menu.h"
#include "slinklist.h"

/*****************************************************************************/

#include <stdarg.h>
#include <stdio.h>

#if defined(__clang__) || defined(__GNUC__)
#   define IS_PRINTF(IDXFMT,IDXARGS)       __attribute__ ((format (printf, IDXFMT, IDXARGS)))
#   define IS_PRINTF_APPLE(IDXFMT,IDXARGS) __attribute__ ((format (__NSString__, IDXFMT, IDXARGS)))
#else
#   define IS_PRINTF(IDXFMT,IDXARGS)
#   define IS_PRINTF_APPLE(IDXFMT,IDXARGS)
#endif

typedef enum loglevel_e
{
    all = 0,
    prod,
    qa,
    dev,
    verbose
} loglevel_e;

/*****************************************************************************/

/* forward declaration - see machine.h for definition */
typedef struct machine_t machine_t;
typedef struct emudevice_t emudevice_t;

/*****************************************************************************/

#define EMU_DEVICE_ID	XFOURCC('e','d','e','v')

#if (defined DEBUG)
#   define ASSERT_EMUDEV(pEmuDev)	assert(pEmuDev != NULL);	\
								    assert(pEmuDev->id == EMU_DEVICE_ID);
#else
#   define ASSERT_EMUDEV(pEmuDev)
#endif

#define MODULE_NAME_LENGTH	256

/*
	UI commands are emulator device IDs combined with the device specific command
 */
#define EMUDEV_UICOMMAND_CREATE(pEmuDev,cmd)	( (((pEmuDev)->iCommandID) << 16) | ((cmd)&0xFFFF) )
#define EMUDEV_UICOMMAND_GETDEVID(uicmd)		( ((uicmd) >> 16) & 0x7FFF )
#define EMUDEV_UICOMMAND_GETDEVCMD(uicmd)		( (uicmd) & 0xFFFF )

/*****************************************************************************/

#define COMMAND_STATE_NONE		-1
#define COMMAND_STATE_OFF		0
#define COMMAND_STATE_ON		1

/*****************************************************************************/


typedef struct version_t version_t;
struct version_t
{
	int			iMajor;						/**< Version #: <iMajor>.<iMinor>.<iRevision>.<build> */
	int			iMinor;
	int			iRevision;
    int         iBuild;
} ;

/*****************************************************************************/

typedef result_t    (*emudevfn_t)(emudevice_t * pEmuDev);

typedef result_t    (*emudevinitfn_t)(emudevice_t * pEmuDev);
typedef result_t    (*emudeveventfn_t)(emudevice_t * pEmuDev, event_t * event);

typedef result_t    (*emudevresetfn_t)(emudevice_t * pEmuDev);
typedef result_t    (*emudevdestroyfn_t)(emudevice_t * pEmuDev);
typedef result_t    (*emudevsavefn_t)(emudevice_t * pEmuDev, config_t * config);
typedef result_t    (*emudevloadfn_t)(emudevice_t * pEmuDev, config_t * config);
typedef result_t    (*emudevgetstatusfn_t)(emudevice_t * pEmuDev, char * pszText, size_t szText);
typedef result_t    (*emudevcreatemenufn_t)(emudevice_t * pEmuDev);
typedef result_t    (*emudevcommandfn_t)(emudevice_t * pEmuDev, int iCommand, int iParam);
typedef bool        (*emudevvalidatefn_t)(emudevice_t * pEmuDev, int iCommand, int * piState);

typedef result_t    (*emudevenumcbfn_t)(emudevice_t * pEmuDev, void * pUser);

/*****************************************************************************/
/**
 */

// forward declaration
typedef struct emurootdevice_t emurootdevice_t;

/**
    BAse for each emulated device
 */
struct emudevice_t
{
	id_t					id;							/**< Always EMU_DEVICE_ID */
	id_t					idModule;					/**< module ID eg. VCC_COCOPAK_ID, EMUDEV_CPU, etc */
	id_t					idDevice;					/**< device ID eg. VCC_PAK_EMUDSK, VCC_PAK_FD502 */

    machine_t *             pMachine;                   /**< Pointer to the machine/root object (set manually) */
    
	int					    iCommandID;                 /**< This device's menu command id */
	hmenu_t					hMenu;                      /**< Menu handle for this device */
    // TODO: this will need to be a callback for the MPI
    hmenu_t                 hChildHookMenu;             /**< Menu handle for this device */

	char					Name[MODULE_NAME_LENGTH];	/**< device name */
	
	version_t				verDevice;					/**< version of device */
	version_t				verVCC;						/**< made for version of VCC */
	version_t				verVCCMin;					/**< minimum version of VCC */

    emurootdevice_t *       pRoot;                      /**< root device pointer (cached on-demand value) */
    
    emudevice_t *			pParent;					/**< parent device */
	emudevice_t *			pSibling;					/**< next/sibling device list */
	emudevice_t *			pChild;						/**< child device list */
    
    emudevinitfn_t          pfnInit;                    /**< one time initialization callback */
    emudeveventfn_t         pfnEventHandler;            /**< event handler */

    emudevresetfn_t         pfnReset;                   /**< reset callback */
	emudevdestroyfn_t		pfnDestroy;					/**< destructor callback */
    
	emudevsavefn_t			pfnSave;					/**< save config callback */
	emudevloadfn_t			pfnLoad;					/**< load config callback */
	
	emudevgetstatusfn_t		pfnGetStatus;				/**< get status text */
	emudevcreatemenufn_t	pfnCreateMenu;				/**< create menu for device */
	emudevvalidatefn_t		pfnValidate;				/**< validate command for device */
	emudevcommandfn_t		pfnCommand;					/**< perform command */
} ;

/*****************************************************************************/

typedef result_t (*emudevgetpathfn_t)(emudevice_t * pEmuDev, char ** ppPath);
// TODO: variable parameters/printf like
typedef result_t (*emudevlogfn_t)(emudevice_t * pEmuDev, const char * pMessage);

typedef struct emurootdevice_t
{
    emudevice_t             device;
    
    void *                  pRootObject;                /**< root object (system specific - eg. NSDocument) */
    emudevgetpathfn_t       pfnGetDocumentPath;         /**< Return document save path (root device) */
    emudevgetpathfn_t       pfnGetAppPath;              /**< Return internal app path (root device) */
    emudevlogfn_t           pfnLog;
    
    slinklist_t             llistEventListeners[eEventCount];
} emurootdevice_t;

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
    /**
        Register device
     
        This does nothing more than set a unique command ID used in the menus
        for the specified device
     */
	XAPI result_t		emuDevRegisterDevice(emudevice_t * pEmuDevice);
	
    /**
        Add emulator device as a child to another
     */
    XAPI result_t        emuDevAddChild(emudevice_t * pEmuDevParent, emudevice_t * pEmuDevChild);
    XAPI result_t        emuDevRemoveChild(emudevice_t * pEmuDevParent, emudevice_t * pEmuDevChild);
    
    /**
        get parent by id
     
        This only walks up the device tree towards the root
     */
	XAPI emudevice_t *	emuDevGetParentModuleByID(emudevice_t * pEmuDevice, id_t idDevice);
	XAPI emudevice_t *	emuDevFindByCommandID(emudevice_t * pEmuDevice, int iCommandID);
    XAPI emudevice_t *  emuDevFindByDeviceID(emudevice_t * pEmuDevice, id_t idDevice);
    XAPI emudevice_t *  emuDevFindByModuleID(emudevice_t * pEmuDevice, id_t idModule);

	XAPI result_t		emuDevConfSave(emudevice_t * pEmuDevice, config_t * config);
	XAPI result_t		emuDevConfLoad(emudevice_t * pEmuDevice, config_t * config);

    XAPI result_t       emuDevSendCommand(emudevice_t * pEmuDevice, int command, int state);
    XAPI result_t       emuDevSetCommandState(emudevice_t * pEmuDevice, int command, int state);
    XAPI result_t       emuDevGetCommandState(emudevice_t * pEmuDevice, int command, int * pstate);

    XAPI result_t       emuDevInit(emudevice_t * pEmuDevice);
    XAPI result_t       emuDevReset(emudevice_t * pEmuDevice);
	XAPI result_t		emuDevDestroy(emudevice_t * pEmuDevice);

	XAPI result_t		emuDevEnumerate(emudevice_t * pEmuDevice, emudevenumcbfn_t pfnEnumCB, void * pUser);

    XAPI hmenu_t        emuDevGetMenuFromParent(emudevice_t * pEmuDevice);
    
    //
    // root device functions
    //
    
    /**
        Get the root device.
     
        @param emuDevice This can be any device in the tree.  If it does not have a cached value
                for the root device it will search up the tree for it and cache it for the next call.
     */
    XAPI emurootdevice_t *  emuDevGetRootDevice(emudevice_t * pEmuDevice);
    
    XAPI result_t       emuDevGetDocumentPath(emudevice_t * pEmuDevice, char ** ppSavePath);
    XAPI result_t       emuDevGetAppPath(emudevice_t * pEmuDevice, char ** ppSavePath);
    
    XAPI result_t       emuRootDevAddListener(emurootdevice_t * pRootDevice, emudevice_t * pEmuDevice, event_e type);
    
    /**
        Dispatch event to all devices listening for the given type of event
     
        @param rootDevice The root device.
     */
    XAPI result_t       emuRootDevDispatchEvent(emurootdevice_t * rootDevice, event_t * event);
    
    /**
        Send an event up the device tree
     */
    XAPI result_t       emuDevSendEventUp(emudevice_t * pEmuDevice, event_t * event);
    
    /*
        Logging
     */
    XAPI result_t       emuDevLog(emudevice_t * pEmuDevice, const char * format, ...) IS_PRINTF(2,3);
    
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif // _emuDevice_h_

