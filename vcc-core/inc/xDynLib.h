#ifndef _xDynLib_h_
#define _xDynLib_h_

/*****************************************************************************/

#include "xTypes.h"

/*****************************************************************************/

typedef struct xdynlib_t * pxdynlib_t;

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	XAPI result_t	XCALL xDynLibLoad(const char * pPathname, pxdynlib_t * ppxdynlib);
	XAPI result_t	XCALL xDynLibUnload(pxdynlib_t * ppxdynlib);
	XAPI result_t	XCALL xDynLibGetSymbolAddress(pxdynlib_t pxdynlib, const char * pcszSymbol, void ** ppvSymbol);
	
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif
