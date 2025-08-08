/*****************************************************************************/
/**
	xDebug.c
 
	uEng - micro cross platform engine
	Copyright 2010 by Wes Gale All rights reserved.
	Used by permission in VccX
*/
/*****************************************************************************/

#include "xDebug.h"

/*
	System headers
*/ 
//#include <ucontext.h>
//#include <unistd.h>
//#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <Windows.h>

/*****************************************************************************/

int	g_bTraceMessageLogging	= TRUE;
//int	g_bTraceMessageLogFile	= FALSE;

/*****************************************************************************/

#if 0
void _xDbgTrace(const void * pFile, const int iLine, const void * pFormat, ...)
{
	va_list		args;
	
	va_start(args,pFormat);
	
	fprintf(stdout,"%s(%d) : ", (char *)pFile, iLine);

	vfprintf(stdout,(char *)pFormat,args);
	
	va_end(args);
	
	fflush(stdout);
}
#else
void _xDbgTrace(const void * pFile, const int iLine, const void * pFormat, ...)
{
	va_list		args;
	char temp[1024];

	va_start(args, pFormat);

	sprintf(temp, "%s(%d) : ", (char *)pFile, iLine);
	OutputDebugString(temp);

	vsprintf(temp, (char *)pFormat, args);
	OutputDebugString(temp);

	va_end(args);

//	fflush(stdout);
}
#endif

/*****************************************************************************/

