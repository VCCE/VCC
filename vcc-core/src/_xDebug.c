/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/
/*****************************************************************************/
/*
	_xDebug.c : simple platform specific debug helpers
	Windows version
*/
/*****************************************************************************/

#include "xDebug.h"

/*
	System headers
*/ 
#include <stdio.h>
#include <stdarg.h>
#include <Windows.h>

/*****************************************************************************/

int	g_bTraceMessageLogging	= TRUE;
//int	g_bTraceMessageLogFile	= FALSE;

/*****************************************************************************/

#if 0
// stdio verison
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
	char temp[1024]; //yuck

	va_start(args, pFormat);

	// format string for Visual Studio Output window
	// lets you double click it to take you to the source file line
	// called from
	sprintf(temp, "%s(%d) : ", (char *)pFile, iLine);
	OutputDebugString(temp);

	vsprintf(temp, (char *)pFormat, args);
	OutputDebugString(temp);

	va_end(args);
}
#endif

/*****************************************************************************/

