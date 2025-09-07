//------------------------------------------------------------------
// Copyright 2015 by Joseph Forgione
// This file is part of VCC (Virtual Color Computer).
//
// VCC (Virtual Color Computer) is free software: you can redistribute it
// and/or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.  You should have
// received a copy of the GNU General Public License along with VCC
// (Virtual Color Computer). If not see <http://www.gnu.org/licenses/>.
//------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include "logger.h"

#define TOCONS 0
#define TOFILE 1

static HANDLE hLog_Out = nullptr;
DWORD dummy;

char LogFileName[MAX_PATH]="VccLog.txt";

//void CpuDump(void) {
//    for (x=0;x<=65535;x++)
//        PrintLogF("%c",MemRead8(x));
//}

void WriteLog(char *Message, unsigned char Type) {
    switch (Type) {
    case TOCONS:
        PrintLogC(Message);
        break;
    case TOFILE:
        PrintLogF(Message);
        break;
    }
}

// PrintLogC - Put formatted string to the console
void PrintLogC(const void * fmt, ...)
{
    va_list args;
    char msg[512];
    va_start(args, fmt);
    vsnprintf(msg, 512, (char *)fmt, args);
    va_end(args);

    if (hLog_Out == nullptr) {
        AllocConsole();
        hLog_Out = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTitle("Logging Window");
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

    int oflag = _O_CREAT | _O_APPEND | _O_RDWR;
    int pmode = _S_IREAD | _S_IWRITE;
    int flog = _open(LogFileName,oflag,pmode);
    _write(flog, msg, strlen(msg));
    _close(flog);
}

// OpenLogFile - create new log file
void OpenLogFile(char * logfile)
{
	if (strlen(logfile) > MAX_PATH) return;
	strcpy(LogFileName,logfile);
    int oflag = _O_CREAT | _O_TRUNC | _O_RDWR;
    int pmode = _S_IREAD | _S_IWRITE;
    int fLog = _open(LogFileName,oflag,pmode);
    _close(fLog);
}

