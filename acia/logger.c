/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it
    and/or modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see 
    <http://www.gnu.org/licenses/>.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "logger.h"

static FILE  *fLog_Out=NULL;
static HANDLE hLog_Out=NULL;
DWORD dummy;

// PrintLogC - Put formatted string to the console
void PrintLogC(const void * fmt, ...)
{
	va_list args;
	char msg[512];
	va_start(args, fmt);
	vsnprintf(msg, 512, (char *)fmt, args);
	va_end(args);

	if (hLog_Out==NULL) {
		hLog_Out = GetStdHandle(STD_OUTPUT_HANDLE);
		char heading[]="\n -- Vcc Log --\n";
		if (!WriteFile(hLog_Out,heading,strlen(heading),&dummy,0)) {
			AllocConsole();
			hLog_Out=GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTitle("Logging Window");
		}
	}

	WriteFile(hLog_Out,msg,strlen(msg),&dummy,0);
}

// PrintLogF - Put formatted string to the log file
void PrintLogF(const void * fmt, ...)
{
	va_list args;
	char msg[512];
	va_start(args, fmt);
	vsnprintf(msg, 512, (char *)fmt, args);
	va_end(args);

	if (fLog_Out == NULL)
		fLog_Out = fopen("VccLog.txt","w");

	fprintf(fLog_Out,msg);
	fflush(fLog_Out);
}

// OpenLogFile - open non-default file for logging
void OpenLogFile(char * logfile)
{
	if (fLog_Out == NULL) fclose(fLog_Out);
	fLog_Out = fopen(logfile,"wb");
}

