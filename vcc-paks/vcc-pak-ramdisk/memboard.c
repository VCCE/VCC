/*********************************************************************/
/*
 */
/*********************************************************************/

#include "memboard.h"

#include <assert.h>

/*********************************************************************/
/**
 */
bool InitMemBoard(memboard_t * pMemboard)
{
	pMemboard->IndexAddress.Address = 0;
	if ( pMemboard->RamBuffer != NULL )
	{
		free(pMemboard->RamBuffer);
	}
	pMemboard->RamBuffer = (unsigned char *)malloc(RAMSIZE);
	if ( pMemboard->RamBuffer == NULL )
	{
		// true is failure?
		return TRUE;
	}
	memset(pMemboard->RamBuffer,0,RAMSIZE);
	
	// false is success?
	return FALSE;
}

/*********************************************************************/
/**
 */
bool WritePort(memboard_t * pMemboard, uint8_t Port, uint8_t Data)
{
	switch ( Port )
	{
		case 0x40:
			pMemboard->IndexAddress.Byte.lswlsb = Data;
		break;
			
		case 0x41:
			pMemboard->IndexAddress.Byte.lswmsb = Data;
		break;
			
		case 0x42:
			pMemboard->IndexAddress.Byte.mswlsb = (Data & 0x7);
		break;
	}
	
	return FALSE;
}

/*********************************************************************/
/**
 */
bool WriteArray(memboard_t * pMemboard, uint8_t Data)
{
	assert(pMemboard != NULL);
	
	pMemboard->RamBuffer[pMemboard->IndexAddress.Address]=Data;
	
	return FALSE;
}

/*********************************************************************/
/**
 */
uint8_t ReadArray(memboard_t * pMemboard)
{
	assert(pMemboard != NULL);
	
	return pMemboard->RamBuffer[pMemboard->IndexAddress.Address];
}

/*********************************************************************/


