#ifndef _orch90_h_
#define _orch90_h_

/*****************************************************************************/

#include "pakinterface.h"

/*****************************************************************************/

// TODO: move to private header
typedef struct orch90_t orch90_t;
struct orch90_t
{
	cocopak_t		pak;				// base
	
	uint8_t	LeftChannel;
	uint8_t	RightChannel;
} ;

#define VCC_COCOPAK_ORCH90_ID	XFOURCC('o','r','9','0')

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
