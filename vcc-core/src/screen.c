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
	CoCo3 'screen' object.
 
    The UI connects to this for platform specific drawing.
    The screen object is responsible for copying
	from Coco3 graphics to this buffer.
*/
/********************************************************************/

#include "Screen.h"

#include "tcc1014graphics.h"
#include "tcc1014mmu.h"
#include "coco3.h"
#include "vcc.h"

#include "xDebug.h"

/********************************************************************/

void MakeRGBPalette(screen_t * pScreen);
void MakeCMPpalette(screen_t * pScreen);

/********************************************************************/

gime_t * _getGIME(screen_t * pScreen)
{
    ASSERT_SCREEN(pScreen);
    
    if ( pScreen != NULL )
    {
        if ( pScreen->pGIME == NULL )
        {
            coco3_t * pCoco3 = (coco3_t *)emuDevGetParentModuleByID(&pScreen->device, EMU_MACHINE_ID);
            pScreen->pGIME = pCoco3->pGIME;
        }
        
        return pScreen->pGIME;
    }
    
    return NULL;
}

/********************************************************************/
/*
    artifact colours
 */
//const unsigned char     Afacts8[2][4]   = { { 0,0xA4,0x89,0xBF }, { 0,137,164,191} };
//const unsigned short	Afacts16[2][4]  = {0,0xF800,0x001F,0xFFFF,0,0x001F,0xF800,0xFFFF};
const unsigned int		Afacts32[2][4]  = {
    {
        0xFF000000, 0xFFFF0000, 0xFF0000FF, 0xFFFFFFFF
    },
    {
        0xFF000000, 0xFF0000FF, 0xFFFF0000, 0xFFFFFFFF
    }
};

/********************************************************************************/

#pragma mark -
#pragma mark --- local macros ---

/********************************************************************************/

bool surfaceInit(surface_t * surface, int width, int height, int bitDepth)
{
    assert(surface != NULL);
    if ( surface != NULL )
    {
        int bytespp = bitDepth>>3;
        
        surface->width = width;
        surface->height = height;
        surface->bitDepth = bitDepth;
        surface->linePitch = surface->width * bytespp;
        
        surface->pBuffer = (uint8_t *)calloc(1, surface->linePitch * surface->height );
        
        return (surface->pBuffer != NULL);
    }
    
    return false;
}

void surfaceFreeStorage(surface_t * surface)
{
    assert(surface != NULL);
    if ( surface != NULL )
    {
        if ( surface->pBuffer != NULL )
        {
            free(surface->pBuffer);
            surface->pBuffer = NULL;
            
            surface->bitDepth = 0;
            surface->linePitch = 0;
            surface->width = 0;
            surface->height = 0;
        }
    }
}

uint8_t * surfaceGetLinePtr(surface_t * surface, int row)
{
    assert(surface != NULL);

    assert(row >= 0 && row < surface->height);
    
    return surface->pBuffer + (row * surface->linePitch);
}

void surfaceFill(surface_t * surface, uint32_t color)
{
    assert(surface != NULL);
    
    for (int y=0; y<surface->height; y++)
    {
        uint32_t * line = (uint32_t *)surfaceGetLinePtr(surface, y);
        
        for (int x=0; x<surface->width; x++)
        {
            line[x] = color;
        }
    }
}

#pragma mark -
#pragma mark --- emulator device callbacks ---

/********************************************************************/
/**
 */
result_t screenEmuDevDestroy(emudevice_t * pEmuDevice)
{
	screen_t *	pScreen;
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	
	pScreen = (screen_t *)pEmuDevice;
	
	ASSERT_SCREEN(pScreen);
	
	if ( pScreen != NULL )
	{
		emuDevRemoveChild(pScreen->device.pParent,&pScreen->device);
		
        surfaceFreeStorage(&pScreen->surface);
		
		free(pScreen);
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/
/**
 */
result_t screenEmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	screen_t *	pScreen;
	
	pScreen = (screen_t *)pEmuDevice;
	
	ASSERT_SCREEN(pScreen);
	
	if ( pScreen != NULL )
	{
		confSetInt(config,CONF_SECTION_VIDEO, CONF_SETTING_MONITORTYPE, pScreen->confMonitorType);
		confSetInt(config,CONF_SECTION_VIDEO, CONF_SETTING_SCANLINES, pScreen->confScanLines);
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/
/**
 */
result_t screenEmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	screen_t *	pScreen;
	int 		iValue;
	
	pScreen = (screen_t *)pEmuDevice;
	
	ASSERT_SCREEN(pScreen);
	
	if ( pScreen != NULL )
	{
		/*
		 Screen
		 */
		if ( confGetInt(config, CONF_SECTION_VIDEO, CONF_SETTING_MONITORTYPE, &iValue) == XERROR_NONE )
		{
			pScreen->confMonitorType = iValue;
		}
		
		if ( confGetInt(config, CONF_SECTION_VIDEO, CONF_SETTING_SCANLINES, &iValue) == XERROR_NONE )
		{
			pScreen->confScanLines = iValue;
		}
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/*********************************************************************************/
/**
 create menu for VCC (highest level)
 */
result_t screenEmuDevCreateMenu(emudevice_t * pEmuDev)
{
	screen_t *		pScreen	= (screen_t *)pEmuDev;
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	
	ASSERT_SCREEN(pScreen);
	if ( pScreen != NULL )
	{
		assert(pScreen->device.hMenu == NULL);
		
		// create menu
		pScreen->device.hMenu = menuCreate(pScreen->device.Name);
		
        hmenu_t hMonitorMenu = menuCreate("Monitor Type");
		menuAddItem(hMonitorMenu,"Composite",	(pScreen->device.iCommandID<<16) | SCREEN_COMMAND_MONTYPE + eMonType_CMP);
        menuAddItem(hMonitorMenu,"RGB",    (pScreen->device.iCommandID<<16) | SCREEN_COMMAND_MONTYPE + eMonType_RGB);
        menuAddSubMenu(pScreen->device.hMenu, hMonitorMenu);

        menuAddItem(pScreen->device.hMenu,"Scan Lines",		(pScreen->device.iCommandID<<16) | SCREEN_COMMAND_SCANLINES);
        if ( pScreen->pScreenFullScreenCallback != NULL )
        {
            menuAddItem(pScreen->device.hMenu,"Full Screen",    (pScreen->device.iCommandID<<16) | SCREEN_COMMAND_FULLSCREEN);
        }
        
        // TODO: move to VCC?
        menuAddItem(pScreen->device.hMenu,"Screen Shot",    (pScreen->device.iCommandID<<16) | SCREEN_COMMAND_SCREEN_SHOT);
	}
	
	return errResult;
}

/*********************************************************************************/
/**
    emulator device callback to validate a device command
 */
bool screenEmuDevValidate(emudevice_t * pEmuDev, int iCommand, int * piState)
{
	bool			bValid		= FALSE;
	screen_t *		pScreen		= (screen_t *)pEmuDev;
	
	ASSERT_SCREEN(pScreen);
	
	switch ( iCommand )
	{
		case SCREEN_COMMAND_MONTYPE + eMonType_CMP:
			if ( piState != NULL )
			{
				*piState = ((pScreen->confMonitorType == eMonType_CMP) ? COMMAND_STATE_ON : COMMAND_STATE_OFF);
			}
			bValid = true;
			break;
			
        case SCREEN_COMMAND_MONTYPE + eMonType_RGB:
            if ( piState != NULL )
            {
                *piState = ((pScreen->confMonitorType == eMonType_RGB) ? COMMAND_STATE_ON : COMMAND_STATE_OFF);
            }
            bValid = true;
            break;
            
		case SCREEN_COMMAND_SCANLINES:
			if ( piState != NULL )
			{
				*piState = (pScreen->confScanLines ? COMMAND_STATE_ON : COMMAND_STATE_OFF);
			}
			bValid = true;
			break;
			
		case SCREEN_COMMAND_FULLSCREEN:
			if ( piState != NULL )
			{
				*piState = (pScreen->confFullScreen ? COMMAND_STATE_ON : COMMAND_STATE_OFF);
			}
            bValid = true;
			break;
			
        case SCREEN_COMMAND_SCREEN_SHOT:
            bValid = true;
            break;

        default:
			assert(false && "Screen command not recognized");
            break;
	}
	
	return bValid;
}

/*********************************************************************************/
/**
    emulator device callback to perform VCC high level command
 */
result_t screenEmuDevCommand(emudevice_t * pEmuDev, int iCommand, int iParam)
{
	result_t		errResult	= XERROR_NONE;
	screen_t *		pScreen;
	coco3_t *		pCoco3;
	
    pScreen = (screen_t *)pEmuDev;
    ASSERT_SCREEN(pScreen);
    
    // assumes the parent object of the screen is the machine/coco3
    pCoco3 = (coco3_t *)pScreen->device.pParent;
    ASSERT_CC3(pCoco3);
    
	// do command
	switch ( iCommand )
	{
		case SCREEN_COMMAND_MONTYPE + eMonType_CMP:
        case SCREEN_COMMAND_MONTYPE + eMonType_RGB:
			pScreen->confMonitorType = (iCommand - SCREEN_COMMAND_MONTYPE);
			SetMonitorType(pScreen, pScreen->confMonitorType);
			break;
			
		case SCREEN_COMMAND_SCANLINES:
            // toggle scan lines
			pScreen->confScanLines = ! pScreen->confScanLines;
			break;
			
		case SCREEN_COMMAND_FULLSCREEN:
            // toggle full screen
            pScreen->confFullScreen = ! pScreen->confFullScreen;
            if ( pScreen->pScreenFullScreenCallback != NULL )
            {
                pScreen->pScreenFullScreenCallback(pScreen->pScreenCallbackParam);
            }
			break;
			
        case SCREEN_COMMAND_SCREEN_SHOT:
        {
            emurootdevice_t * rootDevice = emuDevGetRootDevice(&pScreen->device);
            vccScreenShot(rootDevice);
        }
        break;

        default:
			assert(0 && "Screen command not recognized");
			break;
	}
	
	return errResult;
}

/********************************************************************************/
/********************************************************************************/

#pragma mark -
#pragma mark --- implementation ---

/********************************************************************************/
/**
 */
screen_t * screenCreate()
{
	screen_t *	pScreen;
	
	pScreen = calloc(1,sizeof(screen_t));
	if ( pScreen != NULL )
	{
		pScreen->device.id			= EMU_DEVICE_ID;
		pScreen->device.idModule	= EMU_DEVICE_ID;
        pScreen->device.idDevice    = VCC_SCREEN_ID;
		strcpy(pScreen->device.Name,"Screen");
		
		pScreen->device.pfnDestroy		= screenEmuDevDestroy;
		pScreen->device.pfnSave			= screenEmuDevConfSave;
		pScreen->device.pfnLoad			= screenEmuDevConfLoad;
		pScreen->device.pfnCreateMenu	= screenEmuDevCreateMenu;
		pScreen->device.pfnValidate		= screenEmuDevValidate;
		pScreen->device.pfnCommand		= screenEmuDevCommand;
		
		emuDevRegisterDevice(&pScreen->device);
		
		/*
			config defaults
		 */
		pScreen->confMonitorType	= eMonType_RGB;
		pScreen->confScanLines		= false;
        pScreen->confFullScreen     = false;
        
        pScreen->MonType = pScreen->confMonitorType;
        
		/*
            Create screen
		 */
		pScreen->WindowSizeX		= 640;
		pScreen->WindowSizeY		= 480;
		
        MakeRGBPalette(pScreen);
        MakeCMPpalette(pScreen);
        
        // vdg mode
        pScreen->LinesperRow=1;
        pScreen->BytesperRow=32;
        pScreen->VPitch=32;
        
        if ( ! surfaceInit(&pScreen->surface, pScreen->WindowSizeX, pScreen->WindowSizeY, 32) )
        {
            emuDevDestroy(&pScreen->device);
            pScreen = NULL;
        }
	}
	
	return pScreen;
}

#if 0
pGIME->GraphicsMode=0;

#endif

/********************************************************************/
/**
 */
void screenDisplayFlip(screen_t * pScreen)
{
	ASSERT_SCREEN(pScreen);
	
	if ( pScreen != NULL )
	{
		if ( pScreen->pScreenFlipCallback != NULL )
		{
			(*pScreen->pScreenFlipCallback)(pScreen->pScreenCallbackParam);
		}
	}
}

/********************************************************************/
/**
 */
void screenClearBuffer(screen_t * pScreen)
{
	ASSERT_SCREEN(pScreen);
	
	if ( screenLock(pScreen) )
	{
		return;
	}
	
    surfaceFill(&pScreen->surface, 0); //pScreen->Color);
    
	screenUnlock(pScreen);
}

/********************************************************************/
/**
	Fill screen buffer with static i.e. an analog TV with no signal
 */
void screenStatic(screen_t * pScreen)
{
	unsigned short	x				= 0;
	unsigned short	y				= 0;
	unsigned char	Temp			= 0;
//	unsigned char	GreyScales[4]	= {128,135,184,191};
	
	ASSERT_SCREEN(pScreen);
	
	screenLock(pScreen);
		
    assert(pScreen->surface.bitDepth == 32);
    
    for (y=0;y<pScreen->WindowSizeY;y++)
    {
        uint32_t * line = (uint32_t *)surfaceGetLinePtr(&pScreen->surface, y);
        
        for (x=0;x<pScreen->WindowSizeX;x++)
        {
            Temp = rand() & 255;
            line[x] = Temp | (Temp<<8) | (Temp <<16);
        }
    }

	screenUnlock(pScreen);
}

/********************************************************************/
/**
	Lock the screen buffer
 
	Display specific 
*/
int screenLock(screen_t * pScreen)
{
	ASSERT_SCREEN(pScreen);
	
	if ( pScreen->pScreenLockCallback != NULL )
	{
		(*pScreen->pScreenLockCallback)(pScreen->pScreenCallbackParam);
	}
	
	return (0);
}

/********************************************************************/
/**
	Unlock the screen buffer
 
	Display specific
*/
void screenUnlock(screen_t * pScreen)
{
	ASSERT_SCREEN(pScreen);
	
	if ( pScreen->pScreenUnlockCallback != NULL )
	{
		(*pScreen->pScreenUnlockCallback)(pScreen->pScreenCallbackParam);
	}

	// DisplayFlip(pScreen);
}

#pragma mark -
#pragma mark --- from GIME ---

/****************************************************************************/

//const unsigned char ColorValues[4]={0,85,170,255};
const unsigned char ColorTable16Bit[4]={0,10,21,31};    //Color brightness at 0 1 2 and 3 (2 bits)
const unsigned char ColorTable32Bit[4]={0,85,170,255};

uint32_t rgba(int r, int g, int b, int a)
{
    r &= 0xFF;
    g &= 0xFF;
    b &= 0xFF;
    a &= 0xFF;
    
#if (defined _MAC)
    return (a << 24) | (b << 16) | (g << 8) | r;
#else
    return (a << 24) | (r << 16) | (g << 8) | b;
#endif
}

// Create RGB monitor palette
void MakeRGBPalette(screen_t * pScreen)
{
    unsigned char r,g,b;
    
    assert(pScreen != NULL);
    
    for (int Index=0; Index<64; Index++)
    {
        //pScreen->PalleteLookup8[RGB][Index]= Index |128;
        
        // RGB-555?
        //r = ColorTable16Bit [(Index & 32) >> 4 | (Index & 4) >> 2];
        //g = ColorTable16Bit [(Index & 16) >> 3 | (Index & 2) >> 1];
        //b = ColorTable16Bit [(Index & 8 ) >> 2 | (Index & 1) ];
        //pScreen->PalleteLookup16[RGB][Index] = (r<<11) | (g << 6) | b;
        
        // ARGB
        r = ColorTable32Bit[(Index & 32) >> 4 | (Index & 4) >> 2];
        g = ColorTable32Bit[(Index & 16) >> 3 | (Index & 2) >> 1];
        b = ColorTable32Bit[(Index & 8 ) >> 2 | (Index & 1) ];
        pScreen->PalleteLookup32[eMonType_RGB][Index] = rgba(r,g,b,255);
    }
}

// Create Composite monitor palette
// borrowed from M.E.S.S.
void MakeCMPpalette(screen_t * pScreen)
{
    double saturation, brightness, contrast;
    int offset;
    double w;
    double r,g,b;
    unsigned char rr,gg,bb;
    unsigned char Index=0;
    
    assert(pScreen != NULL);
    
    for (Index=0;Index<=63;Index++)
    {
        switch(Index)
        {
            case 0:
                r = g = b = 0;
                break;
                
            case 16:
                r = g = b = 47;
                break;
                
            case 32:
                r = g = b = 120;
                break;
                
            case 48:
            case 63:
                r = g = b = 255;
                break;
                
            default:
                w = .4195456981879*1.01;
                contrast = 70;
                saturation = 92;
                brightness = -20;
                brightness += ((Index / 16) + 1) * contrast;
                offset = (Index % 16) - 1 + (Index / 16)*15;
                r = cos(w*(offset +  9.2)) * saturation + brightness;
                g = cos(w*(offset + 14.2)) * saturation + brightness;
                b = cos(w*(offset + 19.2)) * saturation + brightness;
                
                if (r < 0)
                    r = 0;
                else if (r > 255)
                    r = 255;
                
                if (g < 0)
                    g = 0;
                else if (g > 255)
                    g = 255;
                
                if (b < 0)
                    b = 0;
                else if (b > 255)
                    b = 255;
                break;
        }
        
        rr = (unsigned char)r;
        gg = (unsigned char)g;
        bb = (unsigned char)b;
        
        pScreen->PalleteLookup32[eMonType_CMP][Index] = rgba(rr,gg,bb,255);
        
        //rr = rr>>3;
        //gg = gg>>3;
        //bb = bb>>3;
        //pScreen->PalleteLookup16[CMP][Index] = (rr<<11) | (gg<<6) | bb;
        
        //rr = rr>>3;
        //gg = gg>>3;
        //bb = bb>>3;
        //pScreen->PalleteLookup8[CMP][Index] = 0x80 | ((rr & 2)<<4) | ((gg & 2)<<3) | ((bb & 2)<<2) | ((rr & 1)<<2) | ((gg & 1)<<1) | (bb & 1);
    }
}

void screenUpdatePalette(screen_t * pScreen)
{
    ASSERT_SCREEN(pScreen);
    
    gime_t * pGIME = _getGIME(pScreen);

    // update palette
    for (int PalNum=0; PalNum<16; PalNum++)
    {
        //pScreen->Pallete8Bit[PalNum]   = pScreen->PalleteLookup8[pGIME->MonType][pGIME->Pallete[PalNum]];
        //pScreen->Pallete16Bit[PalNum]  = pScreen->PalleteLookup16[pGIME->MonType][pGIME->Pallete[PalNum]];
        pScreen->Pallete32Bit[PalNum]  = pScreen->PalleteLookup32[pScreen->MonType][pGIME->Pallete[PalNum]];
    }
}

void SetMonitorType(screen_t * pScreen, montype_e monType)
{
    ASSERT_SCREEN(pScreen);
    
    pScreen->MonType = (int)monType & 1;
    
    screenUpdatePalette(pScreen);
}

int GetMonitorType(screen_t * pScreen)
{
    return pScreen->MonType;
}

/*
 unsigned char SetArtifacts(screen_t * pScreen, unsigned char Tmp)
 {
 assert(pGIME != NULL);
 
 if (Tmp!=QUERY)
 Artifacts=Tmp;
 return(Artifacts);
 }
 */

/*
 unsigned char SetColorInvert(screen_t * pScreen, unsigned char Tmp)
 {
 assert(pGIME != NULL);
 
 if (Tmp!=QUERY)
 ColorInvert=Tmp;
 return(ColorInvert);
 }
 */

/**
 Set up for video/render/conversion
 Called every time a video register is changed
 */

void screenUpdateBorderColor(screen_t *pScreen)
{
    gime_t * pGIME = _getGIME(pScreen);
    int         borderColor = 0;
    
    switch (pGIME->CompatMode)
    {
        case 0:
        {
            borderColor = pGIME->CC3BorderColor;
        }
        break;
            
        case 1:
        {
            borderColor=0;    //Black for text modes
            
            if (pScreen->rowLatch.graphicsMode)
            {
                // white - not right?
                borderColor=63;
            }
        }
        break;
    }
    
    pScreen->rowLatch.borderColor = pScreen->PalleteLookup32[pScreen->MonType][borderColor & 63];
}

void _SetupDisplay(screen_t * pScreen)
{
    //const unsigned char Lpf[4]={192,199,225,225}; // 2 is really undefined but I gotta put something here.
    //const unsigned char VcenterTable[4]={29,23,12,12};
    const unsigned char Lpf[4]={192,200,225,225};
    const unsigned char VcenterTable[4]={24,16,8,8};
    
    ASSERT_SCREEN(pScreen);
    
    gime_t * pGIME = _getGIME(pScreen);
    
    pScreen->ColorInvert=1;
    pScreen->BlinkState=1;

    switch (pGIME->CompatMode)
    {
        case 0:
        {
            unsigned char CC3LinesperRow[8]             = {1,1,2,8,9,10,11,200};
            const unsigned char CC3BytesperRow[8]       = {16,20,32,40,64,80,128,160};
            const unsigned char CC3BytesperTextRow[8]   = {32,40,32,40,64,80,64,80};
            
            //Color Computer 3 Mode
            pScreen->rowLatch.newStartofVidram = pGIME->VerticalOffsetRegister*8;
            
            pScreen->GraphicsMode = (pGIME->CC3Vmode & 128)>>7;
            pScreen->VresIndex=(pGIME->CC3Vres & 96) >> 5;
            CC3LinesperRow[7] = pScreen->LinesperScreen;    // For 1 pixel high modes
            pScreen->Bpp=pGIME->CC3Vres & 3;
            pScreen->LinesperRow=CC3LinesperRow[pGIME->CC3Vmode & 7];
            pScreen->BytesperRow=CC3BytesperRow[(pGIME->CC3Vres & 28)>> 2];
            pScreen->PalleteIndex=0;
            if (!pScreen->GraphicsMode)
            {
                if (pGIME->CC3Vres & 1)
                    pGIME->ExtendedText=2;
                pScreen->Bpp=0;
                pScreen->BytesperRow=CC3BytesperTextRow[(pGIME->CC3Vres & 28)>> 2];
            }
        }
        break;
            
        case 1:
        {
            unsigned char        Temp1;
            
            const unsigned char CC2Bpp[8]            = {1,0,1,0,1,0,1,0};
            const unsigned char CC2LinesperRow[8]    = {12,3,3,2,2,1,1,1};
            const unsigned char CC2BytesperRow[8]    = {16,16,32,16,32,16,32,32};
            const unsigned char CC2PaletteSet[4]    = {8,0,10,4};
            
            //Color Computer 2 Mode
            //pGIME->CC3BorderColor=0;    //Black for text modes
            pScreen->rowLatch.newStartofVidram = (512*pGIME->CC2Offset)+(pGIME->VerticalOffsetRegister & 0xE0FF)*8;
            pScreen->GraphicsMode=( pGIME->CC2VDGPiaMode & 16 )>>4; //PIA Set on graphics clear on text
            pScreen->VresIndex=0;
            pScreen->LinesperRow= CC2LinesperRow[pGIME->CC2VDGMode];
            
            if (pScreen->GraphicsMode)
            {
                //pGIME->CC3BorderColor=63;
                pScreen->Bpp=CC2Bpp[ (pGIME->CC2VDGPiaMode & 15) >>1 ];
                pScreen->BytesperRow=CC2BytesperRow[ (pGIME->CC2VDGPiaMode & 15) >>1 ];
                Temp1= (pGIME->CC2VDGPiaMode &1) << 1 | (pScreen->Bpp & 1);
                pScreen->PalleteIndex=CC2PaletteSet[Temp1];
            }
            else
            {    //Setup for 32x16 text Mode
                unsigned char        ColorSet            = 0;
                
                pScreen->Bpp=0;
                pScreen->BytesperRow=32;
                pGIME->InvertAll= (pGIME->CC2VDGPiaMode & 4)>>2;
                pGIME->LowerCase= (pGIME->CC2VDGPiaMode & 2)>>1;
                ColorSet = (pGIME->CC2VDGPiaMode & 1);
                Temp1 = ( (ColorSet<<1) | pGIME->InvertAll);
                switch (Temp1)
                {
                    case 0:
                        pScreen->TextFGPallete=12;
                        pScreen->TextBGPallete=13;
                        break;
                    case 1:
                        pScreen->TextFGPallete=13;
                        pScreen->TextBGPallete=12;
                        break;
                    case 2:
                        pScreen->TextFGPallete=14;
                        pScreen->TextBGPallete=15;
                        break;
                    case 3:
                        pScreen->TextFGPallete=15;
                        pScreen->TextBGPallete=14;
                        break;
                }
            }
        }
        break;
    }
    
    pScreen->ColorInvert = (pGIME->CC3Vmode & 32)>>5;
    
    pScreen->LinesperScreen = Lpf[pScreen->VresIndex];
    
    pScreen->TopBorder = VcenterTable[pScreen->VresIndex&3];
    
    const unsigned char CCPixelsperByte[4] = {8,4,2,2};
    pScreen->PixelsperByte = CCPixelsperByte[pScreen->Bpp];
    pScreen->PixelsperLine = pScreen->BytesperRow * pScreen->PixelsperByte;
    
    pScreen->VPitch = pScreen->BytesperRow;
    
    if (pGIME->HorzOffsetReg & 128)
    {
        pScreen->VPitch=256;
    }
    
    // update latched values for rendering the current row of text or graphics
    pScreen->rowLatch.newStartofVidram = (pScreen->rowLatch.newStartofVidram & pGIME->iVidMask) + pGIME->DistoOffset; //DistoOffset for 2M configuration
    
    if (pScreen->PixelsperLine % 40)
    {
        pScreen->rowLatch.stretch = (512/pScreen->PixelsperLine)-1;
        pScreen->rowLatch.horzCenter = 64;
    }
    else
    {
        pScreen->rowLatch.stretch = (640/pScreen->PixelsperLine)-1;
        pScreen->rowLatch.horzCenter = 0;
    }
    
    if (    pScreen->rowLatch.graphicsMode != pScreen->GraphicsMode
         || pScreen->rowLatch.compatMode != pGIME->CompatMode
       )
    {
        pScreen->rowLatch.srcRow = 0;
        pScreen->rowLatch.startofVidram = pScreen->rowLatch.newStartofVidram;
    }
    
    int previousMode = pScreen->rowLatch.graphicsMode;
    
    pScreen->rowLatch.compatMode    = pGIME->CompatMode;
    pScreen->rowLatch.graphicsMode  = pScreen->GraphicsMode;
    pScreen->rowLatch.hOffset       = pGIME->Hoffset;
    pScreen->rowLatch.extendedText  = pGIME->ExtendedText;
    pScreen->rowLatch.lowerCase     = pGIME->LowerCase;
    
    // change in mode - update the border color
    if ( previousMode != pScreen->rowLatch.graphicsMode )
    {
        screenUpdateBorderColor(pScreen);
    }
    
    ////////////////
    // hack
    // this fixes a problem with Musica
    // when the mode switches from text to graphics.  It is as if the source row
    // the GIME reads is not reset when the line of text is finished rendering
    // but instead is reset when the GIME is told to switch modes mid text line
    // so we latch the row count as if it was counting up since then
    if (    pScreen->rowLatch.graphicsMode
         && previousMode != pScreen->rowLatch.graphicsMode
       )
    {
        pScreen->rowLatch.srcRow = pGIME->preLatchRow+1;
    }
    
    //screenUpdatePalette(pScreen);
    //screenUpdateBorderColor(pScreen);
}

void SetupDisplay(screen_t * pScreen)
{
    screenUpdatePalette(pScreen);
//screenUpdateBorderColor(pScreen);
}

void InitDisplay(screen_t * pScreen)
{
    screenUpdatePalette(pScreen);
    screenUpdateBorderColor(pScreen);
    _SetupDisplay(pScreen);
}

/********************************************************************************/

#pragma mark -
#pragma mark --- screen rendering ---

/**
 */
void screenUpdateTopBorder(screen_t * pScreen, int borderLine)
{
    uint32_t * dstRowPtr = (uint32_t *)surfaceGetLinePtr(&pScreen->surface, borderLine);
    
    // if scan lines is enabled, skip odd lines
    if (    pScreen->confScanLines
        && (borderLine%2)==1
        )
    {
        memset(dstRowPtr,0,pScreen->surface.linePitch);
    }
    else
    {
        for (int i=0; i<pScreen->surface.width; i++)
        {
            dstRowPtr[i] = pScreen->rowLatch.borderColor;
        }
    }
}

/**
 */
void screenUpdateBottomBorder(screen_t * pScreen, int borderLine)
{
    uint32_t * dstRowPtr = (uint32_t *)surfaceGetLinePtr(&pScreen->surface, borderLine);
    
    //screenUpdateBorderColor(pScreen);
    
    // if scan lines is enabled, black out odd lines
    if (    pScreen->confScanLines
        && (borderLine%2)==1
        )
    {
        memset(dstRowPtr,0,pScreen->surface.linePitch);
    }
    else
    {
        // draw bottom border line
        for (int i=0; i<pScreen->surface.width; i++)
        {
            dstRowPtr[i] = pScreen->rowLatch.borderColor;
        }
    }
}

/*
    Render one line from CoCo video memory to the 'screen' in RGBA format
*/
void screenUpdate(screen_t * pScreen, int dstRow)
{
    assert(pScreen != NULL);
    
    gime_t * pGIME = ((coco3_t *)pScreen->device.pParent)->pGIME;
    assert(pGIME != NULL);

    uint8_t * RamBuffer;
    RamBuffer = mmuGetRAMBuffer(pGIME);
    if ( RamBuffer == NULL ) return;
    
    uint32_t *  dstRowPtr   = NULL;
    uint32_t    dstX        = -1;       // set for pre-increment
    
    // time to latch new row settings?
    if ( !pScreen->rowLatch.graphicsMode )
    {
        // in text mode, only reset the display at the beginning of a text row
        if ( (dstRow%12)==0 )
        {
            _SetupDisplay(pScreen);
        }
    }
    else
    {
        // this is slight overkill but we can do this each line in graphics mode
        _SetupDisplay(pScreen);
    }
    
    rowlatch_t * rowLatch = &pScreen->rowLatch;
    
    // ???
    if (pScreen->LinesperRow < 13)
    {
        rowLatch->srcRow++;
        pGIME->preLatchRow++;
    }
    if ( dstRow == 0 )
    {
        // reset the source row
        rowLatch->srcRow = 0;
        rowLatch->startofVidram = rowLatch->newStartofVidram;
    }
    
    // we render every other line starting after the top border
    // Note: the rendered raster line is doubled at the end of this function
    dstRowPtr = (uint32_t *)surfaceGetLinePtr(&pScreen->surface, (dstRow + pScreen->TopBorder) * 2);
    
    // left border
    for (int i=0; i<rowLatch->horzCenter; i++)
    {
        dstRowPtr[i] = pScreen->rowLatch.borderColor;
    }
    // right border
    for (int i=rowLatch->horzCenter+(pScreen->PixelsperLine*2); i<pScreen->surface.width; i++)
    {
        dstRowPtr[i] = pScreen->rowLatch.borderColor;
    }
    
    // the code below assumes the line it starts at the left edge of the
    // visible display, just past the border
    dstRowPtr += rowLatch->horzCenter;
    
    unsigned int Start = rowLatch->startofVidram + (rowLatch->srcRow/pScreen->LinesperRow)*(pScreen->VPitch*rowLatch->extendedText);

    // Set the graphics mode - this is used for rendering/converting the video to RGBA
    // pGIME->GraphicsMode  : on/off
    // pGIME->CompatMode    : on/off
    // pGIME->Bpp           : 0-3
    // Stretch              : horizontal stretch 0-15 (1-16)
    unsigned char MasterMode = (rowLatch->graphicsMode <<7) | (rowLatch->compatMode<<6)  | ((pScreen->Bpp & 3)<<4)  | (rowLatch->stretch & 0x0F);
    switch (MasterMode)
    {
        case 0: //Width 80
        {
            // TODO: untested
            
            unsigned char       Character = 0;
            unsigned int        TextPallete[2] = {0,0};
            uint8_t *           buffer = RamBuffer;
            unsigned char       Pixel=0;
            uint8_t             Attributes=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow*rowLatch->extendedText; HorzBeam+=rowLatch->extendedText)
            {
                Character = buffer[Start+(unsigned char)(HorzBeam+rowLatch->hOffset)];
                Pixel = cc3Fontdata8x12[Character * 12 + (dstRow%pScreen->LinesperRow)];
                
                if (rowLatch->extendedText == 2)
                {
                    Attributes=buffer[Start+(unsigned char)(HorzBeam+rowLatch->hOffset)+1];
                    if  ( (Attributes & 64) && (dstRow%pScreen->LinesperRow==(pScreen->LinesperRow-1)) )    //UnderLine
                        Pixel=255;
                    
                    if ((!pScreen->BlinkState) & !!(Attributes & 128))
                        Pixel=0;
                }
                TextPallete[1] = pScreen->Pallete32Bit[8+((Attributes & 56)>>3)];
                TextPallete[0] = pScreen->Pallete32Bit[Attributes & 7];
                
                dstRowPtr[dstX+=1] = TextPallete[(Pixel>>7)&1];
                dstRowPtr[dstX+=1] = TextPallete[(Pixel>>6)&1];
                dstRowPtr[dstX+=1] = TextPallete[(Pixel>>5)&1];
                dstRowPtr[dstX+=1] = TextPallete[(Pixel>>4)&1];
                dstRowPtr[dstX+=1] = TextPallete[(Pixel>>3)&1];
                dstRowPtr[dstX+=1] = TextPallete[(Pixel>>2)&1];
                dstRowPtr[dstX+=1] = TextPallete[(Pixel>>1)&1];
                dstRowPtr[dstX+=1] = TextPallete[(Pixel>>0)&1];
            }
        }
        break;
            
        case 1:
        case 2: //Width 40
        {
            unsigned char       Character = 0;
            unsigned int        TextPallete[2]={0,0};
            uint8_t *           buffer = RamBuffer;
            unsigned char       Pixel=0;
            uint8_t             Attributes=0;

            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow*rowLatch->extendedText; HorzBeam+=rowLatch->extendedText)
            {
                Character = buffer[Start+(unsigned char)(HorzBeam+rowLatch->hOffset)];
                Pixel = cc3Fontdata8x12[Character  * 12 + (dstRow%pScreen->LinesperRow)];
                if (rowLatch->extendedText == 2)
                {
                    Attributes = buffer[Start+(unsigned char)(HorzBeam+rowLatch->hOffset)+1];
                    
                    if  ( (Attributes & 64) && (dstRow%pScreen->LinesperRow==(pScreen->LinesperRow-1)) )    //UnderLine
                        Pixel=255;
                    
                    if ((!pScreen->BlinkState) & !!(Attributes & 128))
                        Pixel=0;
                }
                
                TextPallete[1]=pScreen->Pallete32Bit[8+((Attributes & 56)>>3)];
                TextPallete[0]=pScreen->Pallete32Bit[Attributes & 7];
                
                dstRowPtr[dstX+=1]=TextPallete[Pixel>>7 ];
                dstRowPtr[dstX+=1]=TextPallete[Pixel>>7 ];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>6)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>6)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>5)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>5)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>4)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>4)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>3)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>3)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>2)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>2)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>1)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>1)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel & 1)];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel & 1)];
            }
        }
        break;
            
#if 0 // unused
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47:
        case 48:
        case 49:
        case 50:
        case 51:
        case 52:
        case 53:
        case 54:
        case 55:
        case 56:
        case 57:
        case 58:
        case 59:
        case 60:
        case 61:
        case 62:
        case 63:
             for (pGIME->HorzBeam=0;pGIME->HorzBeam<pScreen->BytesperRow*pGIME->ExtendedText;pGIME->HorzBeam+=pGIME->ExtendedText)
             {
                 Character=buffer[Start+(unsigned char)(pGIME->HorzBeam+pGIME->Hoffset)];
                 if (pGIME->ExtendedText==2)
                 Attributes=buffer[Start+(unsigned char)(pGIME->HorzBeam+pGIME->Hoffset)+1];
                 else
                 Attributes=0;
                 Pixel=cc3Fontdata8x12[(Character & 127) * 8 + (y%8)];
                 if  ( (Attributes & 64) && (y%8==7) )    //UnderLine
                 Pixel=255;
                 if ((!pGIME->BlinkState) & !!(Attributes & 128))
                 Pixel=0;
                 TextPallete[1]=pScreen->Pallete32Bit[8+((Attributes & 56)>>3)];
                 TextPallete[0]=pScreen->Pallete32Bit[Attributes & 7];
                 dstRowPtr[dstX+=1]=TextPallete[(Pixel & 128)/128 ];
                 dstRowPtr[dstX+=1]=TextPallete[(Pixel & 64)/64];
                 dstRowPtr[dstX+=1]=TextPallete[(Pixel & 32)/32 ];
                 dstRowPtr[dstX+=1]=TextPallete[(Pixel & 16)/16];
                 dstRowPtr[dstX+=1]=TextPallete[(Pixel & 8)/8 ];
                 dstRowPtr[dstX+=1]=TextPallete[(Pixel & 4)/4];
                 dstRowPtr[dstX+=1]=TextPallete[(Pixel & 2)/2 ];
                 dstRowPtr[dstX+=1]=TextPallete[(Pixel & 1)];
             }
            break;
#endif
            
        case 64:
        case 65:
        case 66:
        case 67:
        case 68:
        case 69:
        case 70:
        case 71:
        case 72:
        case 73:
        case 74:
        case 75:
        case 76:
        case 77:
        case 78:
        case 79:
        case 80:
        case 81:
        case 82:
        case 83:
        case 84:
        case 85:
        case 86:
        case 87:
        case 88:
        case 89:
        case 90:
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
        case 96:
        case 97:
        case 98:
        case 99:
        case 100:
        case 101:
        case 102:
        case 103:
        case 104:
        case 105:
        case 106:
        case 107:
        case 108:
        case 109:
        case 110:
        case 111:
        case 112:
        case 113:
        case 114:
        case 115:
        case 116:
        case 117:
        case 118:
        case 119:
        case 120:
        case 121:
        case 122:
        case 123:
        case 124:
        case 125:
        case 126:
        case 127:
        {
            // TODO: optimize - doing repetetive operations
            // Low Res text GraphicsMode=0 CompatMode=1 Ignore Bpp and Stretch
            int         Character = 0;
            uint32_t    TextPallete[2]={0,0};
            uint8_t *   buffer = RamBuffer;
            uint8_t     Pixel=0;
            int         chr = dstRow%12;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam++)
            {
                Character = buffer[Start + (HorzBeam+rowLatch->hOffset)];
                
                switch ((Character & 192) >> 6)
                {
                    case 0:
                        Character = Character & 63;
                        TextPallete[0] = pScreen->Pallete32Bit[pScreen->TextBGPallete];
                        TextPallete[1] = pScreen->Pallete32Bit[pScreen->TextFGPallete];
                        if (rowLatch->lowerCase & (Character < 32))
                            Pixel = ntsc_round_fontdata8x12[(Character + 80 ) * 12 + chr];
                        else
                            Pixel = ~ntsc_round_fontdata8x12[(Character + 0) * 12 + chr];
                        break;
                        
                    case 1:
                        Character = Character & 63;
                        TextPallete[0] = pScreen->Pallete32Bit[pScreen->TextBGPallete];
                        TextPallete[1] = pScreen->Pallete32Bit[pScreen->TextFGPallete];
                        Pixel = ntsc_round_fontdata8x12[(Character + 0) * 12 + chr];
                        break;
                        
                    case 2:
                    case 3:
                        TextPallete[1] = pScreen->Pallete32Bit[(Character & 112) >> 4];
                        TextPallete[0] = pScreen->Pallete32Bit[8];
                        Character = 64 + (Character & 0xF);
                        Pixel = ntsc_round_fontdata8x12[(Character + 0) * 12 + chr];
                        break;
                        
                }
                
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>7)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>7)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>6)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>6)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>5)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>5)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>4)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>4)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>3)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>3)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>2)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>2)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>1)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>1)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>0)&1];
                dstRowPtr[dstX+=1]=TextPallete[(Pixel>>0)&1];

            } // Next pGIME->HorzBeam
        }
        break;
            
        // UNTESTED
        case 128+0: //Bpp=0 Sr=0 1BPP Stretch=1
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //1bbp Stretch=1
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
            }
        }
        break;
            
        // UNTESTED
        //case 192+1:
        case 128+1: //Bpp=0 Sr=1 1BPP Stretch=2
        case 128+2:    //Bpp=0 Sr=2
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //1bbp Stretch=2
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
            }
        }
            break;
            
        // UNTESTED
        case 128+3: //Bpp=0 Sr=3 1BPP Stretch=4
        case 128+4: //Bpp=0 Sr=4
        case 128+5: //Bpp=0 Sr=5
        case 128+6: //Bpp=0 Sr=6
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //1bbp Stretch=4
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
            }
        }
            break;
            
            // UNTESTED
        case 128+7: //Bpp=0 Sr=7 1BPP Stretch=8
        case 128+8: //Bpp=0 Sr=8
        case 128+9: //Bpp=0 Sr=9
        case 128+10: //Bpp=0 Sr=10
        case 128+11: //Bpp=0 Sr=11
        case 128+12: //Bpp=0 Sr=12
        case 128+13: //Bpp=0 Sr=13
        case 128+14: //Bpp=0 Sr=14
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //1bbp Stretch=8
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>7)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>5)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>3)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>1)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>15)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>13)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>11)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>9)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 1 & (WidePixel>>8)];
            }
        }
            break;
            
            // UNTESTED
        case 128+15: //Bpp=0 Sr=15 1BPP Stretch=16
        case 128+16: //BPP=1 Sr=0  2BPP Stretch=1
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //2bbp Stretch=1
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
            }
        }
            break;
            
            // UNTESTED
        case 128+17: //Bpp=1 Sr=1  2BPP Stretch=2
        case 128+18: //Bpp=1 Sr=2
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //2bbp Stretch=2
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
            }
        }
            break;
            
            // UNTESTED
        case 128+19: //Bpp=1 Sr=3  2BPP Stretch=4
        case 128+20: //Bpp=1 Sr=4
        case 128+21: //Bpp=1 Sr=5
        case 128+22: //Bpp=1 Sr=6
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //2bbp Stretch=4
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
            }
        }
        break;
            
            // UNTESTED
        case 128+23: //Bpp=1 Sr=7  2BPP Stretch=8
        case 128+24: //Bpp=1 Sr=8
        case 128+25: //Bpp=1 Sr=9
        case 128+26: //Bpp=1 Sr=10
        case 128+27: //Bpp=1 Sr=11
        case 128+28: //Bpp=1 Sr=12
        case 128+29: //Bpp=1 Sr=13
        case 128+30: //Bpp=1 Sr=14
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //2bbp Stretch=8
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
            }
        }
        break;
            
        // UNTESTED
        case 128+31: //Bpp=1 Sr=15 2BPP Stretch=16
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //2bbp Stretch=16
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>6)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>2)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>14)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>10)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 3 & (WidePixel>>8)];
            }
        }
            break;
            
        case 128+32: //Bpp=2 Sr=0 4BPP Stretch=1
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //4bbp Stretch=1
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam)))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>0)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
            }
        }
        break;
          
        case 128+33: //Bpp=2 Sr=1 4BPP Stretch=2
        case 128+34: //Bpp=2 Sr=2
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //4bbp Stretch=2
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam)))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>0)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>0)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
            }
        }
        break;
            
        case 128+35: //Bpp=2 Sr=3 4BPP Stretch=4
        case 128+36: //Bpp=2 Sr=4
        case 128+37: //Bpp=2 Sr=5
        case 128+38: //Bpp=2 Sr=6
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //4bbp Stretch=4
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
            }
        }
            break;
            
        case 128+39: //Bpp=2 Sr=7 4BPP Stretch=8
        case 128+40: //Bpp=2 Sr=8
        case 128+41: //Bpp=2 Sr=9
        case 128+42: //Bpp=2 Sr=10
        case 128+43: //Bpp=2 Sr=11
        case 128+44: //Bpp=2 Sr=12
        case 128+45: //Bpp=2 Sr=13
        case 128+46: //Bpp=2 Sr=14
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //4bbp Stretch=8
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
            }
        }
            break;
            
        case 128+47: //Bpp=2 Sr=15 4BPP Stretch=16
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //4bbp Stretch=16
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>4)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & WidePixel];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>12)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[ 15 & (WidePixel>>8)];
            }
        }
            break;
            
//        case 128+48: //Bpp=3 Sr=0  Unsupported
//        case 128+49: //Bpp=3 Sr=1
//        case 128+50: //Bpp=3 Sr=2
//        case 128+51: //Bpp=3 Sr=3
//        case 128+52: //Bpp=3 Sr=4
//        case 128+53: //Bpp=3 Sr=5
//        case 128+54: //Bpp=3 Sr=6
//        case 128+55: //Bpp=3 Sr=7
//        case 128+56: //Bpp=3 Sr=8
//        case 128+57: //Bpp=3 Sr=9
//        case 128+58: //Bpp=3 Sr=10
//        case 128+59: //Bpp=3 Sr=11
//        case 128+60: //Bpp=3 Sr=12
//        case 128+61: //Bpp=3 Sr=13
//        case 128+62: //Bpp=3 Sr=14
//        case 128+63: //Bpp=3 Sr=15
//
//            return;
//            break;
            
        case 192+0: //Bpp=0 Sr=0 1BPP Stretch=1
        {
            // TESTED
            // TODO: artifact colours are not quite right - Clowns & Balloons, Project Nebula
            // CMP and RGB - MegaBug, Clowns & Baloons
            
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            char            Sphase=0;
            char            Pix=0;
            char            Bit=0;
            char            Pcolor=0;
            char            Carry1=0;
            char            Carry2=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //1bbp Stretch=1
            {
                WidePixel = WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                
                switch ( pScreen->MonType )
                {
                    case eMonType_RGB:
                    {
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                        dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
                    }
                    break;
                      
                    case eMonType_CMP:
                    {
                        // swap bytes
                        WidePixel = ((WidePixel>>8)&0x00FF) | ((WidePixel<<8)&0xFF00);
                        
                        // artifact colours
                        for (Bit=15;Bit>=0;Bit--)
                        {
                            Pix = (1 & (WidePixel>>Bit) );
                            Sphase = (Carry2<<2) | (Carry1<<1) | Pix;
                            switch(Sphase)
                            {
                                case 0:
                                case 4:
                                case 6:
                                    Pcolor=0;
                                    break;
                                case 1:
                                case 5:
                                    Pcolor=(Bit &1)+1;
                                    break;
                                case 2:
                                        //Pcolor=(!(Bit &1))+1; // Use last color
                                    break;
                                case 3:
                                    Pcolor=3;
                                    if ( dstX > 1 )
                                        dstRowPtr[dstX-1] = Afacts32[pScreen->ColorInvert][3];
                                    if ( dstX > 0 )
                                        dstRowPtr[dstX] = Afacts32[pScreen->ColorInvert][3];
                                    break;
                                case 7:
                                    Pcolor=3;
                                    break;
                            } //END Switch
                            
                            dstRowPtr[dstX+=1] = Afacts32[pScreen->ColorInvert][Pcolor];
                            Carry2 = Carry1;
                            Carry1 = Pix;
                        }
                    }
                    break;
                        
                    default:
                        // unsupported monitor type
                        break;
                }
            }
        }
            break;
            
        case 192+1: //Bpp=0 Sr=1 1BPP Stretch=2
        case 192+2:    //Bpp=0 Sr=2
        {
            uint16_t *      WideBuffer = (uint16_t *)RamBuffer;
            unsigned short  WidePixel=0;
            char            Sphase=0;
            char            Pix=0;
            char            Bit=0;
            char            Pcolor=0;
            char            Carry1=0;
            char            Carry2=0;
            
            Carry1 = 1;

            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //1bbp Stretch=2
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];

                if (pScreen->MonType == eMonType_CMP)
                {
                    // TODO: crashes during Mega bug attract mode
                    
                    //Pcolor
                    for (Bit=7;Bit>=0;Bit--)
                    {
                        Pix=(1 & (WidePixel>>Bit) );
                        Sphase= (Carry2<<2)|(Carry1<<1)|Pix;
                        switch(Sphase)
                        {
                            case 0:
                            case 4:
                            case 6:
                                Pcolor=0;
                                break;
                            case 1:
                            case 5:
                                Pcolor=(Bit &1)+1;
                                break;
                            case 2:
                                //    Pcolor=(!(Bit &1))+1; Use last color
                                break;
                            case 3:
                                Pcolor=3;
                                dstRowPtr[dstX-1] = Afacts32[pScreen->ColorInvert][3];
                                dstRowPtr[dstX] = Afacts32[pScreen->ColorInvert][3];
                                break;
                            case 7:
                                Pcolor=3;
                                break;
                        } //END Switch
                        
                        dstRowPtr[dstX+=1] = Afacts32[pScreen->ColorInvert][Pcolor];
                        dstRowPtr[dstX+=1] = Afacts32[pScreen->ColorInvert][Pcolor];
                        Carry2=Carry1;
                        Carry1=Pix;
                    }
                    
                    for (Bit=15;Bit>=8;Bit--)
                    {
                        Pix=(1 & (WidePixel>>Bit) );
                        Sphase= (Carry2<<2)|(Carry1<<1)|Pix;
                        switch(Sphase)
                        {
                            case 0:
                            case 4:
                            case 6:
                                Pcolor=0;
                                break;
                            case 1:
                            case 5:
                                Pcolor=(Bit &1)+1;
                                break;
                            case 2:
                                //    Pcolor=(!(Bit &1))+1; Use last color
                                break;
                            case 3:
                                Pcolor=3;
                                dstRowPtr[dstX-1]=Afacts32[pScreen->ColorInvert][3];
                                dstRowPtr[dstX]=Afacts32[pScreen->ColorInvert][3];
                                break;
                            case 7:
                                Pcolor=3;
                                break;
                        } //END Switch
                        
                        dstRowPtr[dstX+=1]=Afacts32[pScreen->ColorInvert][Pcolor];
                        dstRowPtr[dstX+=1]=Afacts32[pScreen->ColorInvert][Pcolor];
                        Carry2=Carry1;
                        Carry1=Pix;
                    }
                    
                }
                else // pGIME->MonType== RGB
                {
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
                    dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
                }
            }
        }
            break;
            
        case 192+3: //Bpp=0 Sr=3 1BPP Stretch=4
        case 192+4: //Bpp=0 Sr=4
        case 192+5: //Bpp=0 Sr=5
        case 192+6: //Bpp=0 Sr=6
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //1bbp Stretch=4
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
            }
        }
            break;
            
        case 192+7: //Bpp=0 Sr=7 1BPP Stretch=8
        case 192+8: //Bpp=0 Sr=8
        case 192+9: //Bpp=0 Sr=9
        case 192+10: //Bpp=0 Sr=10
        case 192+11: //Bpp=0 Sr=11
        case 192+12: //Bpp=0 Sr=12
        case 192+13: //Bpp=0 Sr=13
        case 192+14: //Bpp=0 Sr=14
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //1bbp Stretch=8
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>7))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>5))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>3))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>1))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>15))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>13))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>11))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>9))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 1 & (WidePixel>>8))];
            }
        }
            break;
            
        case 192+15: //Bpp=0 Sr=15 1BPP Stretch=16
        case 192+16: //BPP=1 Sr=0  2BPP Stretch=1
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //2bbp Stretch=1
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
            }
        }
            break;
            
        case 192+17: //Bpp=1 Sr=1  2BPP Stretch=2
        case 192+18: //Bpp=1 Sr=2
        {
            uint16_t *      WideBuffer = (uint16_t *)RamBuffer;
            unsigned short  WidePixel=0;
            uint32_t        pixel;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //2bbp Stretch=2
            {
                WidePixel = WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                
                pixel = pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pixel;
                //dstRowPtr[dstX+=1]=pixel;
                pixel = pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pixel;
                ////dstRowPtr[dstX+=1]=pixel;
                pixel = pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pixel;
                //dstRowPtr[dstX+=1]=pixel;
                pixel = pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>0))];
                dstRowPtr[dstX+=1]=pixel;
                //dstRowPtr[dstX+=1]=pixel;
            
                pixel = pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pixel;
                //dstRowPtr[dstX+=1]=pixel;
                pixel = pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pixel;
                //dstRowPtr[dstX+=1]=pixel;
                pixel = pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pixel;
                //dstRowPtr[dstX+=1]=pixel;
                pixel = pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pixel;
                //dstRowPtr[dstX+=1]=pixel;
            }
        }
        break;
            
        case 192+19: //Bpp=1 Sr=3  2BPP Stretch=4
        case 192+20: //Bpp=1 Sr=4
        case 192+21: //Bpp=1 Sr=5
        case 192+22: //Bpp=1 Sr=6
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            uint32_t p;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //2bbp Stretch=4
            {
                WidePixel = WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                
                p = pScreen->Pallete32Bit[pScreen->PalleteIndex+((WidePixel>>6) & 3)];
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                
                p = pScreen->Pallete32Bit[pScreen->PalleteIndex+((WidePixel>>4) & 3)];
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                
                p = pScreen->Pallete32Bit[pScreen->PalleteIndex+((WidePixel>>2) & 3)];
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                
                p = pScreen->Pallete32Bit[pScreen->PalleteIndex+((WidePixel>>0) & 3)];
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                
                p = pScreen->Pallete32Bit[pScreen->PalleteIndex+((WidePixel>>14) & 3)];
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                
                p = pScreen->Pallete32Bit[pScreen->PalleteIndex+((WidePixel>>12) & 3)];
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                
                p = pScreen->Pallete32Bit[pScreen->PalleteIndex+((WidePixel>>10) & 3)];
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                
                p = pScreen->Pallete32Bit[pScreen->PalleteIndex+((WidePixel>>8) & 3)];
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
                dstRowPtr[dstX+=1]=p;
            }
        }
        break;
            
        case 192+23: //Bpp=1 Sr=7  2BPP Stretch=8
        case 192+24: //Bpp=1 Sr=8
        case 192+25: //Bpp=1 Sr=9
        case 192+26: //Bpp=1 Sr=10
        case 192+27: //Bpp=1 Sr=11
        case 192+28: //Bpp=1 Sr=12
        case 192+29: //Bpp=1 Sr=13
        case 192+30: //Bpp=1 Sr=14
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //2bbp Stretch=8
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
            }
        }
            break;
            
        case 192+31: //Bpp=1 Sr=15 2BPP Stretch=16
        {
            uint16_t * WideBuffer = (uint16_t *)RamBuffer;
            unsigned short            WidePixel=0;
            
            for (int HorzBeam=0; HorzBeam<pScreen->BytesperRow; HorzBeam+=2) //2bbp Stretch=16
            {
                WidePixel=WideBuffer[(pGIME->iVidMask & (Start+(unsigned char)(rowLatch->hOffset+HorzBeam) ))>>1];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>6))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>4))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>2))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & WidePixel)];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>14))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>12))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>10))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
                dstRowPtr[dstX+=1]=pScreen->Pallete32Bit[pScreen->PalleteIndex+( 3 & (WidePixel>>8))];
            }
        }
            break;
            
//        case 192+32: //Bpp=2 Sr=0 4BPP Stretch=1 Unsupport with Compat set
//        case 192+33: //Bpp=2 Sr=1 4BPP Stretch=2
//        case 192+34: //Bpp=2 Sr=2
//        case 192+35: //Bpp=2 Sr=3 4BPP Stretch=4
//        case 192+36: //Bpp=2 Sr=4
//        case 192+37: //Bpp=2 Sr=5
//        case 192+38: //Bpp=2 Sr=6
//        case 192+39: //Bpp=2 Sr=7 4BPP Stretch=8
//        case 192+40: //Bpp=2 Sr=8
//        case 192+41: //Bpp=2 Sr=9
//        case 192+42: //Bpp=2 Sr=10
//        case 192+43: //Bpp=2 Sr=11
//        case 192+44: //Bpp=2 Sr=12
//        case 192+45: //Bpp=2 Sr=13
//        case 192+46: //Bpp=2 Sr=14
//        case 192+47: //Bpp=2 Sr=15 4BPP Stretch=16
//        case 192+48: //Bpp=3 Sr=0  Unsupported
//        case 192+49: //Bpp=3 Sr=1
//        case 192+50: //Bpp=3 Sr=2
//        case 192+51: //Bpp=3 Sr=3
//        case 192+52: //Bpp=3 Sr=4
//        case 192+53: //Bpp=3 Sr=5
//        case 192+54: //Bpp=3 Sr=6
//        case 192+55: //Bpp=3 Sr=7
//        case 192+56: //Bpp=3 Sr=8
//        case 192+57: //Bpp=3 Sr=9
//        case 192+58: //Bpp=3 Sr=10
//        case 192+59: //Bpp=3 Sr=11
//        case 192+60: //Bpp=3 Sr=12
//        case 192+61: //Bpp=3 Sr=13
//        case 192+62: //Bpp=3 Sr=14
//        case 192+63: //Bpp=3 Sr=15
//            return;
//            break;
            
        default:
            break;
            
    } //END SWITCH
    
    // double up the line
    dstRowPtr -= rowLatch->horzCenter;
    if ( pScreen->confScanLines )
    {
        // scan lines are one, just clear the line
        memset(dstRowPtr+pScreen->surface.width,0,pScreen->surface.linePitch);
    }
    else
    {
        // copy the current line down one line
        memcpy(dstRowPtr+pScreen->surface.width,dstRowPtr,pScreen->surface.linePitch);
    }
}

/********************************************************************************/


