#ifndef _xTimer_h_
#define _xTimer_h_

/*****************************************************************************/

#include "xTypes.h"

/*****************************************************************************/

typedef u64_t	xtime_t;

typedef struct xtimer_t * pxtimer_t;

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	XAPI xtime_t		XCALL xTimeGetMilliseconds();
	XAPI xtime_t		XCALL xTimeGetNanoseconds();

#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif
