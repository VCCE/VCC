#ifndef __xThread_h_
#define __xThread_h_

/*****************************************************************************/

#include "xTypes.h"

/************************************************************************/

struct xthread_t
{
#if (defined _WINDOWS)
	void *					hSysThread;			/* system thread handle */
#endif
	pfnxthread_t			pThreadFunc;		/* function to call */
	void *					pUserParam;			/* user parameter to pass to thread function */
	
	char					szName[128];
};

/*****************************************************************************/

#endif
