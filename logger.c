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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>		// For PrintLogC
#include "tcc1014mmu.h"	// Need memread for CpuDump
#include "logger.h"

static FILE  *fLogOut=NULL;
static HANDLE hLog_Out=NULL;

void WriteLog(char *Message,unsigned char Type)
{
	static int newconsole=0;
	unsigned long dummy;
	switch (Type)
	{
	case TOCONS:
		if (hLog_Out==NULL) {
			// Write existing console. Create a new one if that fails
			hLog_Out=GetStdHandle(STD_OUTPUT_HANDLE);
			char heading[]="\n -- Vcc Log --\n";
			if (!WriteFile(hLog_Out,heading,strlen(heading),&dummy,0)) {
				AllocConsole();
				hLog_Out=GetStdHandle(STD_OUTPUT_HANDLE);
				SetConsoleTitle("Logging Window");
				newconsole = 1;
			}
		}
		if (newconsole) {
			WriteConsole(hLog_Out,Message,strlen(Message),&dummy,0);
		} else {
			WriteFile(hLog_Out,Message,strlen(Message),&dummy,0);
		}
		break;

	case TOFILE:
		if (fLogOut ==NULL) fLogOut=fopen("c:\\VccLog.txt","w");
		fprintf(fLogOut,"%s\r\n",Message);
		fflush(fLogOut);
		break;
	}
}

void CpuDump(void)
{
	FILE* disk_handle=NULL;
	int x;
	disk_handle=fopen("c:\\cpuspace_dump.txt","wb");
	for (x=0;x<=65535;x++)
		fprintf(disk_handle,"%c",MemRead8(x));
	fflush(disk_handle);
	fclose(disk_handle);
	return;
}

// PrintLogC - Put formatted string to the console
void PrintLogC(const void * fmt, ...)
{
	va_list args;
	char str[512];
	va_start(args, fmt);
	vsnprintf(str, 512, (char *)fmt, args);
	va_end(args);
	WriteLog(str, TOCONS);
}

// OpenLogFile - open non-default file for logging
void OpenLogFile(char * logfile)
{
	if (fLogOut == NULL) fclose(fLogOut);
	fLogOut=fopen(logfile,"wb");
}