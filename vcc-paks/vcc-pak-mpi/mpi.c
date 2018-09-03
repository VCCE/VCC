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

/*************************************************************/
/*
	MPI - VCC Emulator plug-in

	This is an expansion module for the Vcc Emulator. It 
	simulates the functions of the TRS-80 Multi-Pak Interface
*/
/*************************************************************/
/**
    TODO: enforce limit on number of slots to 16
	TODO: Test support for more than 4 MPI slots
 
	TODO: add method to catch requests for port registration for improved dispatch performance
			currently we capture anything in the range and then dispatch to everything
 
    TODO: review - CTS & SCS and address range of paks
 
    TODO: CART needs to go through MPI - not direct to MC6821/GIME - current implementation ok?
    TODO: interrupts should only go through MPI? (except NMI)
*/
/*************************************************************/

#include "mpi.h"

#include "pakinterface.h"
#include "coco3.h"
#include "tcc1014mmu.h"
#include "mc6821.h"
#include "path.h"
#include "Vcc.h"

#include <stdio.h>
#include <assert.h>
#include <limits.h>

/********************************************************************************/

#pragma mark -
#pragma mark --- private functions ---

/********************************************************************************/
/**
 */
result_t _mpiReallocateSlotArray(mpi_t * pMPI)
{
    if (    pMPI->slots != NULL
         && pMPI->confNumSlots != pMPI->numSlots
        )
    {
        assert(false && "if MPI slots are already allocated, resize and move pak ptrs over");
    }
    
    pMPI->slots = calloc(1,sizeof(cocopak_t *) * pMPI->numSlots);
    assert(pMPI->slots != NULL);
    
    return XERROR_NONE;
}

/**
 */
result_t _mpiLoadPak(mpi_t * pMPI, const char * path, int slot)
{
    assert(slot >= 0 && slot < pMPI->numSlots);
    
    //vccLoadPak(vccinstance_t *pInstance, const char *pPathname);
    // pMPI->slots[slot] = ;
    
    assert(false && "TODO: implement");
}

/********************************************************************************/

#pragma mark -
#pragma mark --- emulator device callbacks ---

/*********************************************************************/
/**
    MPI interface instance destruction
 */
result_t mpiEmuDevDestroy(emudevice_t * pEmuDevice)
{
    mpi_t *    pMPI = (mpi_t *)pEmuDevice;
    
    ASSERT_MPI(pMPI);
    if ( pMPI != NULL )
    {
        assert(false && "remove all paks");
        //emuDevRemoveChild(&pMPI->pak.device,&pMPI->wd1793.device);
        
        /* detach ourselves from our parent */
        emuDevRemoveChild(pMPI->pak.device.pParent,&pMPI->pak.device);
        
        free(pEmuDevice);
        
        return XERROR_NONE;
    }
    
    return XERROR_INVALID_PARAMETER;
}

/********************************************************************************/
/**
    Save configuration
 */
result_t mpiEmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
    result_t    errResult    = XERROR_INVALID_PARAMETER;
    mpi_t *    pMPI = (mpi_t *)pEmuDevice;
    
    ASSERT_MPI(pMPI);
    if ( pMPI != NULL )
    {
        // capture the current state
        pMPI->confNumSlots = pMPI->numSlots;
        pMPI->confSwitchSlot = pMPI->confSwitchSlot;
        
        // and save it
        confSetInt(config, MPI_CONF_SECTION, MPI_CONF_NUMSLOTS, pMPI->confNumSlots);
        confSetInt(config, MPI_CONF_SECTION, MPI_CONF_SWITCHSLOT, pMPI->confSwitchSlot);

        // save the paths to the paks
        // the paks' config will be saved separately
        for (int i=0; i<pMPI->confNumSlots; i++)
        {
            cocopak_t * pak = pMPI->slots[i];
            
            if (pak != NULL)
            {
                char temp[256];
                
                sprintf(temp,"%s_%d", MPI_CONF_PAKPATH, i);
            
                confSetPath(config, MPI_CONF_SECTION, temp, pak->pPathname, config->absolutePaths);
            }
        }
        
        errResult = XERROR_NONE;
    }
    
    return errResult;
}

/********************************************************************************/
/**
    Load configuration
 */
result_t mpiEmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
    result_t    errResult    = XERROR_INVALID_PARAMETER;
    mpi_t *    pMPI = (mpi_t *)pEmuDevice;
    
    ASSERT_MPI(pMPI);
    if ( pMPI != NULL )
    {
        // load number of slots
        /* errResult = */ confGetInt(config, MPI_CONF_SECTION, MPI_CONF_NUMSLOTS, &pMPI->confNumSlots);
        // load switch position
        /* errResult = */ confGetInt(config, MPI_CONF_SECTION, MPI_CONF_SWITCHSLOT, &pMPI->confSwitchSlot);

        _mpiReallocateSlotArray(pMPI);

        for (int i=0; i<pMPI->confNumSlots; i++)
        {
            char key[PATH_MAX];
            char * path = NULL;

            sprintf(key,"%s_%d", MPI_CONF_PAKPATH, i);
            
            if ( confGetPath(config, MPI_CONF_SECTION, key, &path, config->absolutePaths) == XERROR_NONE )
            {
                /* errResult = */ _mpiLoadPak(pMPI, path, i);
                
                free(path);
            }
        }

        // remove existing paks?
        // load paks
        // add as children
            // which in turn will allow them to laod themselves
        assert(false && "TODO: Load all paks");
        
        errResult = XERROR_NONE;
    }
    
    return errResult;
}

/*********************************************************************************/

result_t mpiEmuDevGetStatus(emudevice_t * pEmuDevice, char * pszText, size_t szText)
{
    result_t        errResult    = XERROR_INVALID_PARAMETER;
    mpi_t *        pMPI        = (mpi_t *)pEmuDevice;
    
    ASSERT_MPI(pMPI);
    if ( pMPI != NULL )
    {
        sprintf(pszText,"MPI: (%d) %d:%d",pMPI->switchSlot, pMPI->chipSelectSlot, pMPI->spareSelectSlot);
        
        errResult = XERROR_NONE;
    }
    
    return errResult;
}

/*********************************************************************************/
/**
    create menu for Multi-Pak
 */
result_t mpiEmuDevCreateMenu(emudevice_t * pEmuDevice)
{
    result_t        errResult    = XERROR_INVALID_PARAMETER;
    mpi_t *         pMPI        = (mpi_t *)pEmuDevice;
    hmenu_t         hMenu;
    emudevice_t *   pEmuDevRoot;
    int             x;
    char            Temp[256];
    char            File[256];
    
    ASSERT_MPI(pMPI);
    if ( pMPI != NULL )
    {
        assert(pMPI->pak.device.hMenu == NULL);
        
        // TODO: this should get the parent's menu, not a specific one
        // we will add to the cart menu
        // find root device
        pEmuDevRoot = emuDevGetParentModuleByID(&pMPI->pak.device,VCC_CORE_ID);
        assert(pEmuDevRoot != NULL);
        // find cart menu
        hMenu = menuFind(pEmuDevRoot->hMenu,CARTRIDGE_MENU);

        menuAddSeparator(hMenu);
        menuAddItem(hMenu,"MPI Config", EMUDEV_UICOMMAND_CREATE(&pMPI->pak.device,MPI_COMMAND_CONFIG) );
        
        for (x=0; x<pMPI->numSlots; x++)
        {
            menuAddSeparator(hMenu);
            
            sprintf(Temp,"Slot %d - Load",x);
            menuAddItem(hMenu, Temp, EMUDEV_UICOMMAND_CREATE(&pMPI->pak.device,MPI_COMMAND_PAK_LOAD+x) );
            
            if ( pMPI->slots[x] == NULL )
            {
                sprintf(Temp,"Slot %d",x);
            }
            else
            {
                pathGetFilename(pMPI->slots[x]->pPathname,File,sizeof(File));
                sprintf(Temp,"Slot %d - Eject: %s",x,File);
            }
            menuAddItem(hMenu, Temp, EMUDEV_UICOMMAND_CREATE(&pMPI->pak.device,MPI_COMMAND_PAK_EJECT+x) );
        }
    }
    
    return errResult;
}

/*********************************************************************************/
/**
    emulator device callback to validate a device command
 */
bool mpiEmuDevValidate(emudevice_t * pEmuDev, int iCommand, int * piState)
{
    bool           bValid      = false;
    mpi_t *        pMPI        = (mpi_t *)pEmuDev;
    
    ASSERT_MPI(pMPI);
    
    switch ( iCommand )
    {
        case MPI_COMMAND_CONFIG:
            bValid = FALSE;
            break;
        
        default:
            if (    iCommand >= MPI_COMMAND_PAK_LOAD
                 && iCommand < MPI_COMMAND_PAK_LOAD + pMPI->numSlots
                )
            {
                bValid = true;
            }
            else if (    iCommand >= MPI_COMMAND_PAK_EJECT
                      && iCommand < MPI_COMMAND_PAK_EJECT + pMPI->numSlots
                     )
            {
                int slot = iCommand - MPI_COMMAND_PAK_EJECT;
                bValid = (pMPI->slots[slot] != NULL);
            }
            else
            {
                assert(0 && "FD-502 command not recognized");
            }
        break;
    }
    
    return bValid;
}

/*********************************************************************************/
/**
 emulator device callback to perform device specific command
 */
result_t mpiEmuDevCommand(emudevice_t * pEmuDev, int iCommand, int iParam)
{
    result_t        errResult    = XERROR_NONE;
    mpi_t *        pMPI        = (mpi_t *)pEmuDev;
    
    ASSERT_MPI(pMPI);
    
    switch ( iCommand )
    {
        case MPI_COMMAND_CONFIG:
            assert(false && "implement ui preference panel");
            break;
            
        default:
            if (    iCommand >= MPI_COMMAND_PAK_LOAD
                && iCommand < MPI_COMMAND_PAK_LOAD + pMPI->numSlots
                )
            {
                //int slot = iCommand - MPI_COMMAND_PAK_LOAD;
                assert(false && "load pack into slot");
            }
            else if (    iCommand >= MPI_COMMAND_PAK_EJECT
                     && iCommand < MPI_COMMAND_PAK_EJECT + pMPI->numSlots
                     )
            {
                //int slot = iCommand - MPI_COMMAND_PAK_EJECT;
                assert(false && "eject pack from slot");
            }
            else
            {
                assert(0 && "MPI command not recognized");
            }
            break;
    }
    
    return errResult;
}

/*********************************************************************/
/*********************************************************************/

#pragma mark -
#pragma mark --- PAK API Functions ---

/*********************************************************************/
/**
 @param pPak	Pointer to self - MPI object
 */
unsigned char mpiPortRead(emudevice_t * pEmuDevice, unsigned char Port)
{
	mpi_t *		pMPI;
	cocopak_t *	pCurPak;
	
	pMPI = (mpi_t *)pEmuDevice;
	ASSERT_MPI(pMPI);
	
	if ( Port == 0x7F )
	{
        // TODO: this necessary?
		//pMPI->slotRegister &= 0xCC;
        
		pMPI->slotRegister |= (pMPI->spareSelectSlot | (pMPI->chipSelectSlot<<4));
		
		return pMPI->slotRegister;
	}
	
	if ( (Port>=0x40) & (Port<=0x5F) )
	{
		pCurPak = pMPI->slots[pMPI->spareSelectSlot];
		
		if ( pCurPak != NULL )
		{
			if ( pCurPak->pakapi.pfnPakPortRead !=  NULL )
			{
				return (*pCurPak->pakapi.pfnPakPortRead)(&pCurPak->device,Port);
			}
		}
		
		return 0;
	}
	
	int Temp2 = 0;
	for (int Temp=0;Temp<pMPI->numSlots;Temp++)
	{
		pCurPak = pMPI->slots[Temp];
		
		if ( pCurPak != NULL )
		{
			if ( pCurPak->pakapi.pfnPakPortRead !=  NULL )
			{
				Temp2 = (*pCurPak->pakapi.pfnPakPortRead)(&pCurPak->device,Port);
			}
			
			if ( Temp2 != 0 )
			{
				return Temp2;
			}
		}
	}
	
	return 0;
}

/*********************************************************************/
/**
	@param pPak	Pointer to self - MPI object
 */
void mpiPortWrite(emudevice_t * pEmuDevice, unsigned char Port, unsigned char Data)
{
	mpi_t *			pMPI;
	cocopak_t *		pCurPak;
	coco3_t *		pCoco3;
	
	pMPI = (mpi_t *)pEmuDevice;
	ASSERT_MPI(pMPI);
	
	pCoco3 = (coco3_t *)pMPI->pak.device.pParent;
	assert(pCoco3 != NULL);
	
	// Addressing the Multi-Pak?
	if ( Port == 0x7F ) 
	{
        // TODO: verify
		pMPI->spareSelectSlot	= (Data & 3);
		pMPI->chipSelectSlot	= ((Data & 0x30)>>4);
		pMPI->slotRegister		= Data;
		
		assert(0);
		/*
		assert(pMPI->pak.pakapi.pfnPakSetCart != NULL);
		
		//PakSetCart(0);
		(*pMPI->pak.pakapi.pfnPakSetCart)(&pMPI->pak,pCoco3->pMC6821,0);
		pCurPak = pMPI->slots[pMPI->SpareSelectSlot];
		if ( pCurPak != NULL )
		{
			//PakSetCart(1);
			(*pMPI->pak.pakapi.pfnPakSetCart)(&pMPI->pak,pCoco3->pMC6821,1);
		}
		 */
	}
    
	// write to slot specific area?
	else if ( (Port>=0x40) & (Port<=0x5F) )
	{
		pCurPak = pMPI->slots[pMPI->spareSelectSlot];

		if ( pCurPak != NULL )
		{
            // TODO: this shoudl be in the pak port write
            
			pCurPak->rom.BankedCartOffset = (Data & 15) << 14;
			
			if ( pCurPak->pakapi.pfnPakPortWrite != NULL )
			{
				(*pCurPak->pakapi.pfnPakPortWrite)(&pCurPak->device,Port,Data);
			}
		}
	}
	else
	{
		// general port write, send to all paks
		for (int Temp=0; Temp<pMPI->numSlots; Temp++)
		{
			pCurPak = pMPI->slots[Temp];
			
			if ( pCurPak != NULL )
			{
				if ( pCurPak->pakapi.pfnPakPortWrite != NULL )
				{
					(*pCurPak->pakapi.pfnPakPortWrite)(&pCurPak->device,Port,Data);
				}
			}
		}
	}
}

/*********************************************************************/
/**
	@param pPak	Pointer to self - MPI object
 */
unsigned char mpiMemRead8(emudevice_t * pEmuDevice, int Address)
{
	mpi_t *		pMPI;
	cocopak_t *	pCurPak;
	
	pMPI = (mpi_t *)pEmuDevice;
	ASSERT_MPI(pMPI);
	
	pCurPak = pMPI->slots[pMPI->chipSelectSlot];
	if ( pCurPak != NULL )
	{
		if ( (*pCurPak->pakapi.pfnPakMemRead8) != NULL )
		{
			return (*pCurPak->pakapi.pfnPakMemRead8)(&pCurPak->device,Address);
		}
	}
	
	return 0;
}

/*********************************************************************/
/**
	@param pPak	Pointer to self - MPI object
 */
void mpiMemWrite8(emudevice_t * pEmuDevice, unsigned char Data, int Address)
{
	mpi_t *		pMPI;
	cocopak_t *	pCurPak;
	
	pMPI = (mpi_t *)pEmuDevice;
	ASSERT_MPI(pMPI);
	
	pCurPak = pMPI->slots[pMPI->chipSelectSlot];
	if ( pCurPak != NULL )
	{
		if ( (*pCurPak->pakapi.pfnPakMemWrite8) != NULL )
		{
			return (*pCurPak->pakapi.pfnPakMemWrite8)(&pCurPak->device,Data,Address);
		}
	}
}

/*********************************************************************/
/**
	@param pPak	Pointer to self - MPI object
 */
void mpiHeartBeat(cocopak_t * pPak)
{
	mpi_t *		pMPI;
	cocopak_t *	pCurPak;
	
	assert(pPak != NULL);
	
	pMPI = (mpi_t *)pPak;
	ASSERT_MPI(pMPI);
	
    // necessary once all paks are registered / inserted into the tree?
	for (int Temp=0; Temp<pMPI->numSlots; Temp++)
	{
		pCurPak = pMPI->slots[Temp];
		if ( pCurPak != NULL )
		{
			if ( pCurPak->pakapi.pfnPakHeartBeat != NULL )
			{
				(*pCurPak->pakapi.pfnPakHeartBeat)(pCurPak);
			}
		}
	}
}

/*********************************************************************/
/**
	@param pPak	Pointer to self - MPI object
 */
void mpiStatus(cocopak_t * pPak, char * pszStatus)
{
	char            TempStatus[256]="";
	mpi_t *         pMPI;
    cocopak_t *		pCurPak;
    
	assert(pPak != NULL);
	
	pMPI = (mpi_t *)pPak;
	ASSERT_MPI(pMPI);
	
	sprintf(pszStatus,"MPI:%i,%i",pMPI->chipSelectSlot,pMPI->spareSelectSlot);
	
    // necessary once all paks are inserted into the tree?
	for (int Temp=0; Temp<pMPI->numSlots; Temp++)
	{
		strcpy(TempStatus,"");
        
        pCurPak = pMPI->slots[Temp];
		if (    pCurPak != NULL
             && pCurPak->device.pfnGetStatus != NULL
            )
		{
			(*pCurPak->device.pfnGetStatus)(&pCurPak->device,TempStatus,sizeof(TempStatus));
            
			strcat(pszStatus,"|");
			strcat(pszStatus,TempStatus);
		}
	}
}

/*********************************************************************/
/**
	@param pPak	Pointer to self - MPI object
 */
// This gets called at the end of every scan line 262 Lines * 60 Frames = 15780 Hz 15720

unsigned short mpiAudioSample(cocopak_t * pPak)
{
	unsigned short TempSample=0;
	mpi_t *					pMPI;
	cocopak_t *				pCurPak;
	
	assert(pPak != NULL);
	
	pMPI = (mpi_t *)pPak;
	ASSERT_MPI(pMPI);
	
    // necessary once all paks are inserted into the tree?
	for (int Temp=0; Temp<pMPI->numSlots; Temp++)
	{
		pCurPak = pMPI->slots[Temp];
		if ( pCurPak != NULL )
		{
			if ( pCurPak->pakapi.pfnPakAudioSample != NULL )
			{
				TempSample += (*pCurPak->pakapi.pfnPakAudioSample)(pCurPak);
			}
		}
	}
		
	return (TempSample);
}

/*********************************************************************/
/**
    Module configuration GUI
 
    @param pPak	Pointer to self - MPI object
 */
void mpiConfig(cocopak_t * pPak, unsigned char MenuID)
{
    assert(false && "TODO: implement");
}

/*********************************************************************/
/**
	@param pPak Pointer to self - MPI object
 */
void mpiReset(cocopak_t * pPak)
{
	mpi_t *			pMPI;
	cocopak_t *		pCurPak;
	coco3_t *		pCoco3;
	
	assert(pPak != NULL);
	if ( pPak != NULL )
	{
		pMPI = (mpi_t *)pPak;
		ASSERT_MPI(pMPI);
		
		pCoco3 = (coco3_t *)pMPI->pak.device.pParent;
		assert(pCoco3 != NULL);
		
        // set number of slots based on config
        pMPI->numSlots    = pMPI->confNumSlots > 0 ? pMPI->confNumSlots : MPI_DEFAULT_NUM_SLOTS;        /* default number of slots */
        assert(pMPI->numSlots > 0);
        
        _mpiReallocateSlotArray(pMPI);
        
        // default to top slot
        pMPI->switchSlot = pMPI->numSlots-1;
        
        assert(pMPI->switchSlot >=0 && pMPI->switchSlot < pMPI->numSlots);
        
        pMPI->spareSelectSlot = pMPI->chipSelectSlot = pMPI->switchSlot;   /* default to switch 1 */

        /*
		 (re)register our port read/write functions
		 */
        // TODO: is this the correct range or is it $40-5F and $7F
		portRegisterRead(pCoco3->pGIME,mpiPortRead,&pPak->device,0xFF40,0xFF7F);
		portRegisterWrite(pCoco3->pGIME,mpiPortWrite,&pPak->device,0xFF40,0xFF7F);

		pMPI->chipSelectSlot	= pMPI->switchSlot;
		pMPI->spareSelectSlot	= pMPI->switchSlot;
		
        // can be removed once emuDevReset used
		for (int Temp=0; Temp<pMPI->numSlots; Temp++)
		{
			pCurPak = pMPI->slots[Temp];
			if ( pCurPak != NULL )
			{
				// TODO: ROM pack should reset itself
				pCurPak->rom.BankedCartOffset = 0; // Do I need to keep independant selects?
				
				if ( pCurPak->pakapi.pfnPakReset != NULL )
				{
					(*pCurPak->pakapi.pfnPakReset)(pCurPak);
				}
			}
		}
		
        // TODO: should CARTs assert themselves instead of by pakInterface or by MPI?
        mc6821_t * pMC6821 = (mc6821_t *)emuDevFindByDeviceID(&pCoco3->machine.device, VCC_MC6821_ID);
        assert(pMC6821 != NULL);
        
        // no CART
        mc6821_SetCART(pMC6821,High);
        
        // is there a cartridge in the current selected slot?
		pCurPak = pMPI->slots[pMPI->switchSlot];
		if ( pCurPak != NULL )
		{
            mc6821_SetCART(pMC6821,Low);
		}
	}
}

/*********************************************************************/
/*********************************************************************/

#pragma mark -
#pragma mark --- MPI PAK exported create function ---

/*********************************************************************/
/**
	MPI instance creation
 
    TODO: add a way to force this going into the machine and not into another MPI slot
 */
XAPI_EXPORT cocopak_t * vccpakModuleCreate()
{
	mpi_t *		pMPI;
	
	pMPI = calloc(1,sizeof(mpi_t));
	if ( pMPI != NULL )
	{
		pMPI->pak.device.id			= EMU_DEVICE_ID;
		pMPI->pak.device.idModule	= VCC_COCOPAK_ID;
		pMPI->pak.device.idDevice	= VCC_PAK_MPI_ID;
		
        pMPI->pak.device.pfnDestroy    = mpiEmuDevDestroy;
        pMPI->pak.device.pfnSave       = mpiEmuDevConfSave;
        pMPI->pak.device.pfnLoad       = mpiEmuDevConfLoad;
        pMPI->pak.device.pfnCreateMenu = mpiEmuDevCreateMenu;
        pMPI->pak.device.pfnValidate   = mpiEmuDevValidate;
        pMPI->pak.device.pfnCommand    = mpiEmuDevCommand;
        pMPI->pak.device.pfnGetStatus  = mpiEmuDevGetStatus;
        
		/*
		 Initialize PAK API function pointers
		 */
		pMPI->pak.pakapi.pfnPakReset            = mpiReset;
		pMPI->pak.pakapi.pfnPakAudioSample		= mpiAudioSample;
		pMPI->pak.pakapi.pfnPakConfig           = mpiConfig;
		pMPI->pak.pakapi.pfnPakHeartBeat		= mpiHeartBeat;
		pMPI->pak.pakapi.pfnPakPortRead         = mpiPortRead;
		pMPI->pak.pakapi.pfnPakPortWrite		= mpiPortWrite;
		pMPI->pak.pakapi.pfnPakMemRead8         = mpiMemRead8;
		pMPI->pak.pakapi.pfnPakMemWrite8		= mpiMemWrite8;	
		
		strcpy(pMPI->pak.device.Name,"Multi-Pak Interface (xx-xxxx)");
		
		emuDevRegisterDevice(&pMPI->pak.device);

        // set switch
        pMPI->switchSlot = 0;
        
		return &pMPI->pak;
	}
	
	return NULL;
}

/*********************************************************************/
/*********************************************************************/
