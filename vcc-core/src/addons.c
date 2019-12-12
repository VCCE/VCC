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

/*********************************************************************************/
/*
	Add-on manager (unimplemented)
 
	maintain a list of add-ons
	each add-on will state which i/o ports it uses
	add-on container will manage what devices are allowed based on the i/o ports used
*/
/*********************************************************************************/

#include "addons.h"

#include "Vcc.h"

/********************************************************************************/

#pragma mark -
#pragma mark --- emulator device callbacks ---

/********************************************************************************/
/**
 */
result_t aomEmuDevDestroy(emudevice_t * pEmuDevice)
{
	result_t			errResult = XERROR_INVALID_PARAMETER;
	addonmanager_t *	pAddOns;
	
	pAddOns = (addonmanager_t *)pEmuDevice;
	
	ASSERT_ADDONMGR(pAddOns);
	
	if ( pAddOns != NULL )
	{
		/*
			destroy child devices
		 */
		//assert(0);
		
		/*
		 remove ourselves from our parent's child list
		 */
		emuDevRemoveChild(pAddOns->device.pParent,&pAddOns->device);
		
		free(pAddOns);
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/
/**
 */
result_t aomEmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	addonmanager_t *	pAddOns;
	
	pAddOns = (addonmanager_t *)pEmuDevice;
	
	ASSERT_ADDONMGR(pAddOns);
	
	if ( pAddOns != NULL )
	{
		//assert(0);
		//confSetInt(hConf,CONF_SECTION_SYSTEM, CONF_SETTING_CPU, pCoco3->CpuType);
		//confSetPath(hConf,CONF_SECTION_SYSTEM,CONF_SETTING_ROM,pCoco3->pExternalBasicROMPath);
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/
/**
 */
result_t aomEmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
	result_t			errResult	= XERROR_INVALID_PARAMETER;
	addonmanager_t *	pAddOns;
	
	pAddOns = (addonmanager_t *)pEmuDevice;
	
	ASSERT_ADDONMGR(pAddOns);
	
	if ( pAddOns != NULL )
	{
		//assert(0);
		
		//if ( confGetInt(hConf,CONF_SECTION_SYSTEM,CONF_SETTING_CPU,&iValue) == XERROR_NONE )
		//{
		//	pCoco3->CpuType		= iValue;
		//}
		
		// external basic ROM image if desired
		//confGetPath(hConf,CONF_SECTION_SYSTEM,CONF_SETTING_ROM,&pCoco3->pExternalBasicROMPath);
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/*********************************************************************************/
/**
	create menu for device
 */
result_t aomEmuDevCreateMenu(emudevice_t * pEmuDevice)
{
	result_t			errResult	= XERROR_INVALID_PARAMETER;
	addonmanager_t *	pAddOns;
	
	pAddOns = (addonmanager_t *)pEmuDevice;
	
	ASSERT_ADDONMGR(pAddOns);
	
	if ( pAddOns != NULL )
	{
		assert(pAddOns->device.hMenu == NULL);
		
		// create menu
		pAddOns->device.hMenu = menuCreate("Add-on 'Mod' devices");
		
		menuAddItem(pAddOns->device.hMenu,"Add",	(pAddOns->device.iCommandID<<16) | ADDONMGR_COMMAND_ADD);
	}
	
	return errResult;
}

/*********************************************************************************/
/**
	emulator device callback to validate a device command
 */
bool aomEmuDevValidate(emudevice_t * pEmuDevice, int iCommand, int * piState)
{
	bool				bValid		= FALSE;
	//vccinstance_t *		pInstance;
	//addonmanager_t *	pAddOns		= (addonmanager_t *)pEmuDevice;
	
	//ASSERT_ADDONMGR(pAddOns);
	
	//pInstance = (vccinstance_t *)pCoco3->device.pParent;
	//ASSERT_VCC(pInstance);
	
	switch ( iCommand )
	{
		case ADDONMGR_COMMAND_ADD:
			bValid = FALSE;
		break;
			
		default:
			assert(0 && "Add-On Manager command not recognized");
	}
	
	return bValid;
}

/*********************************************************************************/
/**
 emulator device callback to perform coco3 specific command
 */
result_t aomEmuDevCommand(emudevice_t * pEmuDev, int iCommand, int iParam)
{
	result_t			errResult	= XERROR_NONE;
	//addonmanager_t *	pAddOns;
	//vccinstance_t *		pInstance;
	
	//ASSERT_ADDONMGR(pAddOns);
	
	//pInstance = (vccinstance_t *)pAddOns;
	//ASSERT_VCC(pInstance);
	
	// do command
	switch ( iCommand )
	{
		case ADDONMGR_COMMAND_ADD:
			assert(0);
		break;
			
		default:
			assert(0 && "Add-On Manager command not recognized");
			break;
	}
	
	return errResult;
}

/*********************************************************************************/
/**
 */
addonmanager_t * aomInit()
{
	addonmanager_t *	pAddOns	= NULL;
	
	/* 
		TODO: handle allocation failures
	 */
	pAddOns = calloc(1,sizeof(addonmanager_t));
	if ( pAddOns != NULL )
	{
		pAddOns->device.id			= EMU_DEVICE_ID;
		pAddOns->device.idModule	= VCC_ADDONMGR_ID;
		strcpy(pAddOns->device.Name,EMU_DEVICE_NAME_ADDONMGR);
		
		pAddOns->device.pfnDestroy		= aomEmuDevDestroy;
		pAddOns->device.pfnSave			= aomEmuDevConfSave;
		pAddOns->device.pfnLoad			= aomEmuDevConfLoad;
		pAddOns->device.pfnCreateMenu	= aomEmuDevCreateMenu;
		pAddOns->device.pfnValidate		= aomEmuDevValidate;
		pAddOns->device.pfnCommand		= aomEmuDevCommand;
		
		emuDevRegisterDevice(&pAddOns->device);
	}
	
	return pAddOns;
}

/*********************************************************************************/

