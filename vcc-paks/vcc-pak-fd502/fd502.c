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

/*****************************************************************************/
/*
	This Module will emulate a Tandy Floppy Disk Model FD-502 
    With 3 DSDD drives attached
	Copyright 2006 (c) by Joseph Forgione
*/
/*****************************************************************************/
/*
	TODO: restore keyboard indicators (WIN32)
	TODO: restore FDRawCmd (Win32)
    TODO: change base address of FDC?
    TODO: if the RTC address overlaps the FDC address, nothing will get through to the FDC
 */
/*****************************************************************************/

#include "fd502.h"

#include "file.h"
#include "path.h"
#include "coco3.h"
#include "Vcc.h"
#include "xSystem.h"

#include <stdio.h>
#include <assert.h>

/*****************************************************************************/
/**
 */
long CreateDiskHeader(char * FileName, unsigned char Type, unsigned char Tracks, unsigned char DblSided)
{
	FILE *			hr					= NULL;
	unsigned char	Dummy				= 0;
	unsigned char	HeaderBuffer[16]	= "";
	unsigned char	TrackTable[3]		= {35,40,80};
	unsigned short	TrackSize			= 0x1900;
	unsigned char	IgnoreDensity		= 0;
	unsigned char	SingleDensity		= 0;
	unsigned char	HeaderSize			= 0;
	//unsigned long	BytesWritten		= 0;
	unsigned long	FileSize			= 0;
	
	hr = fopen(FileName, "r+");
	if ( hr == NULL )
	{
		return 1; //Failed to create File
	}
	
	switch ( Type )
	{
		case DMK:
			HeaderBuffer[0]=0;
			HeaderBuffer[1]=TrackTable[Tracks];
			HeaderBuffer[2]=(TrackSize & 0xFF);
			HeaderBuffer[3]=(TrackSize >>8);
			HeaderBuffer[4]=(IgnoreDensity<<7) | (SingleDensity<<6) | ((!DblSided)<<4);
			HeaderBuffer[0xC]=0;
			HeaderBuffer[0xD]=0;
			HeaderBuffer[0xE]=0;
			HeaderBuffer[0xF]=0;
			HeaderSize=0x10;
			FileSize= HeaderSize + (HeaderBuffer[1] * TrackSize * (DblSided+1) );
		break;

		case JVC:	// has variable header size
			HeaderSize=0;
			HeaderBuffer[0]=18;			//18 Sectors
			HeaderBuffer[1]=DblSided+1;	// Double or single Sided;
			FileSize = (HeaderBuffer[0] * 0x100 * TrackTable[Tracks] * (DblSided+1));
			if (DblSided)
			{
				FileSize+=2;
				HeaderSize=2;
			}
		break;

		case VDK:	
			HeaderBuffer[9]=DblSided+1;
			HeaderSize=12;
			FileSize = ( 18 * 0x100 * TrackTable[Tracks] * (DblSided+1));
			FileSize+=HeaderSize;
		break;

		case 3:
			HeaderBuffer[0]=0;
			HeaderBuffer[1]=TrackTable[Tracks];
			HeaderBuffer[2]=(TrackSize & 0xFF);
			HeaderBuffer[3]=(TrackSize >>8);
			HeaderBuffer[4]=((!DblSided)<<4);
			HeaderBuffer[0xC]=0x12;
			HeaderBuffer[0xD]=0x34;
			HeaderBuffer[0xE]=0x56;
			HeaderBuffer[0xF]=0x78;
			HeaderSize=0x10;
			FileSize=1;
		break;
	}
    
	fseek(hr, 0, SEEK_SET);
	
    /* BytesWritten = */ fwrite(HeaderBuffer, HeaderSize, 1, hr);
    
	fseek(hr, FileSize-1, SEEK_SET);
	
    /* BytesWritten = */ fwrite(&Dummy, 1, 1, hr);
    
	fclose(hr);
	
	return 0;
}

/*********************************************************************/
/*********************************************************************/

#pragma mark -
#pragma mark --- PAK API Functions ---

/*********************************************************************/
/**
 */
void fd502HeartBeat(cocopak_t * pPak)
{
	fd502_t *		pFD502 = (fd502_t *)pPak;
    
	ASSERT_FD502(pFD502);
	
	if ( pFD502 != NULL )
	{
		wd1793HeartBeat(&pFD502->wd1793);
    }
}

/*********************************************************************/
/**
 */
void fd502PortWrite(emudevice_t * pEmuDevice, unsigned char port, unsigned char data)
{
	fd502_t *		pFD502;
	
    // not SCS range?
    if ( port < 0x40 || port > 0x5F )
    {
        return;
    }
    
	assert(pEmuDevice != NULL);
	if ( pEmuDevice != NULL )
	{
		pFD502 = (fd502_t *)pEmuDevice;
		ASSERT_FD502(pFD502);
		
        if ( pFD502->confDistoRTCEnabled )
        {
            // check for access to RTC
            distortc_t *    pDistoRTC = (distortc_t *)pFD502->pDistoRTC;
            ASSERT_DISTORTC(pDistoRTC);
            if (    port == (pDistoRTC->baseAddress & 0xFF)+0
                 || port == (pDistoRTC->baseAddress & 0xFF)+1
               )
            {
                distoRTCPortWrite(&pFD502->pDistoRTC->device,port,data);
                return;
            }
        }
        
		wd1793IOWrite(&pFD502->wd1793,port,data);
	}
}

/*********************************************************************/
/**
 */
unsigned char fd502PortRead(emudevice_t * pEmuDevice, unsigned char port)
{
	fd502_t *		pFD502;
	
    // not SCS range?
    if ( port < 0x40 || port > 0x5F )
    {
        return 0;
    }
    
    assert(pEmuDevice != NULL);
	if ( pEmuDevice != NULL )
	{
		pFD502 = (fd502_t *)pEmuDevice;
		ASSERT_FD502(pFD502);
		
        if ( pFD502->confDistoRTCEnabled )
        {
            distortc_t *    pDistoRTC = (distortc_t *)pFD502->pDistoRTC;
            ASSERT_DISTORTC(pDistoRTC);
            
            if (   port == (pDistoRTC->baseAddress & 0xFF)+0
                || port == (pDistoRTC->baseAddress & 0xFF)+1
                )
            {
                return distoRTCPortRead(&pFD502->pDistoRTC->device,port);
            }
        }
        
		return wd1793IORead(&pFD502->wd1793,port);
	}
	
	return 0;
}

/*********************************************************************/
/**
	TODO: deprecate, use cocopak ROM read
 */
unsigned char fd502MemRead8(emudevice_t * pEmuDevice, int Address)
{
	fd502_t *		pFD502;
	
	pFD502 = (fd502_t *)pEmuDevice;
	ASSERT_FD502(pFD502);
	
	return (pFD502->pak.rom.pData[Address & (pFD502->pak.rom.szData-1)]);
}

/*********************************************************************/
/**
 */
/*
 void fd502MemWrite8(emudevice_t * pModule, unsigned char Data, int Address)
{
}
*/

/*********************************************************************/
/**
 */
void fd502Config(cocopak_t * pPak, unsigned char MenuID)
{
	assert(0);
}

/*********************************************************************/
/**
    @param pPak Pointer to self
 */
void fd502Reset(cocopak_t * pPak)
{
	fd502_t *		pFD502;
	
	pFD502 = (fd502_t *)pPak;
	ASSERT_FD502(pFD502);
	if ( pFD502 != NULL )
	{
        result_t romLoadResult = XERROR_GENERIC;
        
        // load ROM
        if ( pFD502->confRomPath != NULL )
        {
            // specific file from User
            romLoadResult = romLoad(&pFD502->pak.rom,pFD502->confRomPath);
            
            if ( romLoadResult != XERROR_NONE )
            {
                emuDevLog(&pFD502->pak.device,"Error loading user specified FD502 ROM");
            }
        }
        if ( romLoadResult != XERROR_NONE )
        {
            char temp[PATH_MAX];
            sysGetPathResources("disk11.rom",temp,sizeof(temp));
            romLoadResult = romLoad(&pFD502->pak.rom,temp);
            
            if ( romLoadResult != XERROR_NONE )
            {
                emuDevLog(&pFD502->pak.device,"Failed to laod default FD502 ROM");
            }
        }
        if ( romLoadResult == XERROR_NONE )
        {
            emuDevLog(&pPak->device, "Disk ROM loaded successfully");
        }
        else
        {
            emuDevLog(&pPak->device, "Disk ROM loaded FAILED");
        }
        
        // initialize 1793
        wd1793Reset(&pFD502->wd1793);
        
        // apply settings
        wd1793SetTurboMode(&pFD502->wd1793,pFD502->confTurboDisk);

		distoRTCReset(pFD502->pDistoRTC);
        
        // TODO: RTC settings
	}
}

/********************************************************************************/

#pragma mark -
#pragma mark --- emulator device callbacks ---

/*********************************************************************/
/**
	FD502 interface instance destruction
 */
result_t fd502EmuDevDestroy(emudevice_t * pEmuDevice)
{
	fd502_t *	pFD502 = (fd502_t *)pEmuDevice;
	
	ASSERT_FD502(pFD502);
	if ( pFD502 != NULL )
	{	
		/* remove our child device WD1793 */
		emuDevRemoveChild(&pFD502->pak.device,&pFD502->wd1793.device);

		romDestroy(&pFD502->pak.rom);
		
        // TODO: check
		//distoRTCDestroy(pFD502->pDistoRTC);		
		//pFD502->pDistoRTC = NULL;
		
		/* detach ourselves from our parent */
		emuDevRemoveChild(pFD502->pak.device.pParent,&pFD502->pak.device);
		
        if (pFD502->confLastPath != NULL)
        {
            free(pFD502->confLastPath);
            pFD502->confLastPath = NULL;
        }
        
        if (pFD502->confRomPath != NULL)
        {
            free(pFD502->confRomPath);
            pFD502->confRomPath = NULL;
        }

        free(pEmuDevice);
		
		return XERROR_NONE;
	}
	
	return XERROR_INVALID_PARAMETER;
}

/********************************************************************************/
/**
    FD502 emuDev config save callback
 
 TODO: handle errors?
 */
result_t fd502EmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	fd502_t *	pFD502 = (fd502_t *)pEmuDevice;
	
	ASSERT_FD502(pFD502);
	if ( pFD502 != NULL )
	{
        // FDC settingsd
        /* errResult = */ confSetPath(config, FD502_CONF_SECTION, FD502_CONF_SETTING_LASTPATH, pFD502->confLastPath, config->absolutePaths);
        /* errResult = */ confSetPath(config, FD502_CONF_SECTION, FD502_CONF_SETTING_ROMPATH, pFD502->confRomPath, config->absolutePaths);
        /* errResult = */ confSetInt(config, FD502_CONF_SECTION, FD502_CONF_SETTING_TURBODISK, pFD502->confTurboDisk);

        // Disto RTC settings
        /* errResult = */ confSetInt(config, FD502_CONF_SECTION, FD502_CONF_DISTO_RTC_ENABLED, pFD502->confDistoRTCEnabled);
        /* errResult = */ confSetInt(config, FD502_CONF_SECTION, FD502_CONF_DISTO_RTC_ADDRESS, pFD502->confDistoRTCAddress);

        errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/
/**
 FD502 emuDev config load callback
 
 TODO: handle errors?
 TODO: validate paths?
 */
result_t fd502EmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	fd502_t *	pFD502 = (fd502_t *)pEmuDevice;
	
	ASSERT_FD502(pFD502);
	if ( pFD502 != NULL )
	{	
        // Last disk path path
        /* errResult = */ confGetPath(config, FD502_CONF_SECTION, FD502_CONF_SETTING_LASTPATH, &pFD502->confLastPath,config->absolutePaths);
        
        // Rom path
        /* errResult = */ confGetPath(config, FD502_CONF_SECTION, FD502_CONF_SETTING_ROMPATH, &pFD502->confRomPath,config->absolutePaths);

        // Turbo disk
        int value;
        if ( confGetInt(config,FD502_CONF_SECTION,FD502_CONF_SETTING_TURBODISK,&value) == XERROR_NONE )
        {
            pFD502->confTurboDisk = value;
        }

        // RTC Enable
        if ( confGetInt(config,FD502_CONF_SECTION,FD502_CONF_DISTO_RTC_ENABLED,&value) == XERROR_NONE )
        {
            pFD502->confDistoRTCEnabled = value;
        }
        // RTC address
        if ( confGetInt(config,FD502_CONF_SECTION,FD502_CONF_DISTO_RTC_ADDRESS,&value) == XERROR_NONE )
        {
            pFD502->confDistoRTCAddress = value;
        }

        errResult = XERROR_NONE;
	}
	
	return errResult;
}

/*********************************************************************************/
/**
 */
result_t fd502EmuDevGetStatus(emudevice_t * pEmuDevice, char * pszText, size_t szText)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	fd502_t *		pFD502		= (fd502_t *)pEmuDevice;
	wd1793_t *		pWD1793;
	
	ASSERT_FD502(pFD502);
	if ( pFD502 != NULL )
	{
		pWD1793 = &pFD502->wd1793;
		
		if ( pWD1793->MotorOn == 1 ) 
		{
			sprintf(pszText,"FD-502: Drv:%1.1i %s T:%2.2i S:%2.2i H:%1.1i",	pWD1793->CurrentDisk,
																			ImageFormat[pWD1793->Drive[pWD1793->CurrentDisk].ImageType],
																			pWD1793->Drive[pWD1793->CurrentDisk].HeadPosition,
																			pWD1793->SectorReg,
																			pWD1793->Side
					);
		}
		else
		{
			sprintf(pszText,"FD-502: Idle");
		}
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/*********************************************************************************/
/**
	create menu for FD-502
 */
result_t fd502EmuDevCreateMenu(emudevice_t * pEmuDevice)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	fd502_t *		pFD502		= (fd502_t *)pEmuDevice;
	hmenu_t			hMenu;
	emudevice_t *	pEmuDevRoot;
	int			    x;
	char			Temp[256];
	char			File[256];
	
	ASSERT_FD502(pFD502);
	if ( pFD502 != NULL )
	{
		assert(pFD502->pak.device.hMenu == NULL);
		
        // TODO: this should get the parent's menu, not a specific one
        // Add a method into the pak interface to get the Pak's menu
        
		// we will add to the cart menu
		// find root device
		pEmuDevRoot = emuDevGetParentModuleByID(&pFD502->pak.device,VCC_CORE_ID);
		assert(pEmuDevRoot != NULL);
		// find cart menu
		hMenu = menuFind(pEmuDevRoot->hMenu,CARTRIDGE_MENU);
		
        menuAddSeparator(hMenu);

        // add config menu item
		menuAddItem(hMenu,"FD-502 Config", EMUDEV_UICOMMAND_CREATE(&pFD502->pak.device,FD502_COMMAND_CONFIG) );
		
        // Add set ROM menu item
        menuAddItem(hMenu,"Select Disk ROM", EMUDEV_UICOMMAND_CREATE(&pFD502->pak.device,FD502_COMMAND_SET_ROM) );

        // add FD502 settings sub-menu
        hmenu_t hSettingsMenu = menuCreate("FDC Settings");
        menuAddItem(hSettingsMenu,"Turbo Disk", EMUDEV_UICOMMAND_CREATE(&pFD502->pak.device,FD502_COMMAND_TURBO_DISK) );
        menuAddSubMenu(hMenu, hSettingsMenu);

        hmenu_t hRTCMenu = menuCreate("Disto RTC Settings");
        menuAddItem(hRTCMenu,"RTC Enabled", EMUDEV_UICOMMAND_CREATE(&pFD502->pak.device,FD502_COMMAND_DISTO_ENABLE) );
        menuAddItem(hRTCMenu,"RTC @ FF50", EMUDEV_UICOMMAND_CREATE(&pFD502->pak.device,FD502_COMMAND_DISTO_ADDRESS+0) );
        menuAddItem(hRTCMenu,"RTC @ FF58", EMUDEV_UICOMMAND_CREATE(&pFD502->pak.device,FD502_COMMAND_DISTO_ADDRESS+1) );
        menuAddSubMenu(hMenu, hRTCMenu);

        for (x=0; x<4; x++)
		{
			menuAddSeparator(hMenu);
			
			sprintf(Temp,"Drive %d - Load",x);
			menuAddItem(hMenu,Temp,	EMUDEV_UICOMMAND_CREATE(&pFD502->pak.device,FD502_COMMAND_DISK0_LOAD+x) );
			
			if ( pFD502->wd1793.Drive[x].pPathname == NULL )
			{
				sprintf(Temp,"Drive %d - Eject",x);
			}
			else 
			{
				pathGetFilename(pFD502->wd1793.Drive[x].pPathname,File,sizeof(File));
				sprintf(Temp,"Drive %d - Eject: %s",x,File);
			}
			menuAddItem(hMenu,Temp,	EMUDEV_UICOMMAND_CREATE(&pFD502->pak.device,FD502_COMMAND_DISK0_EJECT+x) );
		}
	}
	
	return errResult;
}

/*********************************************************************************/
/**
	emulator device callback to validate a device command
 */
bool fd502EmuDevValidate(emudevice_t * pEmuDev, int iCommand, int * piState)
{
	bool			bValid		= FALSE;
	fd502_t *		pFD502		= (fd502_t *)pEmuDev;
	int			    iDrive = -1;
	
	ASSERT_FD502(pFD502);
		
	switch ( iCommand )
	{
		case FD502_COMMAND_CONFIG:
            // TODO: when config UI is added
			bValid = FALSE;
		break;
            
        case FD502_COMMAND_SET_ROM:
            // TODO: show ROM name if set (set menu name)
            bValid = true;
        break;
            
        case FD502_COMMAND_TURBO_DISK:
            if ( piState != NULL )
            {
                *piState = (pFD502->confTurboDisk ? COMMAND_STATE_ON : COMMAND_STATE_OFF);
            }
            bValid = true;
        break;
            
		case FD502_COMMAND_DISK0_LOAD:
		case FD502_COMMAND_DISK1_LOAD:
		case FD502_COMMAND_DISK2_LOAD:
		case FD502_COMMAND_DISK3_LOAD:
			bValid = TRUE;
		break;
			
		case FD502_COMMAND_DISK0_EJECT:
		case FD502_COMMAND_DISK1_EJECT:
		case FD502_COMMAND_DISK2_EJECT:
		case FD502_COMMAND_DISK3_EJECT:
			// get disk number
			iDrive = iCommand - FD502_COMMAND_DISK0_EJECT;
			
			bValid = (pFD502->wd1793.Drive[iDrive].pPathname != NULL);
			break;
			
        case FD502_COMMAND_DISTO_ENABLE:
            if ( piState != NULL )
            {
                *piState = (pFD502->confDistoRTCEnabled ? COMMAND_STATE_ON : COMMAND_STATE_OFF);
            }
            bValid = true;
            break;
            
        case FD502_COMMAND_DISTO_ADDRESS+0:
        case FD502_COMMAND_DISTO_ADDRESS+1:
        {
            if ( piState != NULL )
            {
                switch ( iCommand )
                {
                    case FD502_COMMAND_DISTO_ADDRESS+0:
                        *piState = (pFD502->confDistoRTCAddress == 0xFF50 ? COMMAND_STATE_ON : COMMAND_STATE_OFF);
                        break;
                    case FD502_COMMAND_DISTO_ADDRESS+1:
                        *piState = (pFD502->confDistoRTCAddress == 0xFF58 ? COMMAND_STATE_ON : COMMAND_STATE_OFF);
                        break;
                }
            }
            bValid = true;
        }
        break;
            
		default:
			assert(0 && "FD-502 command not recognized");
	}
	
	return bValid;
}

/*********************************************************************************/
/**
	emulator device callback to perform device specific command
 */
result_t fd502EmuDevCommand(emudevice_t * pEmuDev, int iCommand, int iParam)
{
	result_t		errResult	= XERROR_NONE;
	fd502_t *		pFD502		= (fd502_t *)pEmuDev;
	vccinstance_t *	pInstance;
	int             iDrive      = -1;
    bool            updateUI    = false;
    
	ASSERT_FD502(pFD502);
	
	pInstance = (vccinstance_t *)emuDevGetParentModuleByID(&pFD502->pak.device,VCC_CORE_ID);
	ASSERT_VCC(pInstance);
	
	// do command
	switch ( iCommand )
	{
		case FD502_COMMAND_CONFIG:
			assert(0 && "Config UI not yet implemented");
		break;
			
        case FD502_COMMAND_TURBO_DISK:
            // toggle turbo disk
            pFD502->confTurboDisk = !pFD502->confTurboDisk;
            wd1793SetTurboMode(&pFD502->wd1793, pFD502->confTurboDisk);
            updateUI = true;
        break;
            
        case FD502_COMMAND_SET_ROM:
        {
            const char * pPathname;
            
            filetype_e types[] = { COCO_PAK_ROM, COCO_FILE_NONE };

            // get pathname
            pPathname = sysGetPathnameFromUser(&types[0],pFD502->confRomPath);
            if ( pPathname != NULL )
            {
                if (pFD502->confRomPath != NULL)
                {
                    free(pFD502->confRomPath);
                    pFD502->confRomPath = NULL;
                }
                pFD502->confRomPath = strdup(pPathname);
                
                // set pending power cycle
                vccSetCommandPending(pInstance, VCC_COMMAND_POWERCYCLE);
                updateUI = true;
            }
        }
        break;
            
		case FD502_COMMAND_DISK0_LOAD:
		case FD502_COMMAND_DISK1_LOAD:
		case FD502_COMMAND_DISK2_LOAD:
		case FD502_COMMAND_DISK3_LOAD:
		{
			const char * pPathname;
			
			// get disk number
			iDrive = iCommand - FD502_COMMAND_DISK0_LOAD;
			
            filetype_e types[] = { COCO_VDISK_FLOPPY, COCO_FILE_NONE };
            
			// get pathname
			pPathname = sysGetPathnameFromUser(&types[0],pFD502->confLastPath);
			if ( pPathname != NULL )
			{
				// attempt to mount the disk
				wd1793MountDiskImage(&pFD502->wd1793, pPathname, iDrive);
                
                if (pFD502->confLastPath != NULL)
                {
                    free(pFD502->confLastPath);
                    pFD502->confLastPath = NULL;
                }
                pFD502->confLastPath = strdup(pPathname);
                updateUI = true;
			}
		}
		break;

		case FD502_COMMAND_DISK0_EJECT:
		case FD502_COMMAND_DISK1_EJECT:
		case FD502_COMMAND_DISK2_EJECT:
		case FD502_COMMAND_DISK3_EJECT:
		{
			// get disk number
			iDrive = iCommand - FD502_COMMAND_DISK0_EJECT;
			
			// unmount the disk
			wd1793UnmountDiskImage(&pFD502->wd1793, iDrive);
            updateUI = true;
		}
		break;
			
        case FD502_COMMAND_DISTO_ENABLE:
            pFD502->confDistoRTCEnabled = !pFD502->confDistoRTCEnabled;
            updateUI = true;
            break;
            
        case FD502_COMMAND_DISTO_ADDRESS+0:
        case FD502_COMMAND_DISTO_ADDRESS+1:
        {
            pFD502->confDistoRTCAddress = (iCommand-FD502_COMMAND_DISTO_ADDRESS == 0 ? 0xFF50 : 0xFF58);
            
            distortc_t *    pDistoRTC = (distortc_t *)pFD502->pDistoRTC;
            ASSERT_DISTORTC(pDistoRTC);
            pDistoRTC->baseAddress = pFD502->confDistoRTCAddress;
            updateUI = true;
        }
        break;

		default:
			assert(0 && "Coco3 command not recognized");
		break;
	}
	
    if ( updateUI )
    {
        vccUpdateUI(pInstance);
    }
    
	return errResult;
}

/*********************************************************************/
/*********************************************************************/

#pragma mark -
#pragma mark --- FD-502 Interface exported create function ---

/*********************************************************************/
/**
	instance creation
 */
XAPI_EXPORT cocopak_t * vccpakModuleCreate()
{
	fd502_t *		pFD502;
	
	pFD502 = calloc(1,sizeof(fd502_t));
	if ( pFD502 != NULL )
	{
		pFD502->pak.device.id		= EMU_DEVICE_ID;
		pFD502->pak.device.idModule	= VCC_COCOPAK_ID;
		pFD502->pak.device.idDevice	= VCC_PAK_FD502_ID;
		
		pFD502->pak.device.pfnDestroy		= fd502EmuDevDestroy;
		pFD502->pak.device.pfnSave			= fd502EmuDevConfSave;
		pFD502->pak.device.pfnLoad			= fd502EmuDevConfLoad;
		pFD502->pak.device.pfnCreateMenu	= fd502EmuDevCreateMenu;
		pFD502->pak.device.pfnValidate		= fd502EmuDevValidate;
		pFD502->pak.device.pfnCommand		= fd502EmuDevCommand;
		pFD502->pak.device.pfnGetStatus		= fd502EmuDevGetStatus;
		
		/*
		 Initialize PAK API function pointers
		 */
		//pFD502->pak.pakapi.pfnPakDestroy		= fd502Destroy;
		pFD502->pak.pakapi.pfnPakReset			= fd502Reset;
		pFD502->pak.pakapi.pfnPakConfig         = fd502Config;
		pFD502->pak.pakapi.pfnPakHeartBeat		= fd502HeartBeat;
		
		pFD502->pak.pakapi.pfnPakPortRead		= fd502PortRead;
		pFD502->pak.pakapi.pfnPakPortWrite		= fd502PortWrite;
		
		pFD502->pak.pakapi.pfnPakMemRead8		= NULL; //fd502MemRead8;
		pFD502->pak.pakapi.pfnPakMemWrite8		= NULL; //fd502MemWrite8;

		strcpy(pFD502->pak.device.Name,"FD502 Floppy Disk Interface (xx-xxxx)");
				
		emuDevRegisterDevice(&pFD502->pak.device);
		
		/*
			initialize floppy controller object
		 */
		wd1793Init(&pFD502->wd1793);
		emuDevAddChild(&pFD502->pak.device,&pFD502->wd1793.device);

        // Set default FDC settings
        pFD502->confDistoRTCEnabled = true;
        pFD502->confDistoRTCAddress = 0xFF50;
        
		/*
			create Disto RTC object
		 */
		pFD502->pDistoRTC = distoRTCCreate();
		assert(pFD502->pDistoRTC != NULL);
		
        // set default RTC settings
        distortc_t *    pDistoRTC = (distortc_t *)pFD502->pDistoRTC;
        ASSERT_DISTORTC(pDistoRTC);
        pDistoRTC->baseAddress = pFD502->confDistoRTCAddress;
        
		return &pFD502->pak;
	}
	
	return NULL;
}

/*****************************************************************************/
