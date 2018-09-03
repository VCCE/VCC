/*********************************************************************/
/*
 */
/*********************************************************************/

#include "orch90.h"
#include "orch90rom.h"

#include "mc6821.h"
#include "Vcc.h"

#include <assert.h>

/*********************************************************************/

#define ASSERT_ORCH90(pOrch90)	assert(pOrch90 != NULL && "NULL pointer"); \
								assert(pOrch90->pak.device.id == EMU_DEVICE_ID && "Invalid Emu device ID"); \
								assert(pOrch90->pak.device.idModule == VCC_COCOPAK_ID && "Invalid module ID"); \
								assert(pOrch90->pak.device.idDevice == VCC_COCOPAK_ORCH90_ID && "Invalid device ID")

/*********************************************************************/

// This gets called at the end of every scan line 262 Lines * 60 Frames = 15780 Hz 15720
unsigned short orch90ModuleAudioSample(cocopak_t * pPak)
{
	orch90_t *		pOrch90;
	
	assert(pPak != NULL);
	if ( pPak != NULL )
	{
		pOrch90 = (orch90_t *)pPak;
		ASSERT_ORCH90(pOrch90);
		
		return ((pOrch90->LeftChannel<<8) | pOrch90->RightChannel) ;
	}
	
	return 0;
}

/*********************************************************************/
/**
 */
void orch90PortWrite(emudevice_t * pEmuDevice, unsigned char Port, unsigned char Data)
{
	orch90_t *		pOrch90;
	
	assert(pEmuDevice != NULL);
	if ( pEmuDevice != NULL )
	{
		pOrch90 = (orch90_t *)pEmuDevice;
		ASSERT_ORCH90(pOrch90);
	
		switch ( Port )
		{
			case 0x7A:
				pOrch90->RightChannel = Data;			
				break;
				
			case 0x7B:
				pOrch90->LeftChannel = Data;
				break;
		}
	}
}

/*********************************************************************/
/**
 */
unsigned char orch90PortRead(emudevice_t * pEmuDevice, unsigned char Port)
{
	return 0;
}

/*********************************************************************/
/**
	TODO: use CoCo Pak ROM variables
 */
unsigned char orch90MemRead8(emudevice_t * pEmuDevice, int Address)
{
	return (Rom[Address & 8191]);
}

/*********************************************************************/
/**
 TODO: use CoCo Pak ROM variables
 */
void orch90MemWrite8(emudevice_t * pEmuDevice, unsigned char Data, int Address)
{
}

/*********************************************************************/
/**
 */
void orch90Config(cocopak_t * pPak, unsigned char MenuID)
{
	orch90_t *		pOrch90;
	
	assert(pPak != NULL);
	if ( pPak != NULL )
	{
		pOrch90 = (orch90_t *)pPak;
		ASSERT_ORCH90(pOrch90);

		assert(0);
	}
}

/*********************************************************************/
/**
	@param pPak Pointer to self
*/
void orch90Reset(cocopak_t * pPak)
{
	orch90_t *		pOrch90;
	
	assert(pPak != NULL);
	if ( pPak != NULL )
	{
		pOrch90 = (orch90_t *)pPak;
		ASSERT_ORCH90(pOrch90);
		
        assert(0 && "register port read/write");
	
		//assert(0 && "find the MC6821 object");
		// TODO: handle in CoCo3 object?  assert cart if there is a ROM?
		//mc6821_SetCart(NULL,1);
	}
}

/*********************************************************************/
/**
	orch90 interface instance destruction
 */
void orch90Destroy(cocopak_t * pPak)
{
	orch90_t *	pOrch90 = (orch90_t *)pPak;
	
	ASSERT_ORCH90(pOrch90);
	if ( pOrch90 != NULL )
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
#pragma mark --- Orchestra 90 Interface exported create function ---

/*********************************************************************/
/**
 MPI instance creation
 */
XAPI_EXPORT cocopak_t * vccpakModuleCreate()
{
	orch90_t *		pOrch90;
	
	pOrch90 = malloc(sizeof(orch90_t));
	if ( pOrch90 != NULL )
	{
		memset(pOrch90,0,sizeof(orch90_t));
		
		pOrch90->pak.device.id			= EMU_DEVICE_ID;
		pOrch90->pak.device.idModule	= VCC_COCOPAK_ID;
		pOrch90->pak.device.idDevice	= VCC_COCOPAK_ORCH90_ID;
		
		/*
		 Initialize PAK API function pointers
		 */
		//pOrch90->pak.pakapi.pfnPakDestroy		= orch90Destroy;
		pOrch90->pak.device.pfnGetStatus		= NULL;
        
		pOrch90->pak.pakapi.pfnPakReset         = orch90Reset;
		pOrch90->pak.pakapi.pfnPakAudioSample   = orch90ModuleAudioSample;
		pOrch90->pak.pakapi.pfnPakConfig		= orch90Config;
		pOrch90->pak.pakapi.pfnPakHeartBeat		= NULL;
		pOrch90->pak.pakapi.pfnPakPortRead		= orch90PortRead;
		pOrch90->pak.pakapi.pfnPakPortWrite		= orch90PortWrite;
		pOrch90->pak.pakapi.pfnPakMemRead8		= orch90MemRead8;
		pOrch90->pak.pakapi.pfnPakMemWrite8		= orch90MemWrite8;
		
		strcpy(pOrch90->pak.device.Name,"Disto RAM Pak Interface");
		
		emuDevRegisterDevice(&pOrch90->pak.device);

		return &pOrch90->pak;
	}
	
	return NULL;
}

/*****************************************************************************/



