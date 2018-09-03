#ifndef _rampak_h_
#define _rampak_h_

/*****************************************************************************/

#include "pakinterface.h"

#include "memboard.h"

/*****************************************************************************/

// TODO: move to private header
typedef struct rampak_t rampak_t;
struct rampak_t
{
	cocopak_t		pak;				// base
	
	memboard_t		memboard;
} ;

#define VCC_COCOPAK_RAMPAK_ID	XFOURCC('c','d','r','p')

/*****************************************************************************/

#if (defined __cplusplus)
extern "C"
{
#endif
	
	XAPI_EXPORT cocopak_t * vccpakModuleCreate(void);
	
#if (defined __cplusplus)
}
#endif

/*****************************************************************************/

#endif
