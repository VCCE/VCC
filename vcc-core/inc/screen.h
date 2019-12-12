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

#ifndef _screen_h_
#define _screen_h_

/*****************************************************************************/

#include "tcc1014.h"
#include "emuDevice.h"

/*****************************************************************************/

typedef void (*emudisplaycb_t)(void *);

/*****************************************************************************/

#define CONF_SECTION_VIDEO			"Video"

#define CONF_SETTING_MONITORTYPE	"MonitorType"
#define CONF_SETTING_SCANLINES		"ScanLines"
//#define CONF_SETTING_FRAMESKIP		"FrameSkip"

/*****************************************************************************/

#define SCREEN_COMMAND_NONE			0
#define SCREEN_COMMAND_NOP			0
#define SCREEN_COMMAND_FULLSCREEN	2
#define SCREEN_COMMAND_SCANLINES	3
#define SCREEN_COMMAND_SCREEN_SHOT  4
#define SCREEN_COMMAND_MONTYPE      5 // + monitor types

/*****************************************************************************/

#define VCC_SCREEN_ID	XFOURCC('m','s','c','n')

#if (defined DEBUG)
#define ASSERT_SCREEN(screen) \
        assert(screen != NULL);    \
        assert(screen->device.id == EMU_DEVICE_ID); \
        assert(screen->device.idModule == EMU_DEVICE_ID); \
        assert(screen->device.idDevice == VCC_SCREEN_ID)
#else
    #define ASSERT_SCREEN(screen)
#endif

/*****************************************************************************/

typedef struct surface_t
{
    int         width;
    int         height;
    int         bitDepth;

    uint8_t *   pBuffer;
    uint32_t    linePitch;
} surface_t;

typedef struct rowlatch_t
{
    uint32_t            borderColor;
    int                 compatMode;
    int                 graphicsMode;
    int                 hOffset;
    int                 stretch;        // horizontal stretch
    int                 horzCenter;
    int                 extendedText;
    int                 lowerCase;
    
    int                 startofVidram;      // offset into the graphics memory
    int                 newStartofVidram;   // ???
    int                 srcRow;
} rowlatch_t;

typedef struct screen_t
{
	emudevice_t			device;

    gime_t *            pGIME;              // reference for convenience only
    
	/*
		config / persistence
	 */
	bool 				confFullScreen;
    int 				confMonitorType;    // type of monitor displaying to
	bool 				confScanLines;      // scan lines effect (don't draw every other line
    
	/*
		ui callbacks
	 */
	void *				pScreenCallbackParam;
	//emudisplaycb_t	pScreenInitCallback;
	emudisplaycb_t		pScreenFlipCallback;
	emudisplaycb_t		pScreenLockCallback;
	emudisplaycb_t		pScreenUnlockCallback;
    emudisplaycb_t      pScreenFullScreenCallback;

    /*
     */
	int 				WindowSizeX;    // size of the window
	int 				WindowSizeY;

    surface_t           surface;        // screen buffer descriptor (video texture)
    
    montype_e           MonType;        // Monitor type

    //uint8_t           Pallete8Bit[16];
    //uint16_t          Pallete16Bit[16];    // Color values translated to 16 bit
    uint32_t            Pallete32Bit[16];    // Color values translated to 32 bit
    
    // color look up for each monitor type - RGB=0, 1=CMP
    //unsigned char     PalleteLookup8[2][64];
    //unsigned short    PalleteLookup16[2][64];
    uint32_t            PalleteLookup32[eMonType_Count][64];
    
    int                 GraphicsMode;       // unsigned char
    
    int                 VresIndex;          // unsigned char
    
    int                 ColorInvert;
    int                 BlinkState;
    
    // TODO: put in frame latch struct (all?)
    int                 TextFGPallete;
    int                 TextBGPallete;
    int                 PalleteIndex;
    int                 TopBorder;          // height of top border
    int                 LinesperScreen;     // unsigned char
    int                 LinesperRow;        // unsigned char
    int                 Bpp;                // unsigned char
    int                 BytesperRow;        // unsigned char
    int                 PixelsperLine;
    int                 VPitch;
    int                 PixelsperByte;      // unsigned char

    rowlatch_t          rowLatch;
} screen_t;

/*****************************************************************************/

#ifdef __cplusplus
extern"C"
{
#endif

    // TODO: there should be no need for these
    
	extern const unsigned short Afacts16[2][4];
	//extern const unsigned char Afacts8[2][4];
	extern const unsigned char Afacts8[2][4];
	extern const unsigned int Afacts32[2][4];
	
	extern const unsigned char ntsc_round_fontdata8x12[];
	extern const unsigned char cc3Fontdata8x12[];
	
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#ifdef __cplusplus
extern"C"
{
#endif
	
    bool            surfaceInit(surface_t * surface, int width, int height, int bitDepth);
    void            surfaceFreeStorage(surface_t * surface);
    uint8_t *       surfaceGetLinePtr(surface_t * surface, int row);
    void            surfaceFill(surface_t * surface, uint32_t color);

    screen_t *		screenCreate(void);
	
	int 			screenLock(screen_t * pScreen);
	void			screenUnlock(screen_t * pScreen);
	void			screenDisplayFlip(screen_t * pScreen);
	void			screenClearBuffer(screen_t * pScreen);
	void			screenStatic(screen_t * pScreen);

	result_t		screenEmuDevCommand(emudevice_t * pEmuDev, int iCommand, int iParam);

    void SetMonitorType(screen_t * pScreen, montype_e monType);
    int GetMonitorType(screen_t * pScreen);

    void InitDisplay(screen_t * pScreen);
    void SetupDisplay(screen_t * pScreen);
    void screenUpdateBorderColor(screen_t *pScreen);
    void screenUpdatePalette(screen_t * pScreen);

    /*
     for drawing video from emulator to an off-screen buffer
     */
    void screenUpdate(screen_t *, int);
    void screenUpdateBottomBorder(screen_t * pScreen, int borderLine);
    void screenUpdateTopBorder(screen_t * pScreen, int borderLine);
    
#ifdef __cplusplus
}
#endif
	
/*****************************************************************************/

#endif

