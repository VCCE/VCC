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
	TODO: optimize memory read/write
    TODO: move ROM path from coco3 config to here
    TODO: ROM should be re-loaded on hard reset?
*/
/****************************************************************************/

#include "tcc1014mmu.h"
#include "tcc1014graphics.h"
#include "tcc1014registers.h"
#include "coco3.h"
#include "pakinterface.h"
#include "file.h"

#include "xDebug.h"
#include "xSystem.h"

/********************************************************************/

unsigned char mmuMemRead8(emudevice_t * pEmuDevice, int);
void		  mmuMemWrite8(emudevice_t * pEmuDevice, unsigned char, int);

/********************************************************************/
/*
	0 - 128k
	1 - 512k
	2 - 1M		- TEST
	3 - 2M
	4 - 8M		- TEST - CoCoZilla
 */
//												               128k        512k        1M        2M        8M
const unsigned int		MemConfig[RamNumConfigs]		= { 0x20000,	0x80000, 0x100000, 0x200000, 0x800000 };
const unsigned short	RamMask[RamNumConfigs]			= { 	 15,		 63,	  127,      255,     1023 };
const unsigned char		StateSwitch[RamNumConfigs]		= {       8,         56,       56,       56,       56 };
const unsigned char		VectorMask[RamNumConfigs]		= {      15,         63,       63,       63,       63 };
const unsigned char		VectorMaska[RamNumConfigs]		= {      12,         60,       60,       60,       60 };
const unsigned int		VidMask[RamNumConfigs]			= { 0x1FFFF,    0x7FFFF, 0x1FFFFF, 0x3FFFFF, 0x7FFFFF };

/********************************************************************/
/********************************************************************/
/**
    MmuInit Initilize and allocate memory for RAM Internal and External ROM Images.
    Copy Rom Images to buffer space and reset GIME MMU registers to 0
    Returns NULL if any of the above fail.
 
    This must be called before any other port registration on hard reset
*/
void mmuInit(gime_t * pGIME, ramconfig_e RamConfig, const char * pPathname)
{
	unsigned int	RamSize=0;
	unsigned int	Index1=0;
	
	ASSERT_GIME(pGIME);
	
	// clear port read/write functions
	portInit(pGIME);
	// register port i/o read functions
	portRegisterRead(pGIME,gimeRead,&pGIME->mmu.device,0xFF90,0xFFBF);		// GIME
	portRegisterRead(pGIME,samRead,&pGIME->mmu.device,0xFFC0,0xFFDF);		// MC6883 S.A.M.
	portRegisterRead(pGIME,samRead,&pGIME->mmu.device,0xFFF0,0xFFFF);		// vectors handled by S.A.M.
	// register port i/o write functions
	portRegisterWrite(pGIME,gimeWrite,&pGIME->mmu.device,0xFF90,0xFFBF);	// GIME
	portRegisterWrite(pGIME,samWrite,&pGIME->mmu.device,0xFFC0,0xFFDF);		// MC6883 S.A.M.
	
    // set mem read/write functions
    pGIME->mmu.pMemRead8	= mmuMemRead8;
    pGIME->mmu.pMemWrite8	= mmuMemWrite8;

    RamSize = MemConfig[RamConfig];
	pGIME->CurrentRamConfig = RamConfig;
	
	pGIME->MmuState = 0;
	
	/* re-allocate memory */
	if ( pGIME->memory != NULL )
	{
		free(pGIME->memory);
	}
	pGIME->memory = (unsigned char *)calloc(1,RamSize);
	if ( pGIME->memory == NULL )
	{
        // TODO: handle out of memory condition properly
		assert(false);
		return;
	}
	for (Index1=0;Index1<RamSize;Index1++)
	{
		if (Index1 & 1)
			pGIME->memory[Index1]=0;
		else 
			pGIME->memory[Index1]=0xFF;
	}
    
	pGIME->iVidMask = VidMask[pGIME->CurrentRamConfig];
	
    // load the main ROM
    result_t romLoadResult = XERROR_GENERIC;
    if ( pPathname != NULL )
    {
        // try the user specified path
        romLoadResult = romLoad(&pGIME->rom,pPathname);
    }
    if ( romLoadResult != XERROR_NONE )
    {
        // try from the app's resources folder
        char temp[PATH_MAX];
        sysGetPathForResource("coco3.rom",temp,sizeof(temp));
        romLoadResult = romLoad(&pGIME->rom,temp);
    }
    if ( romLoadResult == XERROR_NONE )
    {
        emuDevLog(&pGIME->mmu.device, "CoCo3 ROM loaded successfully");
    }
    else
    {
        //#define COCO3_ROM_SIZE    0x8000
        //romSet(&pGIME->rom, (uint8_t *)CC3Rom, COCO3_ROM_SIZE);

        // this is fatal!
        emuDevLog(&pGIME->mmu.device, "CoCo3 ROM loaded FAILED");
        // TODO: handle this properly
        exit(1);
    }

	mmuReset(pGIME);
}

/********************************************************************/
/**
 */
unsigned char * mmuGetRAMBuffer(gime_t * pGIME)
{
	assert(pGIME != NULL);
	
	return pGIME->memory;
}

/********************************************************************/
/**
 */
void mmuReset(gime_t * pGIME)
{
	unsigned int Index1=0,Index2=0;

	assert(pGIME != NULL);
	
	pGIME->MmuTask=0;
	pGIME->MmuEnabled=0;
	pGIME->RamVectors=0;
	pGIME->MmuState=0;
	pGIME->RomMap=0;
	pGIME->MapType=0;
	pGIME->MmuPrefix=0;
    
	for (Index1=0;Index1<8;Index1++)
		for (Index2=0;Index2<4;Index2++)
			pGIME->MmuRegisters[Index2][Index1] = Index1 + StateSwitch[pGIME->CurrentRamConfig];

	for (Index1=0;Index1<1024;Index1++)
	{
		pGIME->MemPages[Index1] = pGIME->memory + ( (Index1 & RamMask[pGIME->CurrentRamConfig]) * 0x2000);
		pGIME->MemPageOffsets[Index1] = 1;
	}
	mmuSetRomMap(pGIME,0);
	mmuSetMapType(pGIME,0);
}

/********************************************************************/
/**
 */
void mmuSetVectors(gime_t * pGIME, unsigned char data)
{
	assert(pGIME != NULL);
	
	pGIME->RamVectors = !!data; //Bit 3 of $FF90 MC3
}

/********************************************************************/
/**
 */
void mmuSetRegister(gime_t * pGIME, unsigned char Register,unsigned char data)
{	
	unsigned char BankRegister,Task;

	assert(pGIME != NULL);
	
	BankRegister = Register & 7;
	Task=!!(Register & 8);
	pGIME->MmuRegisters[Task][BankRegister] = pGIME->MmuPrefix |(data & RamMask[pGIME->CurrentRamConfig]); //gime.c returns what was written so I can get away with this
}

/********************************************************************/
/**
 */
void mmuUpdateArray(gime_t * pGIME)
{
	assert(pGIME != NULL);
	
	if (pGIME->MapType)
	{
		pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]-3] = pGIME->memory + (0x2000*(VectorMask[pGIME->CurrentRamConfig]-3));
		pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]-2] = pGIME->memory + (0x2000*(VectorMask[pGIME->CurrentRamConfig]-2));
		pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]-1] = pGIME->memory + (0x2000*(VectorMask[pGIME->CurrentRamConfig]-1));
		pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]-0] = pGIME->memory + (0x2000*(VectorMask[pGIME->CurrentRamConfig]-0));
		
		pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]-3] = 1;
		pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]-2] = 1;
		pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]-1] = 1;
		pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]-0] = 1;
		return;
	}
    
	switch (pGIME->RomMap)
	{
		case 0:
		case 1:	//16K Internal 16K External
			pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]-3]=pGIME->rom.pData;
			pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]-2]=pGIME->rom.pData+0x2000;
			pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]-1]=NULL;
			pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]]=NULL;
			
			pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]-3]=1;
			pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]-2]=1;
			pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]-1]=0;
			pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]]=0x2000;
			return;
			break;
			
		case 2:	// 32K Internal
			pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]-3]=pGIME->rom.pData;
			pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]-2]=pGIME->rom.pData+0x2000;
			pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]-1]=pGIME->rom.pData+0x4000;
			pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]]=pGIME->rom.pData+0x6000;
			
			pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]-3]=1;
			pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]-2]=1;
			pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]-1]=1;
			pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]]=1;
			return;
			break;
			
		case 3:	//32K External
			pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]-1]=NULL;
			pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]]=NULL;
			pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]-3]=NULL;
			pGIME->MemPages[VectorMask[pGIME->CurrentRamConfig]-2]=NULL;
			
			pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]-1]=0;
			pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]]=0x2000;
			pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]-3]=0x4000;
			pGIME->MemPageOffsets[VectorMask[pGIME->CurrentRamConfig]-2]=0x6000;
			return;
			break;
	}
	return;
}

/********************************************************************/
/**
 */
void mmuSetRomMap(gime_t * pGIME, unsigned char data)
{	
	assert(pGIME != NULL);
	
	pGIME->RomMap=(data & 3);
	mmuUpdateArray(pGIME);
}

/********************************************************************/
/**
 */
void mmuSetMapType(gime_t * pGIME, unsigned char type)
{
	assert(pGIME != NULL);
	
	pGIME->MapType=type;
	mmuUpdateArray(pGIME);
}

/********************************************************************/
/**
 */
void mmuSetTask(gime_t * pGIME, unsigned char task)
{
	assert(pGIME != NULL);
	
	pGIME->MmuTask=task;
	pGIME->MmuState= (!pGIME->MmuEnabled)<<1 | pGIME->MmuTask;
}

/********************************************************************/
/**
 */
void mmuSetEnabled (gime_t * pGIME, unsigned char usingmmu)
{
	assert(pGIME != NULL);
	
	pGIME->MmuEnabled=usingmmu;
	pGIME->MmuState= (!pGIME->MmuEnabled)<<1 | pGIME->MmuTask;
}

/********************************************************************/
/**
 */
//unsigned char * mmuGetIntROMPointer(gime_t * pGIME)
//{
//    assert(pGIME != NULL);
//    
//    return (pGIME->rom.pData);
//}

/********************************************************************/
/**
 */
void mmuSetDistoRamBank(gime_t * pGIME, unsigned char data)
{
	assert(pGIME != NULL);
	
	switch ( pGIME->CurrentRamConfig )
	{
		case Ram128k:
			return;
			break;
            
		case Ram512k:
			return;
			break;
            
        case Ram1024k:
            // untested
            SetVideoBank(pGIME,data & 1);
            mmuSetPrefix(pGIME,0);
            return;
            break;
            
		case Ram2048k:
			SetVideoBank(pGIME,data & 3);
			mmuSetPrefix(pGIME,0);
			return;
			break;
            
		case Ram8192k:	//8192K	//No Can 3
			SetVideoBank(pGIME,data & 0x0F);
			mmuSetPrefix(pGIME,(data & 0x30)>>4);
			return;
			break;
            
        default:
            assert(false && "Invalid ram configuration");
	}
	return;
}

/********************************************************************/
/**
 */
void mmuSetPrefix(gime_t * pGIME, unsigned char data)
{
	assert(pGIME != NULL);
	
	pGIME->MmuPrefix=(data & 3)<<8;
}

/********************************************************************/
/********************************************************************/
/*
 Coco3 MMU Code
*/

/**
 Read 8 bits from memory via the MMU
 */
uint8_t mmuMemRead8(emudevice_t * pEmuDevice, int address)
{
	gime_t *	pGIME;
	
	pGIME = (gime_t *)pEmuDevice;
	ASSERT_GIME(pGIME);
	
	assert(address >= 0 && address <= 0xFFFF);
	address = address & 0xFFFF;
	
#if 0
    int iBlock = address >> 13;
    int page = pGIME->MmuRegisters[pGIME->MmuState][iBlock];
    //int maska = VectorMaska[pGIME->CurrentRamConfig];
//    int mask = VectorMask[pGIME->CurrentRamConfig];

	// read RAM
	if ( address < 0xFE00 )
	{
		if ( pGIME->MemPageOffsets[page] == 1 )
		{
			return (pGIME->MemPages[page][address & 0x1FFF]);
		}
		
        // TODO: the Pak be registered somehow instead.  the mmu should have no knowledge of the machine as a whole
		// read from pak ROM
        coco3_t * pCoco3 = (coco3_t *)pGIME->mmu.device.pParent;
        ASSERT_CC3(pCoco3);
        
		return ( pakMem8Read(&pCoco3->pPak->device, pGIME->MemPageOffsets[page] + (address & 0x1FFF) ));
	}
	
	// I/O port read?
	// TODO: move to cpuMemRead8 once port read/write moved there?
	if ( address >= 0xFF00 )
	{
		return portRead(pGIME,address);
	}
	
	if ( pGIME->RamVectors )	//Address must be $FE00 - $FEFF
	{
        int mask = VectorMask[pGIME->CurrentRamConfig];
		return (pGIME->memory[(0x2000*mask)|(address & 0x1FFF)]);
	}
	
	if ( pGIME->MemPageOffsets[page] == 1 )
	{
		return (pGIME->MemPages[page][address & 0x1FFF]);
	}
	
	// read ROM?
    coco3_t * pCoco3 = (coco3_t *)pGIME->mmu.device.pParent;
    ASSERT_CC3(pCoco3);
    
	return ( pakMem8Read(&pCoco3->pPak->device, pGIME->MemPageOffsets[page] + (address & 0x1FFF) ));
#else
    coco3_t * pCoco3 = (coco3_t *)pGIME->mmu.device.pParent;
    ASSERT_CC3(pCoco3);

    if ( address < 0xFE00 )
    {
        if ( pGIME->MemPageOffsets[pGIME->MmuRegisters[pGIME->MmuState][address>>13]]==1)
        {
            return (pGIME->MemPages[pGIME->MmuRegisters[pGIME->MmuState][address>>13]][address & 0x1FFF]);
        }
        
        return pakMem8Read(&pCoco3->pPak->device, pGIME->MemPageOffsets[pGIME->MmuRegisters[pGIME->MmuState][address>>13]] + (address & 0x1FFF));
    }
    
    if ( address > 0xFEFF )
    {
        return portRead(pGIME,address);
    }
    
    if ( pGIME->RamVectors )    // Address must be $FE00 - $FEFF
    {
        return (pGIME->memory[(0x2000*VectorMask[pGIME->CurrentRamConfig])|(address & 0x1FFF)]);
    }
    
    if ( pGIME->MemPageOffsets[pGIME->MmuRegisters[pGIME->MmuState][address>>13]]==1)
    {
        return(pGIME->MemPages[pGIME->MmuRegisters[pGIME->MmuState][address>>13]][address & 0x1FFF]);
    }
    
    return pakMem8Read(&pCoco3->pPak->device, pGIME->MemPageOffsets[pGIME->MmuRegisters[pGIME->MmuState][address>>13]] + (address & 0x1FFF));
#endif
}

/********************************************************************/
/**
	Write 8 bits to memory via the MMU
 */
void mmuMemWrite8(emudevice_t * pEmuDevice, uint8_t data, int address)
{
	gime_t *	pGIME;
	coco3_t *	pCoco3;
	
	pGIME = (gime_t *)pEmuDevice;
	ASSERT_GIME(pGIME);
	
	pCoco3 = (coco3_t *)pGIME->mmu.device.pParent;
	ASSERT_CC3(pCoco3);
	
	assert(address >= 0 && address <= 0xFFFF);
	address = address & 0xFFFF;
	
	int iBlock = address >> 13;
    int page = pGIME->MmuRegisters[pGIME->MmuState][iBlock];
    int maska = VectorMaska[pGIME->CurrentRamConfig];
    int mask = VectorMask[pGIME->CurrentRamConfig];
    
	if ( address < 0xFE00 )
	{
		if (    pGIME->MapType 
			 | (page < maska)
			 | (page > mask)
		)
		{
			pGIME->MemPages[page][address & 0x1FFF] = data;
		}
		return;
	}
	
	// TODO: move to cpuMemRead8 once port read/write moved there?
	if ( address >= 0xFF00 )
	{
		portWrite(pGIME,data,address);
		return;
	}
	
    //Address must be $FE00 - $FEFF
    
	if ( pGIME->RamVectors )
	{
		pGIME->memory[(0x2000*mask)|(address & 0x1FFF)] = data;
	}
	else
	{
		if (   pGIME->MapType 
			 | (page < maska)
			 | (page > mask)
		)
		{
			pGIME->MemPages[page][address & 0x1FFF] = data;
		}
	}
}

/********************************************************************/
/********************************************************************/
/*
	'Port' read/write callback management
 */

// TODO: move to MMU?
// TODO: should it just be by address and not 'port'?

/**
	Initialize GIME port read/write dispatch table
*/
void portInit(gime_t * pGIME)
{
	assert(pGIME != NULL);
	
	memset(pGIME->portReadTable,0,sizeof(pGIME->portReadTable));
	memset(pGIME->portWriteTable,0,sizeof(pGIME->portWriteTable));
}

/**
	Register a default read callback
 */
void portRegisterDefaultRead(gime_t * pGIME, portreadfn_t pfnPortRead, emudevice_t * pEmuDevice)
{
	ASSERT_GIME(pGIME);
	
	pGIME->pDefaultPortRead.pEmuDevice	= pEmuDevice;
	pGIME->pDefaultPortRead.pfnReadPort	= pfnPortRead;
}

/**
 Register a default write callback
 */
void portRegisterDefaultWrite(gime_t * pGIME, portwritefn_t pfnPortWrite, emudevice_t * pEmuDevice)
{
	ASSERT_GIME(pGIME);
	
	pGIME->pDefaultPortWrite.pEmuDevice		= pEmuDevice;
	pGIME->pDefaultPortWrite.pfnWritePort	= pfnPortWrite;
}

/**
 Register a read callback for a specific port
 */
void portRegisterRead(gime_t * pGIME, portreadfn_t pfnPortRead, emudevice_t * pEmuDevice, int iStart, int iEnd)
{
	int				x;
	
	assert(pGIME != NULL);
	
	assert(iStart >= 0xFF00 && iStart <= 0xFFFF);
	assert(iEnd >= 0xFF00 && iEnd <= 0xFFFF);
	
	for (x=iStart; x<=iEnd; x++)
	{
		assert(pGIME->portReadTable[x-0xFF00].pfnReadPort == NULL && "Port read already registered");
		assert(pEmuDevice != NULL);
		
		pGIME->portReadTable[x-0xFF00].pEmuDevice	= pEmuDevice;
		pGIME->portReadTable[x-0xFF00].pfnReadPort	= pfnPortRead;
	}
}

/**
 Register a write callback for a specific port
 */
void portRegisterWrite(gime_t * pGIME, portwritefn_t pfnPortWrite, emudevice_t * pEmuDevice, int iStart, int iEnd)
{
	int		x;
	
	assert(pGIME != NULL);
	
	assert(iStart >= 0xFF00 && iStart <= 0xFFFF);
	assert(iEnd >= 0xFF00 && iEnd <= 0xFFFF);

	for (x=iStart; x<=iEnd; x++)
	{
		assert(pGIME->portWriteTable[x-0xFF00].pfnWritePort == NULL && "Port write already registered");
		assert(pEmuDevice != NULL);
		
		pGIME->portWriteTable[x-0xFF00].pEmuDevice		= pEmuDevice;
		pGIME->portWriteTable[x-0xFF00].pfnWritePort	= pfnPortWrite;
	}
}

/**
	read data from a specific port
 */
unsigned char portRead(gime_t * pGIME, int addr)
{
	unsigned char	port		= 0;
    unsigned char	temp		= 0;    // TODO: should this default to 0xFF instead?
	portreadfn_t	pPortReadFN;
	void *			pEmuDevice;
	
	assert(pGIME != NULL);
	assert(addr >= 0xFF00 && addr <=0xFFFF);
	
	port = (addr & 0xFF);
	
    // look for registered read function
	pPortReadFN	= pGIME->portReadTable[port].pfnReadPort;
	pEmuDevice	= pGIME->portReadTable[port].pEmuDevice;
	
	if ( pPortReadFN == NULL )
	{
        // no, use the default
		pPortReadFN = pGIME->pDefaultPortRead.pfnReadPort;
		pEmuDevice	= pGIME->pDefaultPortRead.pEmuDevice;
	}
    
	if ( pPortReadFN != NULL )
	{
		temp = (*pPortReadFN)(pEmuDevice,port);
	}
	
	return (temp);
}

/**
    write data to a specific port
 */
void portWrite(gime_t * pGIME, unsigned char data, int addr)
{
	unsigned char	port=0;
	portwritefn_t	pPortWriteFN;
	void *			pEmuDevice;
	
	assert(pGIME != NULL);
	assert(addr >= 0xFF00 && addr <=0xFFFF);
	
	port = (addr & 0xFF);
	
    // look for a registered write function
	pPortWriteFN	= pGIME->portWriteTable[port].pfnWritePort;
	pEmuDevice		= pGIME->portWriteTable[port].pEmuDevice;
	
	if ( pPortWriteFN == NULL )
	{
        // no use the default
		pPortWriteFN	= pGIME->pDefaultPortWrite.pfnWritePort;
		pEmuDevice		= pGIME->pDefaultPortWrite.pEmuDevice;
	}
    
	if ( pPortWriteFN != NULL )
	{
		(*pPortWriteFN)(pEmuDevice,port,data);
	}
}

/********************************************************************/
