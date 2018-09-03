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
	Peripheral manager
 
	maintain a list of peripherals
	each peripheral will state what ports it requires
	peripheral container will manage what devices are allowed based on the ports used
*/
/*********************************************************************************/

#include "peripherals.h"

#include "Vcc.h"

/********************************************************************************/

#pragma mark -
#pragma mark --- emulator device callbacks ---

/********************************************************************************/
/**
 */
result_t prmEmuDevDestroy(emudevice_t * pEmuDevice)
{
	result_t				errResult = XERROR_INVALID_PARAMETER;
	peripheralmanager_t *	pPeripherals;
	
	pPeripherals = (peripheralmanager_t *)pEmuDevice;
	
	ASSERT_PERIPHERALMGR(pPeripherals);
	
	if ( pPeripherals != NULL )
	{
		/*
		 destroy child devices
		 */
		//assert(0);
		
		/*
		 remove ourselves from our parent's child list
		 */
		emuDevRemoveChild(pPeripherals->device.pParent,&pPeripherals->device);
		
		free(pPeripherals);
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/
/**
 */
result_t prmEmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
	result_t				errResult	= XERROR_INVALID_PARAMETER;
	peripheralmanager_t *	pPeripherals;
	
	pPeripherals = (peripheralmanager_t *)pEmuDevice;
	
	ASSERT_PERIPHERALMGR(pPeripherals);
	
	if ( pPeripherals != NULL )
	{
		//assert(0);
		//confSetInt(config,CONF_SECTION_SYSTEM, CONF_SETTING_CPU, pCoco3->CpuType);
		//confSetPath(config,CONF_SECTION_SYSTEM,CONF_SETTING_ROM,pCoco3->pExternalBasicROMPath);
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/
/**
 */
result_t prmEmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
	result_t				errResult	= XERROR_INVALID_PARAMETER;
	peripheralmanager_t *	pPeripherals;
	
	pPeripherals = (peripheralmanager_t *)pEmuDevice;
	
	ASSERT_PERIPHERALMGR(pPeripherals);
	
	if ( pPeripherals != NULL )
	{
		//assert(0);
		
		//if ( confGetInt(config,CONF_SECTION_SYSTEM,CONF_SETTING_CPU,&iValue) == XERROR_NONE )
		//{
		//	pCoco3->CpuType		= iValue;
		//}
		
		// external basic ROM image if desired
		//confGetPath(config,CONF_SECTION_SYSTEM,CONF_SETTING_ROM,&pCoco3->pExternalBasicROMPath,config->absolutePaths);
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/*********************************************************************************/
/**
 create menu for device
 */
result_t prmEmuDevCreateMenu(emudevice_t * pEmuDevice)
{
	result_t				errResult	= XERROR_INVALID_PARAMETER;
	peripheralmanager_t *	pPeripherals;
	
	pPeripherals = (peripheralmanager_t *)pEmuDevice;
	
	ASSERT_PERIPHERALMGR(pPeripherals);
	
	if ( pPeripherals != NULL )
	{
		assert(pPeripherals->device.hMenu == NULL);
		
		// create menu
		pPeripherals->device.hMenu = menuCreate("Peripherals");
		
		menuAddItem(pPeripherals->device.hMenu,"Add",	(pPeripherals->device.iCommandID<<16) | PERIPHERAL_COMMAND_ADD);
	}
	
	return errResult;
}

/*********************************************************************************/
/**
 emulator device callback to validate a device command
 */
bool prmEmuDevValidate(emudevice_t * pEmuDevice, int iCommand, int * piState)
{
	bool					bValid		= FALSE;
	//peripheralmanager_t *	pPeripherals		= (peripheralmanager_t *)pEmuDevice;
	//vccinstance_t *		pInstance;
	
	//ASSERT_PERIPHERALMGR(pPeripherals);
	
	//pInstance = (vccinstance_t *)pCoco3->device.pParent;
	//ASSERT_VCC(pInstance);
	
	switch ( iCommand )
	{
		case PERIPHERAL_COMMAND_ADD:
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
result_t prmEmuDevCommand(emudevice_t * pEmuDev, int iCommand, int iParam)
{
	result_t				errResult	= XERROR_NONE;
	//peripheralmanager_t *	pPeripherals;
	//vccinstance_t *		pInstance;
	
	//ASSERT_PERIPHERALMGR(pPeripherals);
	
	//pInstance = (vccinstance_t *)pPeripherals;
	//ASSERT_VCC(pInstance);
	
	// do command
	switch ( iCommand )
	{
		case PERIPHERAL_COMMAND_ADD:
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
peripheralmanager_t * prmInit()
{
	peripheralmanager_t *	pPeripherals	= NULL;
	
	/* 
	 TODO: handle allocation failures
	 */
	pPeripherals = calloc(1,sizeof(peripheralmanager_t));
	if ( pPeripherals != NULL )
	{
		pPeripherals->device.id			= EMU_DEVICE_ID;
		pPeripherals->device.idModule	= VCC_PERIPHERALMGR_ID;
		strcpy(pPeripherals->device.Name,EMU_DEVICE_NAME_PERIPHERALMGR);
		
		pPeripherals->device.pfnDestroy		= prmEmuDevDestroy;
		pPeripherals->device.pfnSave		= prmEmuDevConfSave;
		pPeripherals->device.pfnLoad		= prmEmuDevConfLoad;
		pPeripherals->device.pfnCreateMenu	= prmEmuDevCreateMenu;
		pPeripherals->device.pfnValidate	= prmEmuDevValidate;
		pPeripherals->device.pfnCommand		= prmEmuDevCommand;
		
		emuDevRegisterDevice(&pPeripherals->device);
	}
	
	return pPeripherals;
}

/*********************************************************************************/

