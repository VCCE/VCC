/*****************************************************************************/
/**
	xDebug.c
*/
/*****************************************************************************/

#include "xDebug.h"

#if !(defined _WIN32)
#	error "WINDOWS implementation"
#endif

/*
	System headers
*/ 
//#include <ucontext.h>
//#include <unistd.h>
//#include <dlfcn.h>
#include <stdio.h>
#include <stdarg.h>

/*****************************************************************************/

bool	g_bTraceMessageLogging	= TRUE;
bool	g_bTraceMessageLogFile	= FALSE;

/*****************************************************************************/

void XCALL _xDbgTrace(const void * pFile, const int iLine, const void * pFormat, ...)
{
	va_list		args;
	
	va_start(args,pFormat);
	
	fprintf(stdout, "%s(%d) : ", (char *)pFile, iLine);

	vfprintf(stdout,pFormat,args);
	
	va_end(args);
	
	fflush(stdout);
}

/*****************************************************************************/

#ifdef _DEBUG
void XCALL _xDbgBreak()
{
	/*
	 Drop into the debugger; the Debugger() function call seems to produce
	 the same effect as calling SysBreak(). In both cases, you can Step Out
	 to continue execution of the program.
	 */
	_asm
	{
		int 3
	}
}
#endif

/*****************************************************************************/

