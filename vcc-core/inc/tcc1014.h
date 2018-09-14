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

#ifndef _tcc1014_h_
#define _tcc1014_h_

/**************************************************************************/

#include "cpu.h"
#include "rom.h"

// http://www.cocopedia.com/wiki/index.php/TRS-80_Color_Computer#Hardware_Design_and_Integrated_Circuits

/********************************************************************************/

#pragma mark -
#pragma mark --- constants ---

#define COLORBURST ((double)3579545)
#define CPU_DEFAULT_MHZ ((COLORBURST/4)/1000000)
#define PICOSECOND 1000000000
#define FRAMESPERSECOND (double)59.923

// TODO: convert these to calculated at run-time
#define TOTAL_LINESPERSCREEN 525                // NTSC 525 lines
#define VISIBLE_LINESPERSCREEN 480                // NTSC 480 visible lines
#define CyclesPerSecond (COLORBURST/4)
#define LinesPerSecond (FRAMESPERSECOND * TOTAL_LINESPERSCREEN)
#define PicosPerLine (PICOSECOND / LinesPerSecond)
#define PicosPerColorBurst (PICOSECOND/COLORBURST)
#define CyclesPerLine (CyclesPerSecond / LinesPerSecond)

/**************************************************************************/

/*
	CPU 'port' read/write callback function definitions
 */
typedef uint8_t (*portreadfn_t)(emudevice_t * pEmuDevice, uint8_t port);
typedef void (*portwritefn_t)(emudevice_t * pEmuDevice, uint8_t data, uint8_t port);

/*
	port read/write entries for registration
 */
typedef struct portreadentry_t portreadentry_t;
struct portreadentry_t
{
	void *			pEmuDevice;
	portreadfn_t	pfnReadPort;
};

typedef struct portwriteentry_t portwriteentry_t;
struct portwriteentry_t
{
	void *			pEmuDevice;
	portwritefn_t	pfnWritePort;
};

/**************************************************************************/

#define VCC_GIME_ID	XFOURCC('g','i','m','e')

/********************************************************************************/

#if (defined DEBUG)
#define ASSERT_GIME(pGIME)	assert(pGIME != NULL);	\
							assert(pGIME->mmu.device.id == EMU_DEVICE_ID); \
                            assert(pGIME->mmu.device.idModule == EMU_MMU_ID); \
							assert(pGIME->mmu.device.idDevice == VCC_GIME_ID)
#else
#define ASSERT_GIME(pGIME)
#endif

/*****************************************************************************/
/**
    Monitor types
 */
typedef enum montype_e
{
    eMonType_CMP = 0,   // composite
    eMonType_RGB,       // rgb
    
    eMonType_Count
} montype_e;

//
typedef enum ramconfig_e
{
    Ram128k = 0,
    Ram512k,
    Ram1024k,
    Ram2048k,
    Ram8192k,
    
    RamNumConfigs
} ramconfig_e;

typedef enum gimeirqtype_e
{
    irqKeyboard     = (1 << 1), // 2
    irqVertical     = (1 << 3), // 8
    irqHorizontal   = (1 << 4), // 16
    irqTimer        = (1 << 5)  // 32
} gimeirqtype_e;

/**************************************************************************/
/*
	GIME chip object
 */
typedef struct gime_t gime_t;
struct gime_t
{
	mmu_t               mmu;
	
    unsigned char       GimeRegisters[256];

    /*
        Config
     */
    ramconfig_e         CurrentRamConfig;       // RAM configuration/size

	/*
		mmu
	 */
    uint8_t *		    MemPages[1024];
	int		            MemPageOffsets[1024];
	uint8_t *		    memory;					// Emulated RAM
    rom_t               rom;                    // Internal CoCo3 ROM
    
	unsigned char		MmuTask;				// $FF91 bit 0
	unsigned char		MmuEnabled;				// $FF90 bit 6
	unsigned char		RamVectors;				// $FF90 bit 3
    
	unsigned char		MmuState;				// Composite variable handles MmuTask and MmuEnabled
	unsigned char		RomMap;					// $FF90 bit 1-0
	unsigned char		MapType;				// $FFDE/FFDF toggle Map type 0 = ram/rom
	unsigned short		MmuRegisters[4][8];		// $FFA0 - FFAF
	unsigned short		MmuPrefix;
    
    unsigned int        iVidMask;
    unsigned int        DistoOffset;

	/*
		port read/write registration
	 */
	portreadentry_t		pDefaultPortRead;
	portwriteentry_t	pDefaultPortWrite;
	
	portreadentry_t		portReadTable[256];
	portwriteentry_t	portWriteTable[256];
	
    /*
        timing
     */
    // CPU current settings
    int                 CpuRate;                /**< Current CPU Rate - 0 or 1 - 0.89MHz or double */

    /*
     IRQ
     */
    bool               EnhancedFIRQFlag;
    bool               EnhancedIRQFlag;
    bool               HorzInterruptEnabled;
    bool               VertInterruptEnabled;
    bool               TimerInterruptEnabled;
    bool               KeyboardInterruptEnabled;
    
    gimeirqtype_e      LastIrq;    // actually multiple merged together
    gimeirqtype_e      LastFirq;
    
    /*
        Timing
     */
    //int                 IntEnable;
    unsigned short      TimerClockRate;         // current timer rate - 0 = 63.695uS  (1/60*262)  1 scanline time, 1 = 279.265nS (1/ColorBurst)
    unsigned short      UnxlatedTickCounter;    // value of the timer register
    unsigned short      MasterTickCounter;      // current value of the count down

    /*
	 graphics
	 */    
    uint8_t             Pallete[16];        // Coco 3 6 bit colors

    unsigned short      VerticalOffsetRegister;
    unsigned char       VerticalScrollRegister;
    
    unsigned char       HorzOffsetReg;      // unsigned char
    
	unsigned char		LowerCase;          // unsigned char
	unsigned char		InvertAll;          // unsigned char
	unsigned char		ExtendedText;       // unsigned char
	unsigned char		Hoffset;            // unsigned char
    
    unsigned char       Dis_Offset;             // unsigned char
    int                 VDG_Mode;

    unsigned char       CompatMode;         // unsigned char

    unsigned char        CC2Offset;
    unsigned char        CC2VDGMode;
    unsigned char        CC2VDGPiaMode;
    
    unsigned char        CC3Vmode;
    unsigned char        CC3Vres;
    unsigned char        CC3BorderColor;

    int                 preLatchRow;
};

/**************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
    gime_t *	gimeCreate(void);
	
    // TODO: should not need to be exposed
	void		gimeReset(gime_t * pGIME);
		
    int         gimeGetCpuRate(gime_t * pGIME);
    
    void        gimeSetKeyboardInterruptState(gime_t * pGIME, bool);

#ifdef __cplusplus
}
#endif

/**************************************************************************/

#endif // _tcc1014_h_
