#ifndef _xDebug_h_
#define _xDebug_h_

/*****************************************************************************/

#include "xTypes.h"

/*****************************************************************************/

#if (defined DEBUG)
#   include <assert.h>
#   ifndef _DEBUG
#       define _DEBUG
#   endif
#else
#ifndef assert
#   define assert(x)
#endif
#endif

/*
	Debug trace macros
*/
#if (defined _DEBUG)
#	define XTRACE(x,...)			_xDbgTrace( __FILE__, __LINE__, x, ##__VA_ARGS__ )
#else
#	define XTRACE(x,...)
#endif

/*
	Break to debugger macros
*/
#if (defined _DEBUG)
#	define xDbgBreak()			_xDbgBreak()
#else
#	define xDbgBreak()
#endif

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	void			XCALL _xDbgTrace(const void * pFile, const int iLine, const void * pFormat, ...);
	
#ifdef _DEBUG
	void			XCALL _xDbgBreak(void);
#endif
	
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif
