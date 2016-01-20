/*****************************************************************************/
/**
	_xThread.c - Win32 specific implementation of thread support
 */
/*****************************************************************************/

#include "xThread.h"

//#include "xMalloc.h"
#include "xDebug.h"
#include "xAssert.h"

/*
	internal
*/
#include "_xThread.h"

/*
	system specific
*/
#include <Windows.h>

/************************************************************************/
/**
	@internal
 
	Opaque type for system specific mutual exclusion object.
 */
struct xmutex_t
{
	CRITICAL_SECTION	csection;

	char				szName[64];
} ;

/*****************************************************************************/

void sysShowError(const char * pcszText)
{
	assert(0);
}

/*****************************************************************************/
/**
	@internal
 
	Master thread function
*/
DWORD WINAPI _mxThreadMasterProc(LPVOID lpParameter)
{
	pxthread_t	pThread;
	int_t		iResult;
	
	pThread = (pxthread_t)lpParameter;
	assert(pThread != NULL);
	if ( pThread != NULL )
	{
		if ( pThread->pThreadFunc != NULL )
		{
			__try
			{
				iResult = (*pThread->pThreadFunc)(pThread->pUserParam);
			} 
			__except ( EXCEPTION_CONTINUE_SEARCH )
			{
				assert(0);
			}
		}
	}
	
	return iResult;
}

/*****************************************************************************/
/**
	Start a new thread
 */
XAPI pxthread_t XCALL xThreadStart(const char * pnszName, pfnxthread_t pfnThreadFunc, void * pParam)
{
	pxthread_t	pThread;
	//int			result	= 0;
	
	pThread = malloc(sizeof(xthread_t));
	if ( pThread != NULL )
	{
		memset(pThread,0,sizeof(xthread_t));

		pThread->pThreadFunc	= pfnThreadFunc;
		pThread->pUserParam		= pParam;
		
		pThread->hSysThread = CreateThread(	NULL,	// LPSECURITY_ATTRIBUTES lpThreadAttributes,
											8192,	// SIZE_T dwStackSize,
											_mxThreadMasterProc,
											pThread, // LPVOID lpParameter,
											CREATE_SUSPENDED,
											NULL	// LPDWORD lpThreadId
									);
		if ( pThread->hSysThread == NULL )		
		{
			assert(0 && "An error ocurred creating the thread");
			
			free(pThread);
			pThread = NULL;
		}
		else
		{
			ResumeThread(pThread->hSysThread);
		}
	}
	
	return pThread;
}

/*****************************************************************************/
/**
 */
XAPI int_t XCALL xThreadSleep(size_t uiMilliseconds)
{
	/* This function takes time in microseconds */
	Sleep( (DWORD)uiMilliseconds );

	return 0;
}

/*****************************************************************************/
/**
 */
XAPI int_t XCALL xThreadStop(pxthread_t pThread)
{
	int_t	iResult	= -1;
	
	if ( pThread != NULL )
	{
		if ( pThread->hSysThread != NULL )
		{
			assert(0);
			
			pThread->hSysThread			= NULL;
		}
	}
	
	return iResult;
}

/*****************************************************************************/
/**
 */
XAPI int_t XCALL xThreadPause(pxthread_t pThread)
{
	assert(0);
	
	return 0;
}

/*****************************************************************************/
/**
 */
XAPI int_t XCALL xThreadUnpause(pxthread_t pThread)
{
	assert(0);
	
	return 0;
}

/*****************************************************************************/
/**
 */
XAPI pxmutex_t XCALL xMutexCreate()
{
	pxmutex_t			pMutex		= NULL;
	int_t				result		= 0;	
	
	pMutex = (pxmutex_t) malloc( sizeof(xmutex_t) );
	if ( pMutex != NULL )
	{
		/* Clear the structure */
		memset( pMutex, 0, sizeof(xmutex_t) );
		
		InitializeCriticalSection(&pMutex->csection);
	}
	
	return pMutex;
}

/*****************************************************************************/
/**
 */
XAPI int_t XCALL xMutexLock(pxmutex_t pMutex)
{
	//t_id	idCurThread = mxThreadGetID( );
	//int_t	result;
	
	/* Validate the parameter. */
	if ( pMutex == NULL )
	{
		assert( 0 && "Null mutex specified" );
		
		return -1;
	}
	
	EnterCriticalSection(&pMutex->csection);

	return 0;
}

/*****************************************************************************/
/**
 */
XAPI int_t XCALL xMutexUnlock(pxmutex_t pMutex)
{
	//t_id	idCurThread = mxThreadGetID( );
	int_t		result	= 0;
	
	/* Validate the parameter. */
	if ( pMutex == NULL )
	{
		assert( 0 && "Null mutex specified" );
		
		return -1;
	}
	
	LeaveCriticalSection(&pMutex->csection);
	
	return result;
}

/*****************************************************************************/
/**
 */
XAPI void XCALL xMutexDestroy(pxmutex_t pMutex)
{
	int_t		result = -1;
	
	/* Validate the parameter. */
	if ( pMutex == NULL )
	{
		assert(0 && "Null mutex specified");
		return;
	}
	
	do
	{
		DeleteCriticalSection(&pMutex->csection);

	} while ( result != 0 );
	
	/* Release memory */
	free( pMutex );
}

/*****************************************************************************/
