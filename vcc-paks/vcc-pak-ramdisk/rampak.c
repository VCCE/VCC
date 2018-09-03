/*********************************************************************/
/*
	Disto RAM Pak interface VCCX plug-in
 */
/*********************************************************************/

#include "rampak.h"

#include <assert.h>

/*********************************************************************/

#define ASSERT_RAMPAK(pRAMPak)	assert(pRAMPak != NULL && "NULL pointer"); \
								assert(pRAMPak->pak.device.id == EMU_DEVICE_ID && "Invalid Emu device ID"); \
								assert(pRAMPak->pak.device.idModule == VCC_COCOPAK_ID && "Invalid module ID"); \
								assert(pRAMPak->pak.device.idDevice == VCC_COCOPAK_RAMPAK_ID && "Invalid device ID")

/*********************************************************************/
/**
 */
void rampakPortWrite(emudevice_t * pEmuDevice, uint8_t Port, uint8_t Data)
{
	rampak_t *	pRAMPak = (rampak_t *)pEmuDevice;
	
	ASSERT_RAMPAK(pRAMPak);
	if ( pRAMPak != NULL )
	{		
		switch ( Port )
		{
			case 0x40:
			case 0x41:
			case 0x42:
				WritePort(&pRAMPak->memboard,Port,Data);
				return;
				break;
				
			case 0x43:
				WriteArray(&pRAMPak->memboard,Data);
				return;
				break;
				
			default:
				return;
				break;
		}
	}
}

/*********************************************************************/
/**
 */
uint8_t rampakPortRead(emudevice_t * pEmuDevice, uint8_t Port)
{
	rampak_t *	pRAMPak = (rampak_t *)pEmuDevice;
	
	ASSERT_RAMPAK(pRAMPak);
	if ( pRAMPak != NULL )
	{		
		switch (Port)
		{
			case 0x43:
				return ReadArray(&pRAMPak->memboard);
			break;
				
			default:
				return(0);
			break;
		}
	}
	
	return 0;
}

/*********************************************************************/
/**
	TODO: RAM Pak config UI
 */
void rampakConfig(cocopak_t * pPak, uint8_t MenuID)
{
	rampak_t *	pRAMPak = (rampak_t *)pPak;
	
	ASSERT_RAMPAK(pRAMPak);
	if ( pRAMPak != NULL )
	{		
		assert(0);
	}
}

/*********************************************************************/
/**
 @param pPak Pointer to self
 
	TODO: RamPak reset
 */
void rampakReset(cocopak_t * pPak)
{
	rampak_t *	pRAMPak = (rampak_t *)pPak;
	
	ASSERT_RAMPAK(pRAMPak);
	if ( pRAMPak != NULL )
	{		
		assert(0);
	}
}

/*********************************************************************/
/**
 rampak interface instance destruction
 */
void rampakDestroy(cocopak_t * pPak)
{
	rampak_t *	pRAMPak = (rampak_t *)pPak;
	
	ASSERT_RAMPAK(pRAMPak);
	if ( pRAMPak != NULL )
	{		
		// de-register port read/write
		assert(0);
		
		// unload ourselves
		//pakDestroy(pPak);
		assert(0);

		free(pPak);
	}
}

/*********************************************************************/
/*********************************************************************/

#pragma mark -
#pragma mark --- RAM Pak Interface exported create function ---

/*********************************************************************/
/**
 MPI instance creation
 */
XAPI_EXPORT cocopak_t * vccpakModuleCreate()
{
	rampak_t *		pRAMPak;
	
	pRAMPak = malloc(sizeof(rampak_t));
	if ( pRAMPak != NULL )
	{
		memset(pRAMPak,0,sizeof(rampak_t));
		
		pRAMPak->pak.device.id			= EMU_DEVICE_ID;
		pRAMPak->pak.device.idModule	= VCC_COCOPAK_ID;
		pRAMPak->pak.device.idDevice	= VCC_COCOPAK_RAMPAK_ID;
		
		/*
		 Initialize PAK API function pointers
		 */
		//pRAMPak->pak.pakapi.pfnPakDestroy     = rampakDestroy;
		pRAMPak->pak.pakapi.pfnPakReset         = rampakReset;
		
		pRAMPak->pak.pakapi.pfnPakPortRead      = rampakPortRead;
		pRAMPak->pak.pakapi.pfnPakPortWrite		= rampakPortWrite;

		pRAMPak->pak.pakapi.pfnPakConfig		= rampakConfig;
		pRAMPak->pak.device.pfnGetStatus		= NULL;
		
		strcpy(pRAMPak->pak.device.Name,"Disto RAM Pak Interface");
		
		/*
			initialize memory .  This is not changed on reset.
		 */
		InitMemBoard(&pRAMPak->memboard);
		
		emuDevRegisterDevice(&pRAMPak->pak.device);
		
		return &pRAMPak->pak;
	}
	
	return NULL;
}

/*****************************************************************************/
