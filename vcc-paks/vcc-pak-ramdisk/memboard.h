#ifndef _memboard_h_
#define _memboard_h_

/*****************************************************************************/

#include "pakinterface.h"

#include "memboard.h"

/*****************************************************************************/
/*
	TODO: make configurable
 */
#define RAMSIZE 1024*512

/*****************************************************************************/

// TODO: move to private header

typedef union address_t
{
	unsigned int Address;
	struct
	{
		uint8_t lswlsb;
		uint8_t lswmsb;
		uint8_t mswlsb;
		uint8_t mswmsb;
	} Byte;
} address_t;

typedef struct memboard_t memboard_t;
struct memboard_t
{
	address_t   IndexAddress;
	uint8_t *	RamBuffer;
} ;

/*****************************************************************************/

#if (defined __cplusplus)
extern "C"
{
#endif
	
	bool			InitMemBoard(memboard_t * pMemboard);
	bool			WritePort(memboard_t * pMemboard, uint8_t, uint8_t);
	bool			WriteArray(memboard_t * pMemboard, uint8_t);
	uint8_t	        ReadArray(memboard_t * pMemboard);
	
#if (defined __cplusplus)
}
#endif

/*****************************************************************************/

#endif
