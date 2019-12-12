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
	CoCo3 machine object
*/
/*****************************************************************/

#include "coco3.h"

#include "tcc1014mmu.h"
#include "tcc1014graphics.h"
#include "tcc1014registers.h"
#include "cpuplugin.h"
#include "Vcc.h"

#include "xDebug.h"

#include "_audio.h"

#include <stdio.h>

/********************************************************************************/

#pragma mark -
#pragma mark --- Forward declarations ---

/********************************************************************************/

void cc3AudioOut(coco3_t * pCoco3);
void cc3CassOut(coco3_t * pCoco3);
void cc3CassIn(coco3_t * pCoco3);

double cc3GetCurrentCpuFrequency(coco3_t * pCoco3);
int cc3GetCurrentCpuMultiplier(coco3_t * pCoco3);

/********************************************************************************/

#pragma mark -
#pragma mark --- helpers ---

/********************************************************************************/

const char * cc3GetRamSizeAsString(coco3_t * pCoCo3)
{
    switch ( pCoCo3->run.ramSize )
    {
        case Ram128k:
            return "128k";
            break;
        case Ram512k:
            return "512k";
            break;
        case Ram1024k:
            return "1M";
            break;
        case Ram2048k:
            return "2M";
            break;
        case Ram8192k:
            return "8M";
            break;
    }
    return "Unknown";
}

/********************************************************************************/

#pragma mark -
#pragma mark --- settings helpers ---

/********************************************************************************/

void cc3SettingsReleaseStorage(coco3settings_t * settings)
{
    if ( settings->cpuPath )
    {
        free(settings->cpuPath);
        settings->cpuPath = NULL;
    }
    
    if ( settings->externalBasicROMPath )
    {
        free(settings->externalBasicROMPath);
        settings->externalBasicROMPath = NULL;
    }
}

void cc3SettingsCopy(const coco3settings_t * src, coco3settings_t * dst)
{
    if (src == NULL || dst == NULL) return;
    
    cc3SettingsReleaseStorage(dst);
    
    dst->cpuType = src->cpuType;
    dst->ramSize = src->ramSize;
    dst->cpuOverClock = src->cpuOverClock;
    dst->frameThrottle = src->frameThrottle;
    if ( src->cpuPath != NULL) dst->cpuPath = strdup(src->cpuPath);
    if ( src->externalBasicROMPath != NULL) dst->externalBasicROMPath = strdup(src->externalBasicROMPath);
}

/**
    @return 0 if the same, 1 if different
 */
int cc3SettingsCompare(const coco3settings_t * set1, const coco3settings_t * set2)
{
    if (set1 == NULL || set2 == NULL) return 0;
    
    if (   set1->cpuType == set2->cpuType
        && set1->ramSize == set2->ramSize
        && set1->cpuOverClock == set2->cpuOverClock
        && set1->frameThrottle == set2->frameThrottle
        && (   (set1->cpuPath == NULL && set2->cpuPath == NULL)
            || (   set1->cpuPath != NULL && set2->cpuPath != NULL
                && strcmp(set1->cpuPath,set2->cpuPath) == 0
                )
            )
        && (   (set1->externalBasicROMPath == NULL && set2->externalBasicROMPath == NULL)
            || (   set1->externalBasicROMPath != NULL && set2->externalBasicROMPath != NULL
                && strcmp(set1->externalBasicROMPath,set2->externalBasicROMPath) == 0
                )
            )
        )
    {
        return 0;
    }

    return 1;
}

/********************************************************************************/

#pragma mark -
#pragma mark --- emulator device callbacks ---

/********************************************************************************/

bool cc3EmuDevConfCheckDirty(emudevice_t * pEmuDevice)
{
    coco3_t *   pCoco3      = (coco3_t *)pEmuDevice;;
    
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        return (cc3SettingsCompare(&pCoco3->run,&pCoco3->conf) != 0);
    }
    
    return false;
}

/********************************************************************************/
/**
    CoCo3 machine object destruction
 */
result_t cc3EmuDevDestroy(emudevice_t * pEmuDevice)
{
	result_t	errResult   = XERROR_INVALID_PARAMETER;
	coco3_t *   pCoco3      = (coco3_t *)pEmuDevice;;
	
	ASSERT_CC3(pCoco3);
	if ( pCoco3 != NULL )
	{
		/*
			destroy child devices
		 */
		if ( pCoco3->pPak != NULL )
		{
			emuDevDestroy(&pCoco3->pPak->device);
		}
		
		emuDevDestroy(&pCoco3->pCassette->device);
        
		if (pCoco3->pCassBuffer != NULL ) 
		{
			free(pCoco3->pCassBuffer);
		}
	
		if ( pCoco3->pAudio != NULL )
		{
			emuDevDestroy(&pCoco3->pAudio->device);
		}
        
		if ( pCoco3->pAudioBuffer != NULL )
		{
			free(pCoco3->pAudioBuffer);
		}
		
		emuDevDestroy(&pCoco3->pGIME->mmu.device);
		
		emuDevDestroy(&pCoco3->pScreen->device);
		
		emuDevDestroy(&pCoco3->pMC6821->device);
		
		/*
			remove ourselves from our parent's child list
		 */
		emuDevRemoveChild(pCoco3->machine.device.pParent,&pCoco3->machine.device);
		
        cc3SettingsReleaseStorage(&pCoco3->conf);
        cc3SettingsReleaseStorage(&pCoco3->run);
        
		free(pCoco3);
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/
/**
    Save CoCo3 machine object configuration
 */
result_t cc3EmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	coco3_t *	pCoco3;
	
	pCoco3 = (coco3_t *)pEmuDevice;
	
	ASSERT_CC3(pCoco3);
	if ( pCoco3 != NULL )
	{
        cc3SettingsCopy(&pCoco3->run,&pCoco3->conf);
        
		confSetInt(config,CONF_SECTION_SYSTEM, CONF_SETTING_CPU, pCoco3->conf.cpuType);
        confSetPath(config,CONF_SECTION_SYSTEM,CONF_SETTING_CPUPATH,pCoco3->conf.cpuPath,config->absolutePaths);
        confSetInt(config,CONF_SECTION_SYSTEM, CONF_SETTING_OVERCLOCK, pCoco3->conf.cpuOverClock);
		confSetInt(config,CONF_SECTION_SYSTEM, CONF_SETTING_THROTTLE, pCoco3->conf.frameThrottle);
		confSetInt(config,CONF_SECTION_SYSTEM,CONF_SETTING_RAM,pCoco3->conf.ramSize);
		confSetPath(config,CONF_SECTION_SYSTEM,CONF_SETTING_ROMPATH,pCoco3->conf.externalBasicROMPath,config->absolutePaths);

        // TODO: save path to Pak (when moved from VCC)
		
		errResult = XERROR_NONE;
	}

	return errResult;
}

/********************************************************************************/
/**
    Load CoCo3 machine object configuration
 */
result_t cc3EmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	coco3_t *	pCoco3;
	int 		iValue;
	
    assert(config != NULL);
    
	pCoco3 = (coco3_t *)pEmuDevice;
	ASSERT_CC3(pCoco3);
	if ( pCoco3 != NULL )
	{
		if ( confGetInt(config,CONF_SECTION_SYSTEM,CONF_SETTING_CPU,&iValue) == XERROR_NONE )
		{
			pCoco3->conf.cpuType = iValue;
		}
        
        // get CPU path
        confGetPath(config,CONF_SECTION_SYSTEM,CONF_SETTING_CPUPATH,&pCoco3->conf.cpuPath,config->absolutePaths);
        

        if ( confGetInt(config,CONF_SECTION_SYSTEM,CONF_SETTING_OVERCLOCK,&iValue) == XERROR_NONE )
        {
            pCoco3->conf.cpuOverClock = iValue;
        }
		
		if ( confGetInt(config,CONF_SECTION_SYSTEM,CONF_SETTING_THROTTLE,&iValue) == XERROR_NONE )
		{
			pCoco3->conf.frameThrottle = iValue;
		}
		
		/*
            Memory
		 */
		if ( confGetInt(config,CONF_SECTION_SYSTEM,CONF_SETTING_RAM,&iValue) == XERROR_NONE )
		{
			pCoco3->conf.ramSize = iValue;
		}
		
		// external basic ROM image if desired
		confGetPath(config,CONF_SECTION_SYSTEM,CONF_SETTING_ROMPATH,&pCoco3->conf.externalBasicROMPath,config->absolutePaths);
		
        cc3SettingsCopy(&pCoco3->conf,&pCoco3->run);
        
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/*********************************************************************************/
/**
	create menu for Coco3
 */
result_t cc3EmuDevCreateMenu(emudevice_t * pEmuDev)
{
	coco3_t *		pCoco3	= (coco3_t *)pEmuDev;
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	
	ASSERT_CC3(pCoco3);
	if ( pCoco3 != NULL )
	{
		assert(pCoco3->machine.device.hMenu == NULL);
		
		// create menu
		pCoco3->machine.device.hMenu = menuCreate("System");
        
        // ROM sub-menu?
        menuAddItem(pCoco3->machine.device.hMenu,"Load external ROM",EMUDEV_UICOMMAND_CREATE(&pCoco3->machine.device,COCO3_COMMAND_ROMPATH_SET) );
        menuAddItem(pCoco3->machine.device.hMenu,"Eject external ROM",EMUDEV_UICOMMAND_CREATE(&pCoco3->machine.device,COCO3_COMMAND_ROMPATH_SET) );

        menuAddItem(pCoco3->machine.device.hMenu,"Throttle",EMUDEV_UICOMMAND_CREATE(&pCoco3->machine.device,COCO3_COMMAND_THROTTLE) );

        hmenu_t hCPUMenu = menuCreate("CPU");
        menuAddItem(hCPUMenu,"MC6809",EMUDEV_UICOMMAND_CREATE(&pCoco3->machine.device,COCO3_COMMAND_CPU + eCPUType_MC6809) );
        menuAddItem(hCPUMenu,"HD6309",EMUDEV_UICOMMAND_CREATE(&pCoco3->machine.device,COCO3_COMMAND_CPU + eCPUType_HD6309) );
        menuAddItem(hCPUMenu,"Load CPU module",EMUDEV_UICOMMAND_CREATE(&pCoco3->machine.device,COCO3_COMMAND_CPUPATH_SET) );
        menuAddItem(hCPUMenu,"Eject CPU module",EMUDEV_UICOMMAND_CREATE(&pCoco3->machine.device,COCO3_COMMAND_CPUPATH_CLEAR) );
        menuAddSubMenu(pCoco3->machine.device.hMenu, hCPUMenu);
        
        hmenu_t hRAMMenu = menuCreate("RAM");
        menuAddItem(hRAMMenu,"128k",EMUDEV_UICOMMAND_CREATE(&pCoco3->machine.device,COCO3_COMMAND_RAM + Ram128k) );
        menuAddItem(hRAMMenu,"512k",EMUDEV_UICOMMAND_CREATE(&pCoco3->machine.device,COCO3_COMMAND_RAM + Ram512k) );
        menuAddItem(hRAMMenu,"1024k",EMUDEV_UICOMMAND_CREATE(&pCoco3->machine.device,COCO3_COMMAND_RAM + Ram1024k) );
        menuAddItem(hRAMMenu,"2048k",EMUDEV_UICOMMAND_CREATE(&pCoco3->machine.device,COCO3_COMMAND_RAM + Ram2048k) );
        menuAddItem(hRAMMenu,"8192k",EMUDEV_UICOMMAND_CREATE(&pCoco3->machine.device,COCO3_COMMAND_RAM + Ram8192k) );
        menuAddSubMenu(pCoco3->machine.device.hMenu, hRAMMenu);

        char temp[64];
        hmenu_t hCPUSpeedMenu = menuCreate("Overclock");
        for (int i=1; i<OVERCLOCK_MAX; i++)
        {
            sprintf(temp,"%d X",i);
            menuAddItem(hCPUSpeedMenu,temp,EMUDEV_UICOMMAND_CREATE(&pCoco3->machine.device,COCO3_COMMAND_OVERCLOCK + i) );
        }
        menuAddSubMenu(pCoco3->machine.device.hMenu, hCPUSpeedMenu);
    }

	return errResult;
}

/*********************************************************************************/
/**
 emulator device callback to validate a device command
 */
bool cc3EmuDevValidate(emudevice_t * pEmuDev, int iCommand, int * piState)
{
	bool			bValid		= false;
	coco3_t *		pCoco3		= (coco3_t *)pEmuDev;
	//vccinstance_t *	pInstance;
	
	ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        //pInstance = (vccinstance_t *)pCoco3->machine.device.pParent;
        //ASSERT_VCC(pInstance);
        
        switch ( iCommand )
        {
            case COCO3_COMMAND_THROTTLE:
                if ( piState != NULL )
                {
                    *piState = (pCoco3->run.frameThrottle ? COMMAND_STATE_ON : COMMAND_STATE_OFF);
                }
                bValid = true;
                break;
                
            case COCO3_COMMAND_ROMPATH_SET:     // external ROM path
                bValid = true;
                break;
            case COCO3_COMMAND_ROMPATH_CLEAR:
                bValid = (pCoco3->run.externalBasicROMPath != NULL);
                break;
                
            case COCO3_COMMAND_CPUPATH_SET:     // external CPU path
                bValid = true;
                break;
            case COCO3_COMMAND_CPUPATH_CLEAR:
                bValid = (pCoco3->run.cpuPath != NULL);
                break;
                
            case COCO3_COMMAND_CPU + eCPUType_MC6809:         // built in 6809
                bValid = true;
                if ( piState != NULL )
                {
                    *piState = ((pCoco3->run.cpuType == eCPUType_MC6809) ? COMMAND_STATE_ON : COMMAND_STATE_OFF);
                }
                break;
                
            case COCO3_COMMAND_CPU + eCPUType_HD6309:     // built in 6309
                bValid = true;
                if ( piState != NULL )
                {
                    *piState = ((pCoco3->run.cpuType == eCPUType_HD6309) ? COMMAND_STATE_ON : COMMAND_STATE_OFF);
                }
                break;
                
            case COCO3_COMMAND_RAM + Ram128k:
            case COCO3_COMMAND_RAM + Ram512k:
            case COCO3_COMMAND_RAM + Ram1024k:
            case COCO3_COMMAND_RAM + Ram2048k:
            case COCO3_COMMAND_RAM + Ram8192k:
                if ( piState != NULL )
                {
                    *piState = ((iCommand-COCO3_COMMAND_RAM) == pCoco3->run.ramSize ? COMMAND_STATE_ON : COMMAND_STATE_OFF);
                }
                bValid = true;
                break;
                
            default:
                if (    iCommand >= COCO3_COMMAND_OVERCLOCK
                     && iCommand < COCO3_COMMAND_OVERCLOCK + OVERCLOCK_MAX
                    )
                {
                    if ( piState != NULL )
                    {
                        *piState = ((iCommand-COCO3_COMMAND_OVERCLOCK) == pCoco3->run.cpuOverClock ? COMMAND_STATE_ON : COMMAND_STATE_OFF);
                    }
                    bValid = true;
                    break;
                }
                
                assert(0 && "Coco3 command not recognized");
                break;
        }
    }
    
	return bValid;
}

/*********************************************************************************/
/**
	emulator device callback to perform coco3 specific command
 */
result_t cc3EmuDevCommand(emudevice_t * pEmuDev, int iCommand, int iParam)
{
	result_t		errResult	= XERROR_NONE;
	coco3_t *		pCoco3		= (coco3_t *)pEmuDev;
    bool            updateUI    = false;
    
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        vccinstance_t * pInstance = (vccinstance_t *)emuDevGetParentModuleByID(&pCoco3->machine.device,VCC_CORE_ID);
        ASSERT_VCC(pInstance);
        
        // do command
        switch ( iCommand )
        {
            case COCO3_COMMAND_THROTTLE:
                pCoco3->run.frameThrottle = ! pCoco3->run.frameThrottle;
                
                updateUI = true;
            break;
                
            case COCO3_COMMAND_ROMPATH_SET:     // external ROM path
            {
                const char * pPathname;
                
                filetype_e types[] = { COCO_PAK_ROM, COCO_FILE_NONE };
                
                // get pathname
                pPathname = sysGetPathnameFromUser(&types[0],pCoco3->run.externalBasicROMPath);
                if ( pPathname != NULL )
                {
                    if (pCoco3->run.externalBasicROMPath != NULL)
                    {
                        free(pCoco3->run.externalBasicROMPath);
                        pCoco3->run.externalBasicROMPath = NULL;
                    }
                    pCoco3->run.externalBasicROMPath = strdup(pPathname);
                    
                    // set pending power cycle
                    vccSetCommandPending(pInstance, VCC_COMMAND_POWERCYCLE);
                }
                
                updateUI = true;
            }
            break;
            case COCO3_COMMAND_ROMPATH_CLEAR:
            {
                if (pCoco3->run.externalBasicROMPath != NULL)
                {
                    free(pCoco3->run.externalBasicROMPath);
                    pCoco3->run.externalBasicROMPath = NULL;
                }
                vccSetCommandPending(pInstance, VCC_COMMAND_POWERCYCLE);
                
                updateUI = true;
            }
            break;
                
            case COCO3_COMMAND_CPUPATH_SET:     // external CPU path
            {
                const char * pPathname;
                
                filetype_e types[] = { COCO_CPU_PLUGIN, COCO_FILE_NONE };
                
                // get pathname
                pPathname = sysGetPathnameFromUser(&types[0],pCoco3->run.externalBasicROMPath);
                if ( pPathname != NULL )
                {
                    if (pCoco3->run.cpuPath != NULL)
                    {
                        free(pCoco3->run.cpuPath);
                        pCoco3->conf.cpuPath = NULL;
                    }
                    pCoco3->run.cpuPath = strdup(pPathname);
                    
                    // set pending power cycle
                    vccSetCommandPending(pInstance, VCC_COMMAND_POWERCYCLE);
                }
                
                updateUI = true;
            }
            break;
            case COCO3_COMMAND_CPUPATH_CLEAR:
            {
                if (pCoco3->run.cpuPath != NULL)
                {
                    free(pCoco3->conf.cpuPath);
                    pCoco3->conf.cpuPath = NULL;
                }
                vccSetCommandPending(pInstance, VCC_COMMAND_POWERCYCLE);
                
                updateUI = true;
            }
            break;

            case COCO3_COMMAND_CPU + eCPUType_MC6809:     // built in 6809
            case COCO3_COMMAND_CPU + eCPUType_HD6309:     // built in 6309
                pCoco3->run.cpuType = (iCommand-COCO3_COMMAND_CPU);
                vccSetCommandPending(pInstance, VCC_COMMAND_POWERCYCLE);
                
                updateUI = true;
                break;
            case COCO3_COMMAND_CPU + eCPUType_Custom:
                assert(false && "TODO: implement");
                break;

            case COCO3_COMMAND_RAM + Ram128k:
            case COCO3_COMMAND_RAM + Ram512k:
            case COCO3_COMMAND_RAM + Ram1024k:
            case COCO3_COMMAND_RAM + Ram2048k:
            case COCO3_COMMAND_RAM + Ram8192k:
                pCoco3->run.ramSize = (iCommand-COCO3_COMMAND_RAM);
                vccSetCommandPending(pInstance, VCC_COMMAND_POWERCYCLE);
                
                updateUI = true;
                break;
                
            default:
                // Set overclock
                if (    iCommand >= COCO3_COMMAND_OVERCLOCK
                    && iCommand < COCO3_COMMAND_OVERCLOCK + OVERCLOCK_MAX
                    )
                {
                    pCoco3->run.cpuOverClock = (iCommand-COCO3_COMMAND_OVERCLOCK);
                    vccSetCommandPending(pInstance, VCC_COMMAND_POWERCYCLE);
                    updateUI = true;
                    break;
                }
                
                assert(0 && "Coco3 command not recognized");
            break;
        }
        
        if ( updateUI )
        {
            vccUpdateUI(pInstance);
        }
    }
    
	return errResult;
}

/**
 */
result_t cc3EmuDevGetStatus(emudevice_t * pEmuDev, char * pszText, size_t szText)
{
    coco3_t * pCoco3 = (coco3_t *)pEmuDev;

    ASSERT_CC3(pCoco3);
    if (    pCoco3 != NULL
         && pCoco3->pGIME != NULL
         && pCoco3->machine.pCPU != NULL
         && pszText != NULL
        )
    {
        char temp[256];
        float rate = cc3GetCurrentCpuFrequency(pCoco3);
        const char * ramSize = cc3GetRamSizeAsString(pCoco3);
        
        snprintf(temp,sizeof(temp)-1,"%s-%0.2fMHz (R:%d/O:%d) %s",pCoco3->machine.pCPU->device.Name,rate,pCoco3->pGIME->CpuRate,pCoco3->run.cpuOverClock,ramSize);
        
        strncat(pszText,temp,szText-strlen(pszText));
        
        return XERROR_NONE;
    }
    
    return XERROR_INVALID_PARAMETER;
}

/**
 */
result_t cc3EmuDevEventHandler(emudevice_t * pEmuDevice, event_t * event)
{
    result_t    errResult    = XERROR_INVALID_PARAMETER;
    coco3_t * pCoco3        = (coco3_t *)pEmuDevice;
    
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        // unhandled
        errResult = XERROR_GENERIC;
        
        switch ( event->type )
        {
            case eEventDevice:
            {
                deviceevent_t * deviceEvent = (deviceevent_t *)event;
                
                // look for set CART events
                if ( deviceEvent->target == VCC_MC6821_ID )
                {
                    switch ( deviceEvent->param )
                    {
                        case 0:
                            mc6821_SetCART(pCoco3->pMC6821, deviceEvent->param2);
                            // handled
                            errResult = XERROR_NONE;
                        break;
                    }
                }
                else if ( deviceEvent->target == VCC_COCO3_ID )
                {
                    switch ( deviceEvent->param )
                    {
                        case COCO3_DEVEVENT_SCREEN_REFRESH:
                            SetupDisplay(pCoco3->pScreen);
                            // handled
                            errResult = XERROR_NONE;
                        break;
                    }
                }
            }
            break;
                    
            default:
            break;
        }
    }
    
    return errResult;
}

/********************************************************************************/

#pragma mark -
#pragma mark --- Implementation ---

/********************************************************************************/
/**
*/
coco3_t * cc3Create()
{
    result_t    result = XERROR_GENERIC;
	coco3_t *	pCoco3	= NULL;
	
	/* 
		TODO: handle allocation failures
	 */
	pCoco3 = calloc(1,sizeof(coco3_t));
	if ( pCoco3 != NULL )
	{
		pCoco3->machine.device.id		= EMU_DEVICE_ID;
        pCoco3->machine.device.idModule	= EMU_MACHINE_ID;
        pCoco3->machine.device.idDevice = VCC_COCO3_ID;
        
		strcpy(pCoco3->machine.device.Name,EMU_DEVICE_NAME_COCO3);
		
		pCoco3->machine.device.pfnDestroy		= cc3EmuDevDestroy;
		pCoco3->machine.device.pfnSave			= cc3EmuDevConfSave;
		pCoco3->machine.device.pfnLoad			= cc3EmuDevConfLoad;
		pCoco3->machine.device.pfnCreateMenu	= cc3EmuDevCreateMenu;
		pCoco3->machine.device.pfnValidate		= cc3EmuDevValidate;
		pCoco3->machine.device.pfnCommand		= cc3EmuDevCommand;
        pCoco3->machine.device.pfnGetStatus     = cc3EmuDevGetStatus;
        pCoco3->machine.device.pfnEventHandler  = cc3EmuDevEventHandler;
        pCoco3->machine.device.pfnConfCheckDirty= cc3EmuDevConfCheckDirty;
        
		/*
			config / persistence defaults
		 */	
		pCoco3->conf.cpuType			= eCPUType_MC6809;	// default: 6809
		pCoco3->conf.cpuOverClock		= 1;	            // Over clock multiplier (1-200)
		pCoco3->conf.frameThrottle		= true;	            // default: throttle enabled
		pCoco3->conf.ramSize			= Ram128k;	        // default: 512k (0 = 128k, 1=512k, 2=1024k, 3=2048k, 4=8192k)
		pCoco3->conf.externalBasicROMPath = NULL;	        // default: no external CoCo 3 ROM
		
        cc3SettingsCopy(&pCoco3->conf,&pCoco3->run);
        
        /*
         module initiallization
         */
        // dummy while
        while ( true )
        {
            // GIME
            pCoco3->pGIME = gimeCreate();
            if(pCoco3->pGIME == NULL) break;
            emuDevAddChild(&pCoco3->machine.device,&pCoco3->pGIME->mmu.device);
            pCoco3->pGIME->mmu.device.pMachine = &pCoco3->machine;
            
            // Screen
            pCoco3->pScreen = screenCreate();
            if(pCoco3->pScreen == NULL) break;
            emuDevAddChild(&pCoco3->machine.device,&pCoco3->pScreen->device);
            pCoco3->pScreen->device.pMachine = &pCoco3->machine;
            // should not be necessary
            InitDisplay(pCoco3->pScreen);

            // Cassette
            pCoco3->pCassette = casCreate();
            if(pCoco3->pCassette == NULL) break;
            emuDevAddChild(&pCoco3->machine.device,&pCoco3->pCassette->device);
            pCoco3->pCassette->device.pMachine = &pCoco3->machine;
            pCoco3->pCassBuffer = (unsigned char *)calloc(1,AUDIO_BUFFER_SIZE);
            if(pCoco3->pCassBuffer == NULL) break;

            // PIA
            pCoco3->pMC6821	= mc6821Create();
            if(pCoco3->pMC6821 == NULL) break;
            emuDevAddChild(&pCoco3->machine.device,&pCoco3->pMC6821->device);
            pCoco3->pMC6821->device.pMachine = &pCoco3->machine;

            // Keyboard
            pCoco3->pKeyboard = keyboardCreate();
            if(pCoco3->pKeyboard == NULL) break;
            emuDevAddChild(&pCoco3->machine.device,&pCoco3->pKeyboard->device);

            // Joystick
            pCoco3->pJoystick = joystickCreate();
            if(pCoco3->pJoystick == NULL) break;
            emuDevAddChild(&pCoco3->machine.device,&pCoco3->pJoystick->device);
            
            // Audio
            int TARGETFRAMERATE = 60;
            pCoco3->pAudio = audCreate(NULL,NULL,2,TARGETFRAMERATE);
            //if(pCoco3->pAudio == NULL) break;
            if ( pCoco3->pAudio != NULL )
            {
                emuDevAddChild(&pCoco3->machine.device,&pCoco3->pAudio->device);
                pCoco3->pAudio->device.pMachine = &pCoco3->machine;
                
                pCoco3->pAudioBuffer = (unsigned int *)calloc(1,AUDIO_BUFFER_SIZE);
                if(pCoco3->pAudioBuffer == NULL) break;
            
                //cc3SetAudioRate(pCoco3,22050);
                // audio callback
                pCoco3->AudioEvent = cc3AudioOut;
                pCoco3->AudioIndex  = 0;
                pCoco3->SoundRate = 0;
                pCoco3->SndEnable = 1;
                //pCoco3->SoundOutputMode = 0;    // Default to 0 = Speaker, 1 = Cassette
            }
            
            // all good
            result = XERROR_NONE;
            break;
        }
        
        if ( result != XERROR_NONE && pCoco3 != NULL )
        {
            cc3EmuDevDestroy(&pCoco3->machine.device);
            pCoco3 = NULL;
        }
        else
        {
            emuDevRegisterDevice(&pCoco3->machine.device);
        }
	}
	
	return pCoco3;
}

/********************************************************************************/
/**
 TODO: change to return a result
 */
bool cc3InstallCPU(coco3_t * pCoco3)
{
    char path[PATH_MAX];
    
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        strcpy(path,"");
        
        // copy path from config?
        if (    pCoco3->run.cpuPath != NULL
             && strlen(pCoco3->run.cpuPath) > 0
            )
        {
            strcpy(path,pCoco3->run.cpuPath);
        }
        
        // check if we should use one of the default CPUs
        if ( strlen(path) == 0 )
        {
            // get app path
            char * pAppPath = NULL;
            
            strcpy(path,"");
            
            if ( emuDevGetAppPath(&pCoco3->machine.device, &pAppPath) == XERROR_NONE )
            {
                strcpy(path,pAppPath);
                
                free(pAppPath);
                pAppPath = NULL;
            }
            
            // TODO: hard coded names
            switch ( pCoco3->run.cpuType )
            {
                case eCPUType_MC6809:
                    strcat(path,"/libvcc-cpu-mc6809");
                    break;
                    
                case eCPUType_HD6309:
                    strcat(path,"/libvcc-cpu-hd6309");
                    break;
                    
                case eCPUType_Custom:
                    assert(false && "TODO: implement");
                    break;
            }
        }

        // TODO: only do this if it is different?
        /* destroy existing CPU */
        if ( pCoco3->machine.pCPU != NULL )
        {
            /* TODO: check if it is different before destruction */

            emuDevDestroy(&pCoco3->machine.pCPU->device);
            pCoco3->machine.pCPU = NULL;
        }
        
        /*
            load plug-in and create CPU
         */
        pCoco3->machine.pCPU = cpuLoad(path);
        
        // TODO: handle error loading CPU module
        if (pCoco3->machine.pCPU != NULL)
        {
            /* add CPU as a child device to the machine */
            emuDevAddChild(&pCoco3->machine.device,&pCoco3->machine.pCPU->device);
            pCoco3->machine.pCPU->device.pMachine = &pCoco3->machine;
            
            return TRUE;
        }
    }
    
    return FALSE;
}

/********************************************************************************/
/**
	Coco 3 hard reset / power cycle
*/
bool cc3PowerCycle(coco3_t * pCoco3)
{
    bool success = false;
    
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        // dummy while
        while ( true)
        {
            // handle CPU change
            cc3InstallCPU(pCoco3);
            if(pCoco3->machine.pCPU == NULL) break;

            // initialize CPU
            cpuInit(pCoco3->machine.pCPU);
            
            /*
                set CPU mem read/write functions
             */
            pCoco3->machine.pCPU->mmu = &pCoco3->pGIME->mmu;

            /* initialize GIME - this resets port read/write registration */
            mmuInit(pCoco3->pGIME,pCoco3->run.ramSize,pCoco3->run.externalBasicROMPath);
            
            /*
                move to mmuInit or mc6821?
             */
            /* register port read/write callbacks - this must be done after mmuInit */
            portRegisterRead(pCoco3->pGIME,mc6821_pia0Read,&pCoco3->pMC6821->device,0xFF00,0xFF03);			// MC6821 P.I.A Keyboard access $FF00-$FF03
            portRegisterWrite(pCoco3->pGIME,mc6821_pia0Write,&pCoco3->pMC6821->device,0xFF00,0xFF03);		// MC6821 P.I.A Keyboard access $FF00-$FF03
            portRegisterRead(pCoco3->pGIME,mc6821_pia1Read,&pCoco3->pMC6821->device,0xFF20,0xFF23);         // MC6821 P.I.A    Sound and VDG Control
            portRegisterWrite(pCoco3->pGIME,mc6821_pia1Write,&pCoco3->pMC6821->device,0xFF20,0xFF23);		// MC6821 P.I.A	Sound and VDG Control

            // reset PIA
            mc6821_Reset(pCoco3->pMC6821);
            
            // Captures interal rom pointer for CPU Interrupt Vectors
            mc6883_Reset(pCoco3->pGIME);
            
            // reset GIME
            gimeReset(pCoco3->pGIME);

            audResetAudio(pCoco3->pAudio);
            cc3SetAudioRate(pCoco3,pCoco3->SoundRate);
            
            /*
                reset pak
            */
            /*
                register our default pak port read/write functions
             
                TODO: The pak should do this itself
             */
            if ( pCoco3->pPak != NULL )
            {
                portRegisterDefaultRead(pCoco3->pGIME,pakPortRead,&pCoco3->pPak->device);
                portRegisterDefaultWrite(pCoco3->pGIME,pakPortWrite,&pCoco3->pPak->device);

                pakReset(pCoco3->pPak);
            }
            else
            {
                portRegisterDefaultRead(pCoco3->pGIME,NULL,NULL);
                portRegisterDefaultWrite(pCoco3->pGIME,NULL,NULL);
                
                mc6821_SetCART(pCoco3->pMC6821,High);
            }

            // Zero all CPU Registers and sets the PC to VRESET
            cpuReset(pCoco3->machine.pCPU);
            
            success = true;
            break;
        }
    }
    
    return success;
}

/********************************************************************************/
/**
 */
bool cc3Reset(coco3_t * pCoco3)
{
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        mc6883_Reset(pCoco3->pGIME);
        mc6821_Reset(pCoco3->pMC6821);

        gimeReset(pCoco3->pGIME);
        mmuReset(pCoco3->pGIME);

        pCoco3->CycleDrift               = 0;
        pCoco3->AudioIndex               = 0;
        
        //audResetAudio(pCoco3->pAudio);
        
        SetupDisplay(pCoco3->pScreen);

        pakReset(pCoco3->pPak);
        
        cpuReset(pCoco3->machine.pCPU);
        
        return true;
    }
    
    return false;
}

/********************************************************************************/

#pragma mark -
#pragma mark --- Configuration ---

/*********************************************************************************/

/**
    Get the current CPU rate multiplier - single/double speed * overclock multiplier
 */
int cc3GetCurrentCpuMultiplier(coco3_t * pCoco3)
{
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        return ((1+pCoco3->pGIME->CpuRate)*pCoco3->run.cpuOverClock);
    }
    
    return 1;
}

/**
    Get the current CPU rate in cycles per second
 */
double cc3GetCurrentCpuFrequency(coco3_t * pCoco3)
{
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        return CPU_DEFAULT_MHZ * cc3GetCurrentCpuMultiplier(pCoco3);
    }
    
    return CPU_DEFAULT_MHZ;
}

/**
    Get config overclocking rate
 
    Range: 1 - N where N could be any number up to the point the host CPU is running full speed
    Setting to a higher number will result in the frame rate dropping.
 */
int cc3ConfGetOverClock(coco3_t * pCoco3)
{
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        return pCoco3->run.cpuOverClock;
    }
    
    return 1;
}

/**
    Set config overclocking rate
 */
void cc3ConfSetOverClock(coco3_t * pCoco3, int multiplier)
{
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        pCoco3->run.cpuOverClock = multiplier;
    }
}

/********************************************************************************/
/**
 */
unsigned short cc3SetAudioRate(coco3_t * pCoco3, unsigned short Rate)
{
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        pCoco3->SndEnable=1;
        pCoco3->CycleDrift=0;
        pCoco3->AudioIndex=0;

        if ( Rate == 0 )
        {
            pCoco3->SndEnable=0;
        }

        pCoco3->SoundRate = Rate;
    }
    
	return (0);
}

/********************************************************************************/
/**
 @param Mode 0 = Speaker, 1 = Cassette Out, 2 = Cassette In
 */
unsigned char cc3SetSndOutMode(coco3_t * pCoco3, unsigned char Mode)  
{
	unsigned char LastMode=0;
	//unsigned short PrimarySoundRate;
	
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        //PrimarySoundRate = pCoco3->SoundRate;
        LastMode = pCoco3->SoundOutputMode;
        
        switch ( Mode )
        {
            case 0:
                if ( LastMode == 1 )	// Send the last bits to be encoded
                {
                    casFlushCassetteBuffer(pCoco3->pCassette,pCoco3->pCassBuffer,pCoco3->AudioIndex);
                }
                
                pCoco3->AudioEvent = cc3AudioOut;
                cc3SetAudioRate(pCoco3,pCoco3->SoundRate);
                break;
                
            case 1:
                pCoco3->AudioEvent = cc3CassOut;
                //PrimarySoundRate = pCoco3->SoundRate;
                cc3SetAudioRate(pCoco3,TAPEAUDIORATE);
                break;
                
            case 2:
                pCoco3->AudioEvent = cc3CassIn;
                //PrimarySoundRate = pCoco3->SoundRate;
                cc3SetAudioRate(pCoco3,TAPEAUDIORATE);
                break;
                
            default:	// QUERY
                return (pCoco3->SoundOutputMode);
                break;
        }
        
        if ( Mode != LastMode )
        {
            //		if (LastMode==1)
            //			FlushCassetteBuffer(CassBuffer,AudioIndex);	//get the last few bytes
            
            pCoco3->AudioIndex = 0;	// Reset Buffer on true mode switch
            //LastMode = Mode;
        }
        pCoco3->SoundOutputMode = Mode;
        
        return (pCoco3->SoundOutputMode);
    }
    
    return 0;
}

/********************************************************************************/
/********************************************************************************/

#pragma mark -
#pragma mark --- audio out callback functions ---

/********************************************************************************/
/**
	Audio event callback - get audio out sample (byte)
 */
void cc3AudioOut(coco3_t * pCoco3)
{
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        pCoco3->pAudioBuffer[pCoco3->AudioIndex++] = mc6821_GetDACSample(pCoco3->pMC6821);
        
        assert(pCoco3->AudioIndex < AUDIO_BUFFER_SIZE);
    }
}

/********************************************************************************/
/**
	Audio event callback - get cassette out sample (byte)
 */
void cc3CassOut(coco3_t * pCoco3)
{
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        pCoco3->pCassBuffer[pCoco3->AudioIndex++] = mc6821_GetCasSample(pCoco3->pMC6821);
        
        assert(pCoco3->AudioIndex < AUDIO_BUFFER_SIZE);
    }
}

/********************************************************************************/
/**
	Audio event callback - get cassette in sample (byte)
 */
void cc3CassIn(coco3_t * pCoco3)
{
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        pCoco3->pAudioBuffer[pCoco3->AudioIndex] = mc6821_GetDACSample(pCoco3->pMC6821);
        
        mc6821_SetCasSample(pCoco3->pMC6821,pCoco3->pCassBuffer[pCoco3->AudioIndex++]);
        
        assert(pCoco3->AudioIndex < AUDIO_BUFFER_SIZE);
    }
}

/********************************************************************************/

#pragma mark -
#pragma mark --- Screen rendering / CPU execution ---

/********************************************************************************/

void cc3UpdateGIMETimerCounter(coco3_t * pCoco3)
{
    // timer interrupt enabled?
    if ( pCoco3->pGIME->MasterTickCounter > 0 )
    {
        pCoco3->pGIME->MasterTickCounter--;
        if ( pCoco3->pGIME->MasterTickCounter == 0 )
        {
            if ( pCoco3->pGIME->TimerInterruptEnabled )
            {
                gimeAssertTimerInterrupt(pCoco3->pGIME,pCoco3->machine.pCPU);
            }

            // reset timer
            pCoco3->pGIME->MasterTickCounter = pCoco3->pGIME->UnxlatedTickCounter;
            
            // NOTE: some docs say that the state of the IRQ/FIRQ enable is reset as well
        }
    }
}

/********************************************************************************/
/**
	'render' one horizontal line on-screen
    really, emulate CPU execution for the duration of one raster line
 
    TODO: all device servicing should happen here - add calc for next event (sample audio, serial sample, serial IRQ, etc)
*/
int cc3ExecuteCPUForOneLine(coco3_t * pCoco3, int line)
{
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        // since we are running all NTSC lines only fire horizontal interrupts every other line
        if ( line%2==0 )
        {
            // check for CART interrupt
            mc6821_IRQ_CART(pCoco3->pMC6821, pCoco3->machine.pCPU);
            
            // h-sync 63.5 uS
            mc6821_IRQ_HS(pCoco3->pMC6821,pCoco3->machine.pCPU,ANY);
            
            // HBORD: Horiz border FIRQ interrupt generated on falling edge of HSYNC.
            // GIME horizontal interrupt at beginning of line (if enabled)
            // TODO: calc actual time to HBORD and dispatch then (leading edge of actual display, right after the border is drawn)
            gimeAssertHorzInterrupt(pCoco3->pGIME,pCoco3->machine.pCPU);
        }
        
        // service the pak if there and wanted
        if (    pCoco3->pPak != NULL
             && line%2==0   // every other NTSC line
            )
        {
            ASSERT_COCOPAK(pCoco3->pPak);
            
            if ( pCoco3->pPak->pakapi.pfnPakHeartBeat != NULL )
            {
                (*pCoco3->pPak->pakapi.pfnPakHeartBeat)(pCoco3->pPak);
            }
        }
        
        // Update GIME timer?
        if (    pCoco3->pGIME->TimerInterruptEnabled
            && pCoco3->pGIME->TimerClockRate == 0 // (PicosPerLine)
            )
        {
            cc3UpdateGIMETimerCounter(pCoco3);
        }

        // set number of picoseconds to next sound sample
        // picoseconds between sound samples
        //double PicosToSoundSample = PICOSECOND/AudioRate;
        
        // pGIME->TimerClockRate
        // 0 = 63.695uS  (1/60*262)  1 scanline time
        // 1 = 279.265nS (1/ColorBurst)
        double Rate[2] = {PicosPerLine,PicosPerColorBurst};
        int overRateAndClockMultiplier = cc3GetCurrentCpuMultiplier(pCoco3);
        double picosToCyclesMultiplier = (CyclesPerLine * overRateAndClockMultiplier)/PicosPerLine;
        /*
            execute number of CPU cycles for one horizontal line
         
            TODO: for PopStar to work correctly, the timer needed to be decremented accurately \
            Invetigate if there is a more efficient way to accomplish this (executing more cpu cycles per call to cpuExec)
         */
        double picosThisLine = PicosPerLine;
        while ( picosThisLine > 1 )
        {
            //
            // calculate the number of picoseconds to execute the CPU for
            // This will be up to the next (potential) interrupt
            //
            double picosThisStep = Rate[1]; // PicosPerColorBurst

            // update GIME timer?
            if (   pCoco3->pGIME->TimerInterruptEnabled
                && pCoco3->pGIME->TimerClockRate == 1 // (PicosPerColorBurst)
                )
            {
                cc3UpdateGIMETimerCounter(pCoco3);
            }
            
            double cyclesThisStep = pCoco3->CycleDrift + (picosThisStep * picosToCyclesMultiplier);
            if ( cyclesThisStep >= 1 )
            {
                int executeCycles = (int)floor(cyclesThisStep);
                pCoco3->CycleDrift = cpuExec(pCoco3->machine.pCPU,executeCycles) + (cyclesThisStep - executeCycles);
            }
            else
            {
                pCoco3->CycleDrift = cyclesThisStep;
            }

            picosThisLine -= picosThisStep;
            
#if 0   // audio sampling
            if ( audioSample )
            {
                PicosToSoundSample -= picosThisStep;
                if (    pCoco3->SoundInterrupt  // enabled?
                     && PicosToSoundSample == 0 // time for next one?
                    )
                {
                    if ( pCoco3->AudioEvent != NULL )
                    {
                        (*pCoco3->AudioEvent)(pCoco3);
                    }
                    
                    PicosToSoundSample = PicosToSoundSample;
                }
            }
#endif
        }
    }
    
	return (0);
}

/********************************************************************************/
/**
	Simulate the render of one Coco3 video frame
 
    Everything happens here
 
    Executes CPU at speed set by config
 */
// TODO: All screen parameters (border size, lines to render) should be latched at the start of the frame
// and passed into screenUpdate so the video mode changing will not affect the frame render in adverse ways
void cc3RenderFrame(coco3_t * pCoco3)
{
	screen_t *		pScreen;
    
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        assert(pCoco3->pScreen != NULL);
        
        pScreen = pCoco3->pScreen;

        // TODO: restore blink state
        //SetBlinkState(pCoco3->pGIME,pCoco3->pGIME->BlinkPhase);
        
        if ( screenLock(pScreen) )
        {
            // stop now - TODO: this is fatal
            return;
        }
       
        int visibleLineCount            = VISIBLE_LINESPERSCREEN;
        int topBorderUnrenderedLines    = 4*2;                    // unrendered lines in top border to fit vertically into our 640x480 'screen'
        int activeDisplayStartLine      = 28;
        int topBorderHeight             = pScreen->TopBorder*2;
        int firstRenderedLine           = activeDisplayStartLine + topBorderUnrenderedLines;
        int screenStartLine             = firstRenderedLine + topBorderHeight;
        int screenLineCount             = pScreen->LinesperScreen*2;
        
        // vertical interrupts
        int vsyncStart              = 0;
        int vsyncEnd                = vsyncStart + 3;   // vertical sync pulse lasts for 3 lines
        int vInterrupt              = screenStartLine + screenLineCount; // VBORD

        // stats / sanity check
        int borderLine = 0;
        int renderedVisibleLines    = 0;
        int renderedScreenLines    = 0;
        int unrenderedBorderLines    = 0;
        int vsyncLines             = 0;
        int vblankLines             = 0;
        int topBorderRendered       = 0;
        int bottomBorderRendered = 0;
        
        // go through all lines for one frame (including v-blank)
        // the screen is 2X the height of what we actually draw
        for (int y=0; y<TOTAL_LINESPERSCREEN; y++)
        {
            //
            // emulation
            //
            
            if ( y == vsyncEnd )
            {
                // 60HZ Vertical sync pulse 16.667 mS
                // FS goes Low to High
                mc6821_IRQ_FS(pCoco3->pMC6821,pCoco3->machine.pCPU,LowToHigh);
            }

            if ( y == vsyncStart )
            {
                // 60HZ Vertical sync pulse 16.667 mS
                // FS goes High to Low
                mc6821_IRQ_FS(pCoco3->pMC6821,pCoco3->machine.pCPU,HighToLow);
            }
            
            // VBORD vertical interrupt
            if ( y == vInterrupt )
            {
                // Vertical interrupt (if enabled)
                gimeAssertVertInterrupt(pCoco3->pGIME,pCoco3->machine.pCPU);
            }
            
            // execute CPU equivalent of one NTSC raster line
            cc3ExecuteCPUForOneLine(pCoco3,y);

            //
            // Screen update
            //
            
            if ( y < firstRenderedLine )
            {
                if ( y < activeDisplayStartLine )
                {
                    vsyncLines++;
                }
                else
                {
                    unrenderedBorderLines++;
                }
                
                // skip inital unrendered lines
                continue;
            }
            if ( y >= firstRenderedLine + visibleLineCount )
            {
                // skip v-blank lines
                vblankLines++;
                continue;
            }

            // top border
            if ( y < screenStartLine )
            {
                renderedVisibleLines++;
                topBorderRendered++;
                
                // disabled - now drawn below
                //borderLine = y-firstRenderedLine;
                //screenUpdateTopBorder(pScreen,borderLine);
            }
            
            // active display
            if (    y >= screenStartLine
                 && y < screenStartLine + screenLineCount
                )
            {
                renderedScreenLines++;
                renderedVisibleLines++;
                
                // only update on even lines
                if ( y%2==0 )
                {
                    int lineToRender = (y - screenStartLine) / 2;
                    
                    if ( lineToRender >= 0 && lineToRender < pScreen->LinesperScreen )
                    {
                        // update the screen from the video for this line
                        // this will update two lines on the display
                        // it includes the left and right borders
                        screenUpdate(pCoco3->pScreen,lineToRender);
                    }
                    else
                    {
                        // note, this happens during the end of the frame during a video mode switch
                        // from a larger number of lines to a smaller one
                        // It would be addressed by latching all of the screen parameters (pScreen->LinesperScreen, pScreen->TopBorder, etc)
                        // at the beginning of the frame and passing to screenUpdate so parameters changing during the frame render will
                        // not screw things up
                        //char temp[64];
                        //sprintf(temp,"Visible video line to render is out of range : %d (Max:%d)",lineToRender,pScreen->LinesperScreen);
                        //emuDevLog(&pScreen->device, "%s", temp);
                    }
                }
            }
            
            // first visible line?
            if ( y == screenStartLine )
            {
                // draw all of top border at once
                for (int i=firstRenderedLine; i<screenStartLine; i++)
                {
                    borderLine = i-firstRenderedLine;
                    screenUpdateTopBorder(pScreen,borderLine);
                }
            }

            // bottom border
            if (    y >= screenStartLine + screenLineCount
                 && y < firstRenderedLine + visibleLineCount
                )
            {
                renderedVisibleLines++;
                bottomBorderRendered++;
                
                borderLine = y-firstRenderedLine;

                screenUpdateBottomBorder(pScreen,borderLine);
            }
        }
        
        assert(renderedVisibleLines == VISIBLE_LINESPERSCREEN);
        assert(renderedVisibleLines == renderedScreenLines + bottomBorderRendered + topBorderRendered);
        assert(renderedVisibleLines + unrenderedBorderLines + vsyncLines + vblankLines == TOTAL_LINESPERSCREEN);
        
        /*
         done updating the off-screen buffer
         */
        /* unlock it */
        screenUnlock(pScreen);
        
        /*
            output audio for frame
         */
        switch ( pCoco3->SoundOutputMode )
        {
            case 0:
                audFlushAudioBuffer(pCoco3->pAudio,pCoco3->pAudioBuffer,pCoco3->AudioIndex);
                break;
            case 1:
                casFlushCassetteBuffer(pCoco3->pCassette,pCoco3->pCassBuffer,pCoco3->AudioIndex);
                break;
            case 2:
                casLoadCassetteBuffer(pCoco3->pCassette,pCoco3->pCassBuffer);
                break;
        }
        
        pCoco3->AudioIndex = 0;
    }
}

/********************************************************************************/
/**
	'dim' the screen - for when it is paused
 
	modifies the same pixels each time so the screen buffer does not need to be 
	updated each time
 */
void cc3CopyGrayFrame(coco3_t * pCoco3)
{
	screen_t *		pScreen;
	int			    x;
	int			    y;
	unsigned char	Temp;
	
    ASSERT_CC3(pCoco3);
    if ( pCoco3 != NULL )
    {
        pScreen = pCoco3->pScreen;
        
        /*
            make it 'dimmed'
         */
        screenLock(pScreen);
        
        assert(pScreen->surface.bitDepth == 32);

        Temp = 0;
        for (y=0; y<pScreen->WindowSizeY; y++)
        {
            uint32_t * line = (uint32_t *)surfaceGetLinePtr(&pScreen->surface, y);
            
            for (x=0; x<pScreen->WindowSizeX; x++)
            {
                if ( ((x+y) % 4) != 0 )
                {
                    line[x] = Temp | (Temp<<8) | (Temp <<16);
                }
            }
        }
        
        screenUnlock(pScreen);
    }
}

/********************************************************************************/



