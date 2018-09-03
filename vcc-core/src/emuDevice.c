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

/***********************************************************************************/
/*
	emuDevice.c
 */
/***********************************************************************************/

#include "emuDevice.h"

#include "xDebug.h"

/***********************************************************************************/

int g_iNextCommandID	= 1;

result_t emuDevRegisterDevice(emudevice_t * pEmuDevice)
{
	assert(pEmuDevice != NULL);
	
	/* must fit in a signed 16bit value */
	assert(g_iNextCommandID < 32768);
	pEmuDevice->iCommandID = g_iNextCommandID++;
	
	return XERROR_NONE;
}

/***********************************************************************************/

emudevice_t * emuDevGetParentModuleByID(emudevice_t * pEmuDevice, id_t idModule)
{
	emudevice_t *	pEmuDevCur;
	
	pEmuDevCur = pEmuDevice;
	while ( pEmuDevCur != NULL )
	{
		if ( pEmuDevCur->idModule == idModule )
		{
			// found it
			break;
		}
		
		pEmuDevCur = pEmuDevCur->pParent;
	}
	
	return pEmuDevCur;
}

/***********************************************************************************/

result_t emuDevAddChild(emudevice_t * pEmuDevParent, emudevice_t * pEmuDevChild)
{
	result_t		errResult = XERROR_INVALID_PARAMETER;
	emudevice_t *	pEmuDevCur;
	
	assert(pEmuDevParent != NULL);
	assert(pEmuDevChild != NULL);
	
	if (    pEmuDevParent != NULL 
		 && pEmuDevChild != NULL
		)
	{
		// add child to parent device
		if ( pEmuDevParent->pChild == NULL )
		{
			pEmuDevParent->pChild = pEmuDevChild;
		}
		else 
		{
			pEmuDevCur = pEmuDevParent->pChild;
			while ( pEmuDevCur->pSibling != NULL )
			{
				pEmuDevCur = pEmuDevCur->pSibling;
			}
			pEmuDevCur->pSibling = pEmuDevChild;
		}
		
		// point to parent device
		pEmuDevChild->pParent = pEmuDevParent;
	}
	
	return errResult;
}

/***********************************************************************************/
/**
	Remove child device from its parent
 
	It is assumed that the parent is known to the caller so we do not have to descend 
	further into the tree than the parent's children list
 */
result_t emuDevRemoveChild(emudevice_t * pEmuDevParent, emudevice_t * pEmuDevChild)
{
	result_t		errResult = XERROR_INVALID_PARAMETER;
	emudevice_t *	pEmuDevCur;
	
	if (    pEmuDevParent != NULL 
		&& pEmuDevChild != NULL
		)
	{
		/* we only handle a direct child, not a descendant */
		assert(pEmuDevParent == pEmuDevChild->pParent);

		/* if the given device is our direct child */
		if ( pEmuDevParent->pChild == pEmuDevChild )
		{
			pEmuDevParent->pChild = pEmuDevChild->pSibling;
			if ( pEmuDevParent->pChild != NULL )
			{
				pEmuDevParent->pChild->pParent = pEmuDevParent;
			}
			pEmuDevChild->pSibling	= NULL;
			pEmuDevChild->pParent	= NULL;
			errResult = XERROR_NONE;
		}
		else 
		{
			/* search our child list for the given device */
			pEmuDevCur = pEmuDevParent->pChild;
			while (    pEmuDevCur != NULL
				    && pEmuDevCur->pSibling != NULL 
				  ) 
			{
				if ( pEmuDevCur->pSibling == pEmuDevChild )
				{
					// unhook from the list
					pEmuDevCur->pSibling = pEmuDevChild->pSibling;
					
					// clear links
					pEmuDevChild->pSibling	= NULL;
					pEmuDevChild->pParent	= NULL;
					assert(pEmuDevChild->pChild == NULL);
					
					errResult = XERROR_NONE;
					break;
				}
				
				// next sibling
				pEmuDevCur = pEmuDevCur->pSibling;
			}
		}

	}

	return errResult;
}

/***********************************************************************************/
/**
    Do one-time initilization of the device tree
 */
XAPI result_t emuDevInit(emudevice_t * pEmuDevice)
{
    result_t errResult = XERROR_INVALID_PARAMETER;
    
    assert(pEmuDevice != NULL);
    
    if ( pEmuDevice != NULL )
    {
        assert(pEmuDevice->id == EMU_DEVICE_ID);
        
        if ( pEmuDevice->pfnInit != NULL )
        {
            errResult = (*pEmuDevice->pfnInit)(pEmuDevice);
        }
        
        if ( pEmuDevice->pSibling != NULL )
        {
            emuDevInit(pEmuDevice->pSibling);
        }
        
        if ( pEmuDevice->pChild != NULL )
        {
            emuDevInit(pEmuDevice->pChild);
        }
    }
    
    return errResult;
}

/***********************************************************************************/
/**
    Reset device tree
 */
XAPI result_t emuDevReset(emudevice_t * pEmuDevice)
{
    result_t errResult = XERROR_INVALID_PARAMETER;
    
    assert(pEmuDevice != NULL);
    
    if ( pEmuDevice != NULL )
    {
        assert(pEmuDevice->id == EMU_DEVICE_ID);
        
        if ( pEmuDevice->pfnReset != NULL )
        {
            errResult = (*pEmuDevice->pfnReset)(pEmuDevice);
        }
        
        if ( pEmuDevice->pSibling != NULL )
        {
            emuDevReset(pEmuDevice->pSibling);
        }
        
        if ( pEmuDevice->pChild != NULL )
        {
            emuDevReset(pEmuDevice->pChild);
        }
    }
    
    return errResult;
}

/***********************************************************************************/
/**
*/
XAPI result_t emuDevDestroy(emudevice_t * pEmuDevice)
{
	result_t		errResult = XERROR_INVALID_PARAMETER;

	assert(pEmuDevice != NULL);

	if ( pEmuDevice != NULL )
	{
		assert(pEmuDevice->id == EMU_DEVICE_ID);
	
		/* 
            destroy function is expected to unlink itself, and free its own memory 
        */
        if ( pEmuDevice->pfnDestroy != NULL )
        {
            errResult = (*pEmuDevice->pfnDestroy)(pEmuDevice);
        }
        else
        {
            /*
                Disconnect us from our parent
            */
            emuDevRemoveChild(pEmuDevice->pParent,pEmuDevice);
            
            free(pEmuDevice);
            
            errResult = XERROR_NONE;
        }
	}
	
	return errResult;
}

#if 0
/***********************************************************************************/
/**
	Walk device tree and destroy each object by calling its destroy function
 
	Descends to children first, then to siblings
 
	This function is recursive so the last device in the single linked list is 
	destroyed first.  Generally speaking devices will be destroyed in the reverse
	order they were added to the device tree
 */
result_t emuDevDestroyTree(emudevice_t * pEmuDevice)
{
	result_t		errResult = XERROR_INVALID_PARAMETER;

	assert(pEmuDevice != NULL);
	assert(pEmuDevice->id == EMU_DEVICE_ID);
	
	/*
		destroy child device first
	 */
	if ( pEmuDevice->pChild != NULL )
	{
		errResult = emuDevDestroy(pEmuDevice->pChild);
		
		// needs to be run-time check rather than just debug!
		assert(errResult == XERROR_NONE);
		
		pEmuDevice->pChild = NULL;
	}
	
	/*
		destroy sibling device next
	 */
	if ( pEmuDevice->pSibling )
	{
		errResult = emuDevDestroy(pEmuDevice->pSibling);
		
		// needs to be run-time check rather than just debug!
		assert(errResult == XERROR_NONE);
		
		pEmuDevice->pSibling = NULL;
	}
	
	/*
		now destroy ourselves
	 */
	assert(pEmuDevice->pfnDestroy != NULL && "Emulator device has no destroy function!");
	
	errResult = (*pEmuDevice->pfnDestroy)(pEmuDevice);
	
	return errResult;
}
#endif

/***********************************************************************************/
/**
 */
XAPI result_t emuDevSendCommand(emudevice_t * pEmuDevice, int command, int state)
{
    result_t result = XERROR_GENERIC;
    
    assert(pEmuDevice != NULL);
    assert(pEmuDevice->id == EMU_DEVICE_ID);

    if ( pEmuDevice->pfnCommand != NULL )
    {
        result = (*pEmuDevice->pfnCommand)(pEmuDevice,command,state);
    }
    
    return result;
}

/***********************************************************************************/
/**
*/
XAPI result_t emuDevGetCommandState(emudevice_t * pEmuDevice, int command, int * pstate)
{
    result_t result = XERROR_GENERIC;

    assert(pEmuDevice != NULL);
    assert(pEmuDevice->id == EMU_DEVICE_ID);

    if ( pEmuDevice->pfnValidate != NULL )
    {
        if ( (*pEmuDevice->pfnValidate)(pEmuDevice,command,pstate) )
        {
            result = XERROR_NONE;
        }
    }
    
    return result;
}

/***********************************************************************************/
/**
 */
result_t emuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
	result_t		errResult = XERROR_INVALID_PARAMETER;
	
	assert(pEmuDevice != NULL);
	assert(pEmuDevice->id == EMU_DEVICE_ID);

	assert(pEmuDevice->pfnSave != NULL && "Emulator device has no save function!");
	
	errResult = (*pEmuDevice->pfnSave)(pEmuDevice,config);

	if ( pEmuDevice->pChild != NULL )
	{
		errResult = emuDevConfSave(pEmuDevice->pChild,config);
		
		// needs to be run-time check rather than just debug!
		assert(errResult == XERROR_NONE);
	}

	if ( pEmuDevice->pSibling )
	{
		errResult = emuDevConfSave(pEmuDevice->pSibling,config);
		
		// needs to be run-time check rather than just debug!
		assert(errResult == XERROR_NONE);
	}
	
	return errResult;
}

/***********************************************************************************/
/**
 */
result_t emuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
	result_t		errResult = XERROR_INVALID_PARAMETER;
	
	assert(pEmuDevice != NULL);
	assert(pEmuDevice->id == EMU_DEVICE_ID);

    // do self first
    if ( pEmuDevice->pfnLoad != NULL )
    {
        errResult = (*pEmuDevice->pfnLoad)(pEmuDevice,config);
    }
    
	if ( pEmuDevice->pChild != NULL )
	{
		errResult = emuDevConfLoad(pEmuDevice->pChild,config);
		
		// needs to be run-time check rather than just debug!
		assert(errResult == XERROR_NONE);
	}
    
	if ( pEmuDevice->pSibling )
	{
		errResult = emuDevConfLoad(pEmuDevice->pSibling,config);
		
		// needs to be run-time check rather than just debug!
		assert(errResult == XERROR_NONE);
	}
	
	return errResult;
}

/***********************************************************************************/
/**
    Enumerate all objects in a device tree
 */
result_t emuDevEnumerate(emudevice_t * pEmuDevice, emudevenumcbfn_t pfnEnumCB, void * pUser)
{
	result_t		errResult = XERROR_INVALID_PARAMETER;
	
	assert(pEmuDevice != NULL);
	assert(pEmuDevice->id == EMU_DEVICE_ID);
	assert(pfnEnumCB != NULL);
	
	/* fire callback for ourselves */
	errResult = (*pfnEnumCB)(pEmuDevice,pUser);
	
	if ( pEmuDevice->pChild != NULL )
	{
		/* descend into each child */
		errResult = emuDevEnumerate(pEmuDevice->pChild,pfnEnumCB,pUser);
		
		// needs to be run-time check rather than just debug!
		assert(errResult == XERROR_NONE);
	}
	if ( pEmuDevice->pSibling )
	{
		/* descend into each sibling */
		errResult = emuDevEnumerate(pEmuDevice->pSibling,pfnEnumCB,pUser);
		
		// needs to be run-time check rather than just debug!
		assert(errResult == XERROR_NONE);
	}
	
	return errResult;
}

/***********************************************************************************/
/**
	Find a device by command id
 */
emudevice_t * emuDevFindByCommandID(emudevice_t * pEmuDevice, int iCommandID)
{
	emudevice_t *	pFoundDevice = NULL;
	
	assert(pEmuDevice != NULL);
	assert(pEmuDevice->id == EMU_DEVICE_ID);
	
	if ( pEmuDevice->iCommandID == iCommandID )
	{
		pFoundDevice = pEmuDevice;
	}
	
	if (    pFoundDevice == NULL
		 && pEmuDevice->pChild != NULL 
	   )
	{
		/* descend into each child */
		pFoundDevice = emuDevFindByCommandID(pEmuDevice->pChild,iCommandID);
	}
	if (    pFoundDevice == NULL
		 && pEmuDevice->pSibling 
		)
	{
		/* descend into each sibling */
		pFoundDevice = emuDevFindByCommandID(pEmuDevice->pSibling,iCommandID);
	}
	
	return pFoundDevice;
}

/**
 Find a device by device/module id
 */
XAPI emudevice_t * emuDevFindByDeviceID(emudevice_t * pEmuDevice, id_t idDevice)
{
    emudevice_t *    pFoundDevice = NULL;
    
    assert(pEmuDevice != NULL);
    assert(pEmuDevice->id == EMU_DEVICE_ID);
    
    if ( pEmuDevice->idDevice == idDevice )
    {
        pFoundDevice = pEmuDevice;
    }
    
    if (    pFoundDevice == NULL
        && pEmuDevice->pChild != NULL
        )
    {
        /* descend into each child */
        pFoundDevice = emuDevFindByDeviceID(pEmuDevice->pChild,idDevice);
    }
    if (    pFoundDevice == NULL
        && pEmuDevice->pSibling
        )
    {
        /* descend into each sibling */
        pFoundDevice = emuDevFindByDeviceID(pEmuDevice->pSibling,idDevice);
    }
    
    return pFoundDevice;
}

/**
 Find a device by device/module id
 */
XAPI emudevice_t * emuDevFindByModuleID(emudevice_t * pEmuDevice, id_t idModule)
{
    emudevice_t *    pFoundDevice = NULL;
    
    assert(pEmuDevice != NULL);
    assert(pEmuDevice->id == EMU_DEVICE_ID);
    
    if ( pEmuDevice->idModule == idModule )
    {
        pFoundDevice = pEmuDevice;
    }
    
    if (    pFoundDevice == NULL
        && pEmuDevice->pChild != NULL
        )
    {
        /* descend into each child */
        pFoundDevice = emuDevFindByModuleID(pEmuDevice->pChild,idModule);
    }
    if (    pFoundDevice == NULL
        && pEmuDevice->pSibling
        )
    {
        /* descend into each sibling */
        pFoundDevice = emuDevFindByModuleID(pEmuDevice->pSibling,idModule);
    }
    
    return pFoundDevice;
}

/**
    Send an event up the device tree
 */
XAPI result_t emuDevSendEventUp(emudevice_t * pEmuDevice, event_t * event)
{
    result_t    result = XERROR_INVALID_PARAMETER;
    
    if ( pEmuDevice != NULL )
    {
        // default to unhandled
        result = XERROR_GENERIC;
        
        emudevice_t * curDevice = pEmuDevice->pParent;
        while ( curDevice != NULL )
        {
            if ( curDevice->pfnEventHandler != NULL )
            {
                result = (*curDevice->pfnEventHandler)(curDevice,event);
                if ( result == XERROR_NONE )
                {
                    break;
                }
            }
            
            curDevice = curDevice->pParent;
        }
        
        if ( result != XERROR_NONE )
        {
            emuDevLog(pEmuDevice,"Unhandled event");
        }
    }
    
    return result;
}

/***********************************************************************************/

XAPI emurootdevice_t * emuDevGetRootDevice(emudevice_t * pEmuDevice)
{
    emurootdevice_t * pRootDevice = NULL;
    
    if (pEmuDevice != NULL)
    {
        // is root device cached?
        if (pEmuDevice->pRoot == NULL)
        {
            // find and cache it
            emudevice_t * pTemp = pEmuDevice;
            while ( pTemp->pParent != NULL )
            {
                pTemp = pTemp->pParent;
            }
            
            assert(pTemp->pRoot != NULL);
            
            pEmuDevice->pRoot = pTemp->pRoot;
        }
        
        pRootDevice = pEmuDevice->pRoot;
    }
    
    return pRootDevice;
}

/**
    Get the menu handle from the parent for the given device.
*/
XAPI hmenu_t emuDevGetMenuFromParent(emudevice_t * pEmuDevice)
{
    assert(pEmuDevice != NULL);
    assert(pEmuDevice->pParent != NULL);
    assert(pEmuDevice->pParent->hMenu != NULL);

    if ( pEmuDevice != NULL )
    {
        if ( pEmuDevice->pParent != NULL )
        {
            // the parent has a specific menu to hook into
            // there should be only one
            // This menu will only be set available certain types of devices
            // such as the CoCo cartridge slot or the MPI.
            // TODO: set name of menu to child name?
            return pEmuDevice->pParent->hChildHookMenu;
        }
        
        // does the parent already have a menu for us?
        hmenu_t hMenu = menuFind(pEmuDevice->pParent->hMenu, pEmuDevice->Name);
        if ( hMenu != NULL )
        {
            return hMenu;
        }
        
        // no menu
        // create one
        hMenu = menuCreate(pEmuDevice->Name);
        menuAddSubMenu(pEmuDevice->pParent->hMenu, hMenu);
        return hMenu;
    }
    
    return NULL;
}

/**
 */
XAPI result_t emuDevGetDocumentPath(emudevice_t * pEmuDevice, char ** ppPath)
{
    result_t result = XERROR_GENERIC;
    
    emurootdevice_t * pRoot = emuDevGetRootDevice(pEmuDevice);
    
    if ( pRoot && pRoot->pfnGetDocumentPath )
    {
        result = pRoot->pfnGetDocumentPath(&pRoot->device,ppPath);
    }
    
    return result;
}

/**
 */
XAPI result_t emuDevGetAppPath(emudevice_t * pEmuDevice, char ** ppPath)
{
    result_t result = XERROR_GENERIC;
    
    emurootdevice_t * pRoot = emuDevGetRootDevice(pEmuDevice);
    
    if ( pRoot && pRoot->pfnGetAppPath )
    {
        result = pRoot->pfnGetAppPath(&pRoot->device,ppPath);
    }
    
    return result;
}

/**
 */
XAPI result_t emuDevLog(emudevice_t * pEmuDevice, const char * pMessage)
{
    result_t result = XERROR_GENERIC;
    
    emurootdevice_t * pRoot = emuDevGetRootDevice(pEmuDevice);
    
    if ( pRoot != NULL && pRoot->pfnLog != NULL && pMessage != NULL)
    {
        result = pRoot->pfnLog(&pRoot->device,pMessage);
    }
    
    return result;
}


/**
 */
XAPI result_t emuRootDevAddListener(emurootdevice_t * pRootDevice, emudevice_t * pEmuDevice, event_e type)
{
    result_t result = XERROR_GENERIC;
    
    if ( pRootDevice == NULL )
    {
        pRootDevice = emuDevGetRootDevice(pEmuDevice);
    }
    
    if ( pRootDevice != NULL )
    {
        eventlistener_t * pEventListener = calloc(1,sizeof(eventlistener_t));
        if ( pEventListener != NULL )
        {
            pEventListener->pDevice = pEmuDevice;
            pEventListener->type = type;
        
            slinklistAddToHead(&pRootDevice->llistEventListeners[type], &pEventListener->link);
        
            result = XERROR_NONE;
        }
    }
    
    return result;
}

/**
 */
XAPI result_t emuRootDevDispatchEvent(emurootdevice_t * pRootDevice, event_t * event)
{
    result_t result = XERROR_NONE;
    
    if ( pRootDevice != NULL )
    {
        eventlistener_t * pEventListener = (eventlistener_t *)pRootDevice->llistEventListeners[event->type].head;
        while ( pEventListener != NULL )
        {
            if ( pEventListener->pDevice != NULL
                 && pEventListener->pDevice->pfnEventHandler != NULL
                )
            {
                (*pEventListener->pDevice->pfnEventHandler)(pEventListener->pDevice,event);
            }
            
            pEventListener = (eventlistener_t *)pEventListener->link.next;
        }
    }
    
    return result;
}

/***********************************************************************************/
