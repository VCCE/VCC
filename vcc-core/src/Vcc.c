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

/*****************************************************************/
/*
	This is the highest level of the emulator.  This provides an
	interface to the emulator and the user interface.
 
	Emulation is run on a separate thread, controlled by this
	API.
 
	TODO: add debugging helpers - RAM dump, log
	TODO: status indicators.  i.e. mini-icons like HD LED, graphical tape counter, etc
*/
/*****************************************************************/

#include "Vcc.h"

#include "tcc1014graphics.h"
#include "tcc1014mmu.h"
#include "screen.h"
#include "coco3.h"
#include "pakinterface.h"
#include "audio.h"
#include "mc6821.h"
#include "screen.h"
#include "joystick.h"
#include "config.h"
#include "file.h"
#include "path.h"

#include "xDebug.h"
#include "xSystem.h"

#if (defined _WIN32)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <stdio.h>

/*********************************************************************************/

#define STATUS_SEPARATOR		"|"

/*********************************************************************************/

#pragma mark -
#pragma mark --- Forward declarations ---

/*********************************************************************************/

int 	vccEmuLoop(void *);

/********************************************************************************/

void vccSettingsReleaseStorage(vccsettings_t * settings)
{
    if ( settings->pModulePath )
    {
        free(settings->pModulePath);
        settings->pModulePath = NULL;
    }
}

void vccSettingsCopy(const vccsettings_t * src, vccsettings_t * dst)
{
    if (src == NULL || dst == NULL) return;
    
    vccSettingsReleaseStorage(dst);
    
    //dst->cpuType = src->cpuType;
    if ( src->pModulePath != NULL) dst->pModulePath = strdup(src->pModulePath);
}

/**
 @return 0 if the same, 1 if different
 */
int vccSettingsCompare(const vccsettings_t * set1, const vccsettings_t * set2)
{
    if (set1 == NULL || set2 == NULL) return 0;
    
    if (   /* set1->cpuType == set2->cpuType
        && */ (   (set1->pModulePath == NULL && set2->pModulePath == NULL)
            || (   set1->pModulePath != NULL && set2->pModulePath != NULL
                && strcmp(set1->pModulePath,set2->pModulePath) == 0
                )
            )
        )
    {
        return 0;
    }
    
    return 1;
}

/********************************************************************/

#pragma mark -
#pragma mark --- Configuration ---

/*********************************************************************************/

#define SYS_PREF_VCC_LAST_PATH "vccLastPakPath"

void vccSetLastPath(vccinstance_t * pInstance, const char * pPathname)
{
    sysSetPreference(SYS_PREF_VCC_LAST_PATH, pPathname);
}

const char * vccGetLastPath(vccinstance_t * pInstance)
{
    return sysGetPreference(SYS_PREF_VCC_LAST_PATH);
}

/*********************************************************************************/
/**
 */
void vccSetModulePath(vccinstance_t * pInstance, const char * pPathname)
{
	if ( pPathname != pInstance->run.pModulePath )
	{
		if ( pInstance->run.pModulePath != NULL )
		{
			// NOTE: Do not reference pPathname after this in case it was the same as pInstance->CurrentConfig.pModulePath
			free(pInstance->run.pModulePath);
			pInstance->run.pModulePath = NULL;
		}
		
        char * newPath = NULL;
        if ( pPathname != NULL )
        {
            newPath = strdup(pPathname);
        }
        
        pInstance->run.pModulePath = newPath;
	}
}

/********************************************************************/

#pragma mark -
#pragma mark --- Apply Configuration ---

/********************************************************************/
/**
	Apply current configuration to this instance
 
	Most things are handled via the power cycle command since each
	device handles its own setup, and its own current config.
 
	However, there are a few things we need to do at a high level
	currently.
 
	Emulation must be locked when this is called
 */
result_t vccConfigApply(vccinstance_t * pInstance)
{
	result_t	errResult = XERROR_NONE;
	
	assert(pInstance != NULL);
	assert(pInstance->pCoco3 != NULL);
	
	/* TODO: move to Coco3 power cycle or have a 'hard reset' option for all devices? */
	if ( pInstance->run.pModulePath != NULL )
	{
		errResult = vccLoadPak(pInstance, pInstance->run.pModulePath);
		if ( errResult == XERROR_NONE )
		{
			/* 'load' / extract settings into pak device tree (multiple if it is an MPI with other paks) */
			errResult = emuDevConfLoad(&pInstance->pCoco3->pPak->device,pInstance->config);
            
            if ( errResult == XERROR_NONE )
            {
                /*
                    power cycle the system
                 */
                vccSetCommandPending(pInstance,VCC_COMMAND_POWERCYCLE);
            }
		}
		else
		{
            emuDevLog(&pInstance->root.device, "Error loading pak: %s", pInstance->run.pModulePath);

            /* error loading, clear module path */
			free(pInstance->run.pModulePath);
			pInstance->run.pModulePath = NULL;
		}
	}
    
    return errResult;
}

/*********************************************************************************/

#pragma mark -
#pragma mark ---Load /save ---

/*********************************************************************************/
/**
 Load configuration from file, then apply it
 
 This is done in multiple stages.
 
 1. Load config from file
 2. 'load' in-memory configuration into the emulator devices
 3. apply the configuration
 this recreates the emulation objects as-needed
 and handled loading of the pak(s)
 */
result_t vccLoad(vccinstance_t * pInstance, const char * pPathname)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	
	if ( pInstance != NULL )
	{
		/*
			lock emulation while we change things
		 */
		vccEmuLock(pInstance);
		
		/* we should never have an in-mem config object when loading */
		assert(pInstance->config == NULL);
		
		/* 
            create/load cross platform 'config' object
            this loads the file and allows us to extract the settings
            that were loaded from the save file
		 */
		pInstance->config = confCreate(pPathname);
		assert(pInstance->config != NULL);
		
		// 'load' (extract) settings into the entire device tree
		errResult = emuDevConfLoad(&pInstance->root.device,pInstance->config);
        if ( errResult != XERROR_NONE )
        {
            // TODO: log message?
        }
        
		/*
            update current instance to the new configuration
		 */
		errResult = vccConfigApply(pInstance);
		
		/*
			unlock emulation
		 */
		vccEmuUnlock(pInstance);
	}
	
	return errResult;
}

/*********************************************************************************/
/**
 */
result_t vccSave(vccinstance_t * pInstance, const char * pathname, const char * refPath)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	
	if ( pInstance != NULL )
	{
		/* 
            create cross platform 'config' object for storing settings in.
		 */
		if ( pInstance->config == NULL )
		{
			pInstance->config = confCreate(refPath);
			assert(pInstance->config != NULL);
		}
		
		// save entire device tree to config object
		/* errResult = */ emuDevConfSave(&pInstance->root.device,pInstance->config);
		
		/*
            actually save the configuration to the file chosen by the user
		 */
		errResult = confSave(pInstance->config, pathname);
	}
	
	return errResult;
}

/*********************************************************************************/

#pragma mark -
#pragma mark --- Emulator Control ---

/*********************************************************************************/
/**
	Set pending command for next time through the emulation loop
 */
void vccSetCommandPending(vccinstance_t * pInstance, vcc_commands_e iCommand)
{
	assert(pInstance != NULL);
	
    if (    iCommand == VCC_COMMAND_POWERCYCLE
         || iCommand == VCC_COMMAND_POWERON
       )
    {
        if (    pInstance->CommandPending != VCC_COMMAND_POWERCYCLE
             && pInstance->CommandPending != VCC_COMMAND_POWERON
           )
        {
            assert(pInstance->CommandPending == VCC_COMMAND_NONE);
        }
    }
    else
    {
        assert(pInstance->CommandPending == VCC_COMMAND_NONE);
    }
    
	pInstance->CommandPending = iCommand;
}

/*********************************************************************************/
/**
	Load a Pak - ROM Pak or Device Pak
 
    TODO: move to Coco3
    TODO: make more generic so MPI can use it
 */
result_t vccLoadPak(vccinstance_t * pInstance, const char * pPathname)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	bool		bDoReset	= FALSE;
	
    /*
     TODO: update pakInterface call, it should include most of this code
     */
	if ( pInstance != NULL )
	{
		// lock the Emu thread
		vccEmuLock(pInstance);
		
		// remove existing pak from device tree
		if ( pInstance->pCoco3->pPak != NULL ) 
		{
			// destroy existing pak
			errResult = emuDevDestroy(&pInstance->pCoco3->pPak->device);
			pInstance->pCoco3->pPak = NULL;
			
            vccSetModulePath(pInstance,NULL);
			
			bDoReset = TRUE;
		}

		if ( pPathname != NULL )
		{
			/*
				Load new pak
			 */
			if ( pakLoadPAK(pPathname,&pInstance->pCoco3->pPak) == LOADPAK_SUCCESS )
			{
				assert(pInstance->pCoco3->pPak != NULL);
				
				vccSetModulePath(pInstance,pPathname);
				
				// add to device tree
				emuDevAddChild(&pInstance->pCoco3->machine.device,&pInstance->pCoco3->pPak->device);
                pInstance->pCoco3->pPak->device.pMachine = &pInstance->pCoco3->machine;
                
                emuDevLog(&pInstance->root.device,"Successfully loaded Pak: %s", pPathname);

				bDoReset = TRUE;
				
				errResult = XERROR_NONE;
			}
            else
            {
                emuDevLog(&pInstance->root.device,"Error loading Pak: %s", pPathname);
            }
		}
	
		if ( bDoReset )
		{
			// hard reset after changing a cart
			vccSetCommandPending(pInstance,VCC_COMMAND_POWERCYCLE);
		}
		
		// release (unpause) the Emu thread
		vccEmuUnlock(pInstance);
	}
	
	return errResult;
}

/*********************************************************************************/

void vccScreenShot(emurootdevice_t * rootDevice)
{
    vccinstance_t * pInstance = (vccinstance_t *)rootDevice;
    
    ASSERT_VCC(pInstance);
    if ( pInstance != NULL )
    {
        if ( pInstance->pfnScreenShot != NULL )
        {
            (*pInstance->pfnScreenShot)(pInstance);
        }
    }
}

/*********************************************************************************/

void vccUpdateUI(vccinstance_t * pInstance)
{
	if ( pInstance != NULL )
	{
        vccEmuLock(pInstance);
        
		if ( pInstance->pfnUIUpdate != NULL )
		{
			(*pInstance->pfnUIUpdate)(pInstance);
		}
        
        vccEmuUnlock(pInstance);
	}
}

/*********************************************************************************/
/**
 */
void vccEmuLock(vccinstance_t * pInstance)
{
	if ( pInstance != NULL )
	{
		/* set lock, emulator will pause at start of next loop */
		SDL_LockMutex(pInstance->hEmuMutex);
		
        if (    pInstance->pCoco3 != NULL
             && pInstance->pCoco3->pAudio != NULL
            )
        {
            audPauseAudio(pInstance->pCoco3->pAudio,TRUE);
        }
	}
}

/*********************************************************************************/
/**
 */
void vccEmuUnlock(vccinstance_t * pInstance)
{
	if ( pInstance != NULL )
	{
		assert(pInstance != NULL);
		
        if (    pInstance->pCoco3 != NULL
            && pInstance->pCoco3->pAudio != NULL
            )
        {
            audPauseAudio(pInstance->pCoco3->pAudio,FALSE);
        }
        
		/* release the lock */
        if ( pInstance->hEmuMutex != NULL )
        {
            SDL_UnlockMutex(pInstance->hEmuMutex);
        }
	}
}

/*********************************************************************************/
/*
 Emulator loop / thread
 */
int vccEmuLoop(void * pParam)
{
	vccinstance_t *	pInstance	= (vccinstance_t *)pParam;
	int 			iCommand;
	
	assert(pInstance != NULL);
	
	thrInit(&pInstance->FrameThrottle);
	
	/* main emulation loop */
	while ( TRUE ) 
	{
		/*
		 lock out changes while running the current frame
		 the UI locks this while it is changing something
		 once the UI releases, we can continue
		 */
		SDL_LockMutex(pInstance->hEmuMutex);
		
		/*
		 Check emulator thread state (controlled by UI)
		 We check here so the UI can lock this thread, change the state
		 and then release the lock, and expect that we will
		 exit immediately
		 */
		if ( pInstance->EmuThreadState == emuThreadStop )
		{
			// unlock
			SDL_UnlockMutex(pInstance->hEmuMutex);
			// and exit
			break;
		}
		
		/*
		 check for pending command
		 */
		while ( pInstance->CommandPending != VCC_COMMAND_NONE )
		{
			iCommand = pInstance->CommandPending;
			pInstance->CommandPending = VCC_COMMAND_NONE;
			
			vccEmuDevCommand(&pInstance->root.device,iCommand,0);
		}

		/* initiate start of frame for frame throttler */
		thrStartRender(&pInstance->FrameThrottle);
		{
			switch ( pInstance->EmuThreadState )
			{
                // emulator is running
				case emuThreadRunning:
					/* render the current frame, this actually runs the emulation */
					cc3RenderFrame(pInstance->pCoco3);
				break;
					
                // emulator is paused
				case emuThreadPause:
					/* copy current video buffer dimmed/gray */
					//cc3CopyGrayFrame(pInstance->pCoco3);
					
					sysSleep(1);
				break;
					
                // emulator is powered off
				case emuThreadOff:
					/* off state - leaves video frame static */
					screenStatic(pInstance->pCoco3->pScreen);
					
					sysSleep(1);
				break;
					
				default:
					assert(0 && "unhandled emulator thread state");
			}
		}
		/* flag end of rendered frame */
		thrEndRender(&pInstance->FrameThrottle);
		
		/*
             Update status of various things displayed in UI
             This just updates the VCC instance status text member
             It is shown via the display update callback via
             screenDisplayFlip() called just below
		 */
		vccUpdateStatus(pInstance);
		
		/*
			update screen display with new frame
		 */
        screenDisplayFlip(pInstance->pCoco3->pScreen);
        
		/*
             unlock mutex for emulator thread
             UI can now do whatever it wants
		 */
		SDL_UnlockMutex(pInstance->hEmuMutex);
		
		// if throttling is enabled, wait here until the theoretical time for this frame has elapsed
		if ( pInstance->pCoco3->run.frameThrottle )
		{
			thrFrameWait(&pInstance->FrameThrottle);
		}
	} //Still Emulating
	
	// we have quit, make it known
	pInstance->EmuThreadState = emuThreadQuit;
	
	// exit thread, no error
	return 0;
}

/*********************************************************************************/

#pragma mark -
#pragma mark --- UI ---

/*********************************************************************************/

/**
	@param pInstance
	@param iCommand		Command ID/Command combo.  upper word = command ID of device

	Validate an emulator command.  Return whether it can be performed
	or not given the state of the emulator.  This is called from the
	platform to determine which menu items or toolbar items to activate
 
	We dispatch the validation request to the appropriate device
*/
bool vccCommandValidate(vccinstance_t * pInstance, int32_t iCommand, int32_t * piState)
{
	bool			bValid		= false;
	int32_t 		iCmdID;
	int32_t 		iCmdCommand;
	emudevice_t *	pEmuDevice;
	
	if ( pInstance == NULL )
	{
		// no emulator instance, nothing is enabled
		bValid = false;
	}
	else 
	{
		iCmdID		= EMUDEV_UICOMMAND_GETDEVID(iCommand);		// device specific commands for menu items created
		iCmdCommand	= EMUDEV_UICOMMAND_GETDEVCMD(iCommand);		// emulator commands as defined in Vcc.h
		
		/* find the target device */
		pEmuDevice = emuDevFindByCommandID(&pInstance->root.device,iCmdID);
		/* this should never happen unless a menu item was generated by that device */
		if ( pEmuDevice != NULL )
		{
			/* and a device that generates a menu item needs to also validate it */
			assert(pEmuDevice->pfnValidate != NULL && "device command has no validation handler");
			if ( pEmuDevice->pfnValidate != NULL )
			{
				/*
					ask device is this command is currently valid (enabled)
				 */
				bValid = (*pEmuDevice->pfnValidate)(pEmuDevice,iCmdCommand,piState);
			}
		}
        else
        {
            emuDevLog(&pInstance->root.device, "vccCommandValidate - Unable to find device for command: %d (%d/%d)",iCommand,iCmdID,iCmdCommand);
        }
	}
	
	return bValid;
}

/*********************************************************************************/
/**
	Execute an emulator command - called from the user interface
 
	We dispatch the command to the appropriate device
 */
result_t vccCommand(vccinstance_t * pInstance, int32_t iCommand)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	int			    iCmdID;
	int			    iCmdCommand;
	emudevice_t *	pEmuDevice;
	
	if ( pInstance != NULL )
	{
		iCmdID		= EMUDEV_UICOMMAND_GETDEVID(iCommand);		// device specific commands for menu items created
		iCmdCommand	= EMUDEV_UICOMMAND_GETDEVCMD(iCommand);		// emulator commands as defined in Vcc.h
		
        errResult = XERROR_NOT_FOUND;
        
		/* find the target device */
		pEmuDevice = emuDevFindByCommandID(&pInstance->root.device,iCmdID);
		/* since this should never happen unless a menu item was generated by that device */
		assert(pEmuDevice != NULL);
		if ( pEmuDevice != NULL )
		{
            errResult = XERROR_NONE;
            
			/* and a device that generates a menu item needs to also validate it */
			assert(pEmuDevice->pfnCommand != NULL && "no device command handler");
			if ( pEmuDevice->pfnCommand != NULL )
			{
				/*
					do device command
				 */
				errResult = (*pEmuDevice->pfnCommand)(pEmuDevice,iCmdCommand,0);
			}
		}
	}
	
	return errResult;
}

/*********************************************************************************/
/*
	emulator device enumaration callback for getting device status
 */
result_t vccUpdateStatusCB(emudevice_t * pEmuDev, void * pUser)
{
	vccinstance_t * pInstance = (vccinstance_t *)pUser;
	char			cTemp[256];
	
    cTemp[0] = 0;
    
	/* does this device offer a status text? */
	if ( pEmuDev->pfnGetStatus != NULL )
	{
		/* get status text for this device */
		(*pEmuDev->pfnGetStatus)(pEmuDev,cTemp,sizeof(cTemp)-1);
		
        if ( strlen(cTemp) > 0 )
        {
            // TODO: should not be an assertion, should just skip appending the string
            // string length must be able to handle current string, new string, separator, and NULL
            assert(strlen(pInstance->cStatusText) + strlen(cTemp) + strlen(STATUS_SEPARATOR) + 1 <= sizeof(pInstance->cStatusText));
		
            // add separator
            strcat(pInstance->cStatusText,STATUS_SEPARATOR);
            
            // add device status text
            strcat(pInstance->cStatusText,cTemp);
        }
	}
	
	return XERROR_NONE;
}

/**
	Update user interface
 
	update status display
*/
result_t vccUpdateStatus(vccinstance_t * pInstance)
{
	result_t	errResult = XERROR_INVALID_PARAMETER;
	
	if ( pInstance != NULL )
	{
		// limit to every so often - 1/60s
		if ( pInstance->tmStatus + 1000/60 < xTimeGetMilliseconds() )
		{
			// clear status text
			strcpy(pInstance->cStatusText,"");

			/*
             walk all devices with our status update callback
			 */
			emuDevEnumerate(&pInstance->root.device,vccUpdateStatusCB,pInstance);
		
			pInstance->tmStatus = xTimeGetMilliseconds();
		}
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/*********************************************************************************/

#pragma mark -
#pragma mark --- EmuDevice callbacks ---

/********************************************************************************/

bool vccEmuDevConfCheckDirty(emudevice_t * pEmuDevice)
{
    vccinstance_t * pInstance = (vccinstance_t *)pEmuDevice;

    ASSERT_VCC(pInstance);
    if ( pInstance != NULL )
    {
        return (vccSettingsCompare(&pInstance->run,&pInstance->conf) != 0);
    }
    
    return false;
}

/********************************************************************/
/**
	EmuDev callback to save configuration
 */
result_t vccEmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	//vccconfig_t *	pConfig;
	vccinstance_t * pInstance;
	
	pInstance = (vccinstance_t *)pEmuDevice;
	
	ASSERT_VCC(pInstance);
	
    vccSettingsCopy(&pInstance->run, &pInstance->conf);
    
    confSetPath(config, VCC_CONF_SECTION, VCC_CONF_SETTING_PAK, pInstance->conf.pModulePath, config->absolutePaths);

	return errResult;
}

/********************************************************************/
/**
	Emu Device load callback.  This is just for 'loading' the 
	settings for this specific device from the cross platform
	configuration object.
 
	TODO: load needs proper error checking and reporting to account for manual edits
 */
result_t vccEmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	vccinstance_t * pInstance;
	
	pInstance = (vccinstance_t *)pEmuDevice;
	
	ASSERT_VCC(pInstance);
	
	/*
	 Module
	 */
	confGetPath(config, VCC_CONF_SECTION, VCC_CONF_SETTING_PAK, &pInstance->conf.pModulePath,config->absolutePaths);
	
    vccSettingsCopy(&pInstance->conf, &pInstance->run);

	return errResult;
}

/*********************************************************************************/
/**
	emulator device callback to terminate this device
 */
result_t vccEmuDevDestroy(emudevice_t * pEmuDevice)
{
	vccinstance_t * pInstance;
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	
	pInstance = (vccinstance_t *)pEmuDevice;
	
	ASSERT_VCC(pInstance);
	
	if ( pInstance != NULL )
	{		
		// tell the emulation thread to stop
		pInstance->EmuThreadState = emuThreadStop;
		// we should be paused prior to this, so resume so it can exit
		vccEmuUnlock(pInstance);
		// wait for it to enter the quitting state, the thread will exit
		// and terminate immediately following that state change
		while (    pInstance->hEmuThread != NULL
                && pInstance->EmuThreadState != emuThreadQuit
               )
		{
			sysSleep(1);
		}
		
		/*
			destroy in-memory configuration
		 */
		if ( pInstance->config != NULL )
		{
			// TODO: save if dirty
			
			confDestroy(pInstance->config);
		}
		
		/*
			destroy the thread mutex object
		 */
        if ( pInstance->hEmuMutex != NULL )
        {
            SDL_DestroyMutex(pInstance->hEmuMutex);
            pInstance->hEmuMutex = NULL;
        }
        
		/*
			destroy add-on manager
		 */
        if ( pInstance->pAddOnMgr != NULL )
        {
            emuDevDestroy(&pInstance->pAddOnMgr->device);
        }
        
		/*
			destroy peripheral manager
		 */
        if ( pInstance->pPeripheralMgr != NULL )
        {
            emuDevDestroy(&pInstance->pPeripheralMgr->device);
        }
        
		/* 
		 destroy CoCo3 object 
		 */
        if ( pInstance->pCoco3 != NULL )
        {
            emuDevDestroy(&pInstance->pCoco3->machine.device);
        }
        
		// no need to remove ourselves from the list, we are the parent device
		//emuDevRemoveChild(&pInstance->device.pParent->device,&pInstance->device);
		// however - we should have nothing linked anymore when we die
		assert(pInstance->root.device.pParent == NULL);
		assert(pInstance->root.device.pChild == NULL);
		assert(pInstance->root.device.pSibling == NULL);
		
		free(pInstance);
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/*********************************************************************************/
/**
	emulator device callback to get status text
 */
result_t vccEmuDevGetStatus(emudevice_t * pEmuDev, char * pszText, size_t szText)
{
	vccinstance_t *	pInstance = (vccinstance_t *)pEmuDev;
	
    if ( pInstance->tmFrame + 1000 < xTimeGetMilliseconds() )
    {
        thrCalculateFPS(&pInstance->FrameThrottle);

        pInstance->tmFrame = xTimeGetMilliseconds();
    }

    sprintf(pszText,"FPS: %0.2f",pInstance->FrameThrottle.fps);
	
    assert(strlen(pszText) < szText);
    
	return XERROR_NONE;
}

/*********************************************************************************/
/**
	create menu for VCC (highest level)
 */
result_t vccEmuDevCreateMenu(emudevice_t * pEmuDev)
{
	vccinstance_t *		pInstance		= (vccinstance_t *)pEmuDev;
	result_t			errResult		= XERROR_INVALID_PARAMETER;
	hmenu_t				hControlMenu	= NULL;
	hmenu_t				hCartMenu;
	char				Temp[256];
	char				Name[256];
	
	ASSERT_VCC(pInstance);
	if ( pInstance != NULL )
	{
		assert(pInstance->root.device.hMenu == NULL);

		// create new VCC main "Virtual Machine" menu
		pInstance->root.device.hMenu = menuCreate("Virtual Machine");
		
        // move to Coco3?
		hControlMenu = menuCreate("Control");
		menuAddSubMenu(pInstance->root.device.hMenu,hControlMenu);
		
		// add items to control menu
		menuAddItem(hControlMenu,"Power Off",	EMUDEV_UICOMMAND_CREATE(&pInstance->root.device,VCC_COMMAND_POWEROFF) );
		menuAddItem(hControlMenu,"Power On",	EMUDEV_UICOMMAND_CREATE(&pInstance->root.device,VCC_COMMAND_POWERON) );
		menuAddItem(hControlMenu,"Power Cycle",	EMUDEV_UICOMMAND_CREATE(&pInstance->root.device,VCC_COMMAND_POWERCYCLE) );
		menuAddItem(hControlMenu,"Run",			EMUDEV_UICOMMAND_CREATE(&pInstance->root.device,VCC_COMMAND_RUN) );
		menuAddItem(hControlMenu,"Pause",		EMUDEV_UICOMMAND_CREATE(&pInstance->root.device,VCC_COMMAND_STOP) );
		menuAddItem(hControlMenu,"Reset",		EMUDEV_UICOMMAND_CREATE(&pInstance->root.device,VCC_COMMAND_RESET) );

        // TODO: Move to CoCo3
		// create Pak/CART menu
		hCartMenu = menuCreate(CARTRIDGE_MENU);
		menuAddSubMenu(pInstance->root.device.hMenu,hCartMenu);
        
		// Load Pak menu item
		menuAddItem(hCartMenu,"Load", (pInstance->root.device.iCommandID<<16) | VCC_COMMAND_LOADCART);
		
		// set Eject menu item to have cart name
		strcpy(Temp,"Eject:");
		if ( pInstance->run.pModulePath != NULL )
		{
			pathGetFilename(pInstance->run.pModulePath,Name,sizeof(Name));
			sprintf(Temp,"Eject: %s",Name);
		}
		menuAddItem(hCartMenu, Temp, (pInstance->root.device.iCommandID<<16) | VCC_COMMAND_EJECTCART);
	}
	
	return errResult;
}

/**
 */
hmenu_t vccEmuDevGetParentMenu(emudevice_t * pEmuDev)
{
    vccinstance_t * pInstance = (vccinstance_t *)pEmuDev;

    ASSERT_VCC(pInstance);
    if ( pInstance != NULL )
    {
        // find cart menu
        hmenu_t hMenu = menuFind(pInstance->root.device.hMenu,CARTRIDGE_MENU);
        assert(hMenu != NULL);
        
        return hMenu;
    }
    
    return NULL;
}

/*********************************************************************************/
/**
	emulator device callback to validate a device command
 */
bool vccEmuDevValidate(emudevice_t * pEmuDev, int iCommand, int * piState)
{
	vccinstance_t *		pInstance	= (vccinstance_t *)pEmuDev;
	bool				bValid		= FALSE;
	
	ASSERT_VCC(pInstance);
	
	switch ( iCommand )
	{
		case VCC_COMMAND_POWEROFF:
			bValid = (pInstance->EmuThreadState == emuThreadRunning);
			break;
			
		case VCC_COMMAND_POWERON:
			bValid = (pInstance->EmuThreadState == emuThreadOff);
			break;
			
		case VCC_COMMAND_POWERCYCLE:
			bValid = (pInstance->EmuThreadState == emuThreadRunning);
			break;
			
		case VCC_COMMAND_RESET:
			bValid = (pInstance->EmuThreadState == emuThreadRunning);
			break;
			
		case VCC_COMMAND_RUN:
			bValid = (pInstance->EmuThreadState == emuThreadPause);
			break;
			
		case VCC_COMMAND_STOP:
			bValid = (pInstance->EmuThreadState == emuThreadRunning);
			break;
			
		case VCC_COMMAND_LOADCART:
			bValid = TRUE;
			break;
			
		case VCC_COMMAND_EJECTCART:
			bValid = (pInstance->pCoco3->pPak != NULL);
			break;
			
		default:
			assert(0 && "VCC command not recognized");
	}
	
	return bValid;
}

/*********************************************************************************/
/**
	emulator device callback to perform VCC high level command
 */
result_t vccEmuDevCommand(emudevice_t * pEmuDev, int iCommand, int iParam)
{
	result_t            errResult	= XERROR_NONE;
	vccinstance_t *		pInstance	= (vccinstance_t *)pEmuDev;
    bool                bUpdateUI   = false;
    
	ASSERT_VCC(pInstance);
	
	// lock emulator thread
	vccEmuLock(pInstance);
	
	// do command
	switch ( iCommand )
	{
		case VCC_COMMAND_RUN:
			pInstance->EmuThreadState = emuThreadRunning;
            bUpdateUI = true;
		break;
			
		case VCC_COMMAND_STOP:
			pInstance->EmuThreadState = emuThreadPause;
            bUpdateUI = true;
		break;
			
        case VCC_COMMAND_SETCART:
        {
            bool cart = (iParam!=0);
            mc6821_SetCART(pInstance->pCoco3->pMC6821,cart?Low:High);
            //bUpdateUI = true;
        }
        break;
            
		case VCC_COMMAND_POWEROFF:
			pInstance->EmuThreadState = emuThreadOff;
            bUpdateUI = true;
		break;
			
		case VCC_COMMAND_POWERON:
			pInstance->EmuThreadState = emuThreadRunning;
			// fall through to power cycle
		//break;

		case VCC_COMMAND_POWERCYCLE:
        {
            //if ( m_pInstance->EmuThreadState == emuThreadRunning )
            {
                screenClearBuffer(pInstance->pCoco3->pScreen);
                cc3PowerCycle(pInstance->pCoco3);
                bUpdateUI = true;
            }
        }
		break;
			
		case VCC_COMMAND_RESET:
        {
            //if ( m_pInstance->EmuThreadState == emuThreadRunning )
            {
                cc3Reset(pInstance->pCoco3);
                bUpdateUI = true;
            }
        }
		break;
			
		case VCC_COMMAND_LOADCART:
		{
			char *		pPathname;
			result_t	errResult;
			
            filetype_e types[] = { COCO_PAK_ROM, COCO_PAK_PLUGIN, COCO_FILE_NONE };
            
			pPathname = sysGetPathnameFromUser(&types[0],vccGetLastPath(pInstance));
			if ( pPathname != NULL )
			{
                vccSetLastPath(pInstance,pPathname);

                errResult = vccLoadPak(pInstance,pPathname);
				if ( errResult != XERROR_NONE )
				{
                    // TODO: show a useful error message
					sysShowError("Error loading PAK");
				}
			}
            bUpdateUI = true;
		}
		break;
			
		case VCC_COMMAND_EJECTCART:
			vccLoadPak(pInstance,NULL);
            bUpdateUI = true;
		break;
            
		default:
			assert(0 && "VCC command not recognized");
		break;
	}
	
    if ( bUpdateUI )
    {
        vccUpdateUI(pInstance);
    }
    
	// unlock emlator thread
	vccEmuUnlock(pInstance);
	
	return errResult;
}

/*********************************************************************************/

#pragma mark -
#pragma mark --- Init / Termination ---

/*********************************************************************************/
/**
    Initialize / start VCC

    This loads the current configuration, initializes the CPU and starts the
    emulation thread.

    This should be called from the main program / GUI during initialization
    when ready to display.
 */
vccinstance_t * vccCreate(const char * pcszName)
{
	vccinstance_t *	pInstance = NULL;
    result_t result = XERROR_GENERIC;
    
	pInstance = calloc(1,sizeof(vccinstance_t));
	assert(pInstance != NULL);
	if ( pInstance != NULL )
	{
        // we are the root device
        pInstance->root.device.pRoot = &pInstance->root;
        
		/*
            object/module identification
		 */
		pInstance->root.device.id		= EMU_DEVICE_ID;
		pInstance->root.device.idModule	= VCC_CORE_ID;
		strcpy(pInstance->root.device.Name,"VCC");
		
		// TODO: set from file / SVN revision
		pInstance->root.device.verDevice.iMajor      = 0;
		pInstance->root.device.verDevice.iMinor      = 5;        // (pre) Pre-alpha
		pInstance->root.device.verDevice.iRevision	= 0;
		
		/*
            emulator device callbacks
		 */
		pInstance->root.device.pfnDestroy	        = vccEmuDevDestroy;
		pInstance->root.device.pfnSave		        = vccEmuDevConfSave;
		pInstance->root.device.pfnLoad		        = vccEmuDevConfLoad;
        pInstance->root.device.pfnConfCheckDirty    = vccEmuDevConfCheckDirty;
		pInstance->root.device.pfnGetStatus	        = vccEmuDevGetStatus;
		pInstance->root.device.pfnCreateMenu        = vccEmuDevCreateMenu;
        pInstance->root.device.pfnGetParentMenu     = vccEmuDevGetParentMenu;
		pInstance->root.device.pfnValidate	        = vccEmuDevValidate;
		pInstance->root.device.pfnCommand	        = vccEmuDevCommand;
        
		emuDevRegisterDevice(&pInstance->root.device);
		
		/*
            initialize with default configuration
		 */
		pInstance->run.pModulePath = NULL;

        vccSettingsCopy(&pInstance->conf, &pInstance->run);

        // dummy while
        while ( true)
        {
            /*
                init coco3 object
             */
            pInstance->pCoco3 = cc3Create();
            if(pInstance->pCoco3 == NULL) break;
            emuDevAddChild(&pInstance->root.device,&pInstance->pCoco3->machine.device);
            
            /*
                create add-on manager
             */
            pInstance->pAddOnMgr		= aomInit();
            if(pInstance->pAddOnMgr == NULL) break;
            emuDevAddChild(&pInstance->root.device,&pInstance->pAddOnMgr->device);

            /*
                create peripheral manager
             */
            pInstance->pPeripheralMgr	= prmInit();
            if(pInstance->pPeripheralMgr == NULL) break;
            emuDevAddChild(&pInstance->root.device,&pInstance->pPeripheralMgr->device);
            
            /*
                Initialize the window
             */
            //initWindow(pInstance->pCoco3->pScreen);
            
            /*
                create thread mutex
             */
            // TODO: SDL won't let us name it, do we care?
            pInstance->hEmuMutex		= SDL_CreateMutex();
            if(pInstance->hEmuMutex == NULL) break;

            emuDevInit(&pInstance->root.device);
            
            result = XERROR_NONE;
            break;
        }
        
        if ( result == XERROR_NONE )
        {
            /* lock it so the thread does not run until we are ready */
            SDL_LockMutex(pInstance->hEmuMutex);
            
            /*
                start the emulation thread
             */
            pInstance->hEmuThread = SDL_CreateThread(vccEmuLoop, pcszName, pInstance);
            
            /*
                set initial state - this can be overridden by load
             */
            pInstance->CommandPending	= VCC_COMMAND_POWERON;
            pInstance->EmuThreadState	= emuThreadRunning;
        }
        else
        {
            emuDevDestroy(&pInstance->root.device);
            pInstance = NULL;
        }
	}
	
	return pInstance;
}

/********************************************************************/
