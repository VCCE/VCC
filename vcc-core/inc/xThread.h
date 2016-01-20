#ifndef _xThread_h_
#define _xThread_h_

/*****************************************************************************/

#include "xTypes.h"

/*****************************************************************************/

/*
	opaque thread handle
 */
typedef struct xthread_t	xthread_t;
typedef struct xthread_t *	pxthread_t;

/*
	opaque mutex handle
*/
typedef struct xmutex_t		xmutex_t;
typedef struct xmutex_t *	pxmutex_t;

/*
	Thread function prototype
*/
typedef int_t (* pfnxthread_t)(void * pParam);

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	/* 
		thread management 
	*/	
	XAPI pxthread_t			XCALL xThreadStart(const char * pnszName, pfnxthread_t pFunction, void * pParam);
	XAPI int_t				XCALL xThreadSleep(size_t uiMilliseconds);
	XAPI int_t				XCALL xThreadStop(pxthread_t pThread);

	/* only needed for OSX */
	void		sysStartThread(void * pThread);

	void		sysShowError(const char * pcszText);

	/*
		Mutual exclusion
	 */
	XAPI pxmutex_t			XCALL xMutexCreate();
	XAPI int_t				XCALL xMutexLock(pxmutex_t pMutex);
	XAPI int_t				XCALL xMutexUnlock(pxmutex_t pMutex);
	XAPI void				XCALL xMutexDestroy(pxmutex_t pMutex);
	
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif
