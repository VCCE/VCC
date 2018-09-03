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

/****************************************************************************/
/*
	pakinterface.c

    CoCo Cartridge module
 
	TODO: add version reporting - version of plug-in, version of vcc built for
    TODO: add support for detecting banked ROM or not so port write will do nothing if it is the disk ROM :-)
 */
/****************************************************************************/

#include "pakinterface.h"

#include "tcc1014mmu.h"
#include "mc6821.h"
#include "coco3.h"
#include "file.h"
#include "Vcc.h"

#include "xDebug.h"

/****************************************************************************/

#pragma mark -
#pragma mark --- Implementation ---

/****************************************************************************/
/**
 */
void pakDestroyROM(cocopak_t * pPak)
{
	if ( pPak != NULL )
	{
        ASSERT_COCOPAK(pPak);
        
		romDestroy(&pPak->rom);
		strcpy(pPak->device.Name,"Blank");
		free(pPak->pPathname);
		pPak->pPathname = NULL;
	}
}

/****************************************************************************/
/**
	Query whether module has a cofiguration UI
 */
bool pakCanConfig(cocopak_t * pPak, int iMenuItem)
{
	ASSERT_COCOPAK(pPak);
	
	return (pPak->pakapi.pfnPakConfig != NULL);
}

/****************************************************************************/
/**
	Call module's configuration UI
 */
void pakConfig(cocopak_t * pPak, int iMenuItem)
{
	ASSERT_COCOPAK(pPak);
	
	if ( pPak->pakapi.pfnPakConfig != NULL )
	{
		(*pPak->pakapi.pfnPakConfig)(pPak,iMenuItem);
	}	
}

/****************************************************************************/
/**
    Do we care about this anymore?
 */
void pakGetConfigDescription(cocopak_t * pPak, char * String)
{
	ASSERT_COCOPAK(pPak);

	//sprintf(Temp,"Configure %s",pPak->Modname);
	
	strcat(String,"Module Name: ");
	strcat(String,pPak->device.Name);
	strcat(String,"\n");
	
	if ( pPak->pakapi.pfnPakConfig != NULL )
	{
		pPak->ModuleParams |= HASCONFIG;
		strcat(String,"Has Configurable options\n");
	}
	if ( pPak->pakapi.pfnPakPortWrite != NULL )
	{
		pPak->ModuleParams |= HASIOWRITE;
		strcat(String,"Is IO writable\n");
	}
	if ( pPak->pakapi.pfnPakPortRead != NULL )
	{
		pPak->ModuleParams |= HASIOREAD;
		strcat(String,"Is IO readable\n");
	}
	
    // TODO: fix/restore
	/*
	if ( pPak->pakAPIFunctions.pfnPakSetInterruptCallPointer != NULL )
	{
		pPak->ModuleParams |= NEEDSCPUIRQ;
		strcat(String,"Generates Interrupts\n");
	}
	*/
	
    // TODO: fix/restore
	/*
	if ( pPak->pakAPIFunctions.pfnPakDmaMemPointer != NULL )
	{
		pPak->ModuleParams |= DOESDMA;
		strcat(String,"Generates DMA Requests\n");
	}
	 */
	
	if ( pPak->pakapi.pfnPakHeartBeat != NULL )
	{
		pPak->ModuleParams |= NEEDHEARTBEAT;
		strcat(String,"Needs Heartbeat\n");
	}
	if ( pPak->pakapi.pfnPakAudioSample != NULL )
	{
		pPak->ModuleParams |= ANALOGAUDIO;
		strcat(String,"Analog Audio Outputs\n");
	}
	if ( pPak->pakapi.pfnPakMemWrite8 != NULL )
	{
		pPak->ModuleParams |= CSWRITE;
		strcat(String,"Needs ChipSelect Write\n");
	}
	if ( pPak->pakapi.pfnPakMemRead8 != NULL )
	{
		pPak->ModuleParams |= CSREAD;
		strcat(String,"Needs ChipSelect Read\n");
	}
	if ( pPak->device.pfnGetStatus != NULL )
	{
		pPak->ModuleParams |= RETURNSSTATUS;
		strcat(String,"Returns Status\n");
	}
	if ( pPak->pakapi.pfnPakReset != NULL )
	{
		pPak->ModuleParams |= CARTRESET;
		strcat(String,"Needs Reset Notification\n");
	}
}

/****************************************************************************/
/**
	Reset pak if one is loaded
 */
void pakReset(cocopak_t * pPak)
{
	if ( pPak != NULL )
	{
		ASSERT_COCOPAK(pPak);
	
        // Reset catridge ROM bank
		pPak->rom.BankedCartOffset = 0;

        if ( pPak->pakapi.pfnPakReset != NULL )
		{
			(*pPak->pakapi.pfnPakReset)(pPak);
		}
        
        // send CART signal to the 6821
        // Sends up the device tree to the first handler
        // This allows the MPI to intercept it before it gets to the
        // CoCo depending on what slot is active
        int cart = (pPak->bCartWanted&pPak->bCartEnabled) ? Low : High;
        deviceevent_t deviceEvent;
        deviceEvent.event.type = eEventDevice;
        deviceEvent.source = &pPak->device;
        deviceEvent.target = VCC_MC6821_ID;
        deviceEvent.param = 0;
        deviceEvent.param2 = cart;
        emuDevSendEventUp(&pPak->device, &deviceEvent.event);
	}
}

/****************************************************************************/
/**
 */
unsigned short pakAudioSample(cocopak_t * pPak)
{
	if ( pPak != NULL )
	{
		ASSERT_COCOPAK(pPak);
	
		if ( pPak->pakapi.pfnPakAudioSample != NULL )
		{
			return (*pPak->pakapi.pfnPakAudioSample)(pPak);
		}
	}
	
	return 0;
}

/****************************************************************************/

#pragma mark -
#pragma mark --- pak port and memory read/write CPU plug-in callbacks ---

/****************************************************************************/
/**
	default Pak port read
 */
unsigned char pakPortRead(emudevice_t * pEmuDevice, unsigned char port)
{
	cocopak_t *	pPak = (cocopak_t *)pEmuDevice;
	
	if ( pPak != NULL )
	{
		ASSERT_COCOPAK(pPak);
	
		/*
			if Pak has an explicit port read handler, call it
		*/
		if ( pPak->pakapi.pfnPakPortRead != NULL )
		{
			return (* pPak->pakapi.pfnPakPortRead)(pEmuDevice,port);
		}
	}
	
	return 0;
}

/****************************************************************************/
/**
	default Pak port write
 */
void pakPortWrite(emudevice_t * pEmuDevice, unsigned char Port, unsigned char Data)
{
	cocopak_t *	pPak = (cocopak_t *)pEmuDevice;
	
	if ( pPak != NULL )
	{
		ASSERT_COCOPAK(pPak);
	
		/*
			if Pak has an explicit port write handler, call it
		 */
		if (  pPak->pakapi.pfnPakPortWrite != NULL )
		{
			(* pPak->pakapi.pfnPakPortWrite)(pEmuDevice,Port,Data);
			return;
		}

		/*
			otherwise we assume it is to set the ROM pak bank
		 */
		if (   (Port == 0x40) 
			 & (pPak->rom.bLoaded == 1)
		   )
		{
			// Set ROM pak offset
			pPak->rom.BankedCartOffset = (Data & 15) << 14;
		}
	}
}

/****************************************************************************/
/**
	default Pak memory read
 
	currently explicitly called from mmuMemRead8
 */
unsigned char pakMem8Read(emudevice_t * pEmuDevice, unsigned short Address)
{
	cocopak_t *	pPak = (cocopak_t *)pEmuDevice;
	
	if ( pPak != NULL )
	{
		ASSERT_COCOPAK(pPak);
	
		/*
			if Pak has its own memory read handler, call it
		 */
		if (  pPak->pakapi.pfnPakMemRead8 != NULL )
		{
			return (* pPak->pakapi.pfnPakMemRead8)(pEmuDevice,Address & 32767);
		}
		
		/*
			otherwise we assume we should read from the Pak ROM, if there is one
		 */
		if (  pPak->rom.pData != NULL )
		{
			// TODO: size mask should match ROM size?
			return pPak->rom.pData[(Address & (pPak->rom.szData-1)) + pPak->rom.BankedCartOffset];
		}
	}
	
	return 0;
}

/****************************************************************************/
/**
	default Pak memory write
 
	currently explicitly called from mmuMemWrite8
 */
void pakMem8Write(emudevice_t * pEmuDevice, unsigned char Port,unsigned char Data)
{
#if (defined _DEBUG)
	cocopak_t *	pPak = (cocopak_t *)pEmuDevice;
	
	ASSERT_COCOPAK(pPak);
	
	// ROM! - nothing to do
#endif
}

/****************************************************************************/

#pragma mark -
#pragma mark --- ROM pack specific functions ---

/****************************************************************************/
/**
	Load external ROM
 
	@return Size of ROM loaded, 0 if load failed
*/
size_t pakLoadExtROM(cocopak_t * pPak, const char * pPathname)
{
	size_t			index		= 0;

	ASSERT_COCOPAK(pPak);
	if ( pPak != NULL )
	{
		pakDestroyROM(pPak);
		
		if ( pPathname != NULL )
		{
			/*
			 load the new ROM
			 */
			if ( romLoad(&pPak->rom,pPathname) == XERROR_NONE )
			{
				// ???
				index = pPak->rom.szData;
			
				pPak->pPathname = strdup(pPathname);
			
				pPak->device.idDevice	= VCC_COCOPAK_ROM_ID;
			}
		}
	}
	
	return (index);
}

/****************************************************************************/

#pragma mark -
#pragma mark --- ROM pack load/unload ---

/****************************************************************************/
/**
	Load a PAK.  This is either a ROM pack (.rom or .bin file) or it
	is a plug-in module via a dynamic library (see pakinterface.h).
 
	@param pPathname Path to the file to load
	@param ppPak 
 
	@return LOADPAK_NOMODULE		Nothing loaded
			LOADPAK_SUCCESS			DLL or ROM loaded
			LOADPAK_NOTVCC			Not a VCC plug-in DLL
 
	If *ppPak is NULL on return the load failed
 
	This is expected to be called from the UI, not the Emu thread
	and the Emu thread should be locked prior to calling
 
	caller needs to set pointers, reset, assert CART etc.
*/
int pakLoadPAK(const char * pPathname, cocopak_t ** ppPak)
{
	pxdynlib_t		hModule;
	
	assert(ppPak != NULL);
	assert(*ppPak == NULL && "destruction of previous cocopak_t must be done by caller");

	if ( pPathname != NULL )
	{
		int 		iType	= 0;
		result_t	errResult;
        
		iType = sysGetFileType(pPathname);
		
		switch ( iType )
		{
			case COCO_FILE_NONE:		// File doesn't exist
			{
				return LOADPAK_NOMODULE;
			}
			break;
				
			case COCO_PAK_ROM:			// File is a ROM image
			{
				(*ppPak) = pakCreate();
                assert(*ppPak != NULL);// TODO: return error on failure
				
				// load external ROM
				pakLoadExtROM((*ppPak),pPathname);
				
				(*ppPak)->bCartWanted	= TRUE;
				(*ppPak)->bCartEnabled	= TRUE;

				(*ppPak)->type			= COCO_PAK_ROM;

				strcpy((*ppPak)->device.Name,"CoCo ROM Pak");

				emuDevRegisterDevice(&(*ppPak)->device);

				return LOADPAK_SUCCESS;
			}
			break;
				
			case COCO_PAK_PLUGIN:		// dynamic library, potential plug-in
			{
				pakcreatefn_t	pfnPakCreate;
				
				// attempt to load new DLL
				errResult = xDynLibLoad(pPathname,&hModule);
				if (    errResult != XERROR_NONE
					 || hModule == NULL 
					)
				{
					// unable to load dynamic library
					return LOADPAK_NOMODULE;
				}

				// check for creation function
				*ppPak = NULL;
				errResult = xDynLibGetSymbolAddress(hModule, VCCPLUGIN_FUNCTION_MODULECREATE, (void **)&pfnPakCreate);
				if (    errResult == XERROR_NONE
					&& pfnPakCreate != NULL 
					)
				{
					// create custom PAK
					(*ppPak) = (*pfnPakCreate)();
				}

                // check if creation failed
                if ( (*ppPak) == NULL )
                {
                    // unload the module
                    xDynLibUnload(&hModule);
                    hModule = NULL;
                    
                    // not a plug-in module
                    return LOADPAK_NOTVCC;
                }

                emuDevRegisterDevice(&(*ppPak)->device);
                
                // set generic name for device in case it does not set it itself
                if ( strlen((*ppPak)->device.Name) == 0 )
                {
                    strcpy((*ppPak)->device.Name,"CoCo Device Pak");
                }
                
                /*
                    Initialize
                 */
                (*ppPak)->hinstLib = hModule;
                
                (*ppPak)->type = COCO_PAK_PLUGIN;
                
                // save path
                (*ppPak)->pPathname = strdup(pPathname);
                
                // Note: device will set its own CART status
                
                // module loaded
                return LOADPAK_SUCCESS;
			}
			break;
		}
	}
	
    // unknown type - not a plug-in or the pathname is bad
	return LOADPAK_NOMODULE;
}

/****************************************************************************/
/**
 */
void pakUnloadPack(cocopak_t * pPak)
{
	coco3_t *		pCoco3;
	vccinstance_t *	pInstance;
	
	ASSERT_COCOPAK(pPak);
	
	pCoco3 = (coco3_t *)pPak->device.pParent;
	assert(pCoco3 != NULL);
	
	pInstance = (vccinstance_t *)emuDevGetParentModuleByID(&pCoco3->machine.device,VCC_CORE_ID);
	assert(pInstance != NULL);
	
	/* clear function pointers */
	memset(&pPak->pakapi,0,sizeof(pPak->pakapi));

	if ( pPak->hinstLib != NULL )
	{
		xDynLibUnload(&pPak->hinstLib);
		pPak->hinstLib=NULL;
	}

	pPak->bCartWanted	= FALSE;
	pPak->bCartEnabled	= FALSE;

	pakDestroyROM(pPak);
}

/****************************************************************************/

#pragma mark -
#pragma mark --- emulator device callbacks ---

/****************************************************************************/
/**
 Destroy pak
 */
result_t pakEmuDevDestroy(emudevice_t * pEmuDevice)
{
    result_t		errResult	= XERROR_INVALID_PARAMETER;
	cocopak_t *		pPak;
	
    pPak = (cocopak_t *)pEmuDevice;
	if ( pPak != NULL )
	{
		ASSERT_COCOPAK(pPak);
		
		pakUnloadPack(pPak);
		
		// remove ourselves from the device list
		emuDevRemoveChild(pPak->device.pParent,&pPak->device);
		
		free(pPak);
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/****************************************************************************/
/**
 */
result_t pakEmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	cocopak_t *		pPak;
	
	pPak = (cocopak_t *)pEmuDevice;
	
	ASSERT_COCOPAK(pPak);
	
	if ( pPak != NULL )
	{
		// do nothing for now
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/****************************************************************************/
/**
 */
result_t pakEmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	cocopak_t *		pPak;
	
	pPak = (cocopak_t *)pEmuDevice;
	
	ASSERT_COCOPAK(pPak);
	
	if ( pPak != NULL )
	{
		// do nothing for now
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/****************************************************************************/

#pragma mark -
#pragma mark --- Initialization / Termination ---

/****************************************************************************/
/**
	create generic pak
 
	Called when module does not need to create an extension of
	cocopak_t or when the loaded PAK is a ROM.
 */
cocopak_t * pakCreate()
{
	cocopak_t *	pPak;
	
	pPak = calloc(1,sizeof(cocopak_t));
	if ( pPak != NULL )
	{
		pPak->device.id			= EMU_DEVICE_ID;
		pPak->device.idModule	= VCC_COCOPAK_ID;
		
		pPak->device.pfnDestroy	= pakEmuDevDestroy;
		pPak->device.pfnSave	= pakEmuDevConfSave;
		pPak->device.pfnLoad	= pakEmuDevConfLoad;
		
		// Note: menu is handled in Coco3 object
		
		strcpy(pPak->device.Name,"CoCo ROM Pak/Cart");
		
		emuDevRegisterDevice(&pPak->device);
	}
	
	return pPak;
}

/****************************************************************************/
