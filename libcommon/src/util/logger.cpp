////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute itand/or
//	modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your
//	option) any later version.
//	
//	VCC (Virtual Color Computer) is distributed in the hope that it will be
//	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
//	Public License for more details.
//	
//	You should have received a copy of the GNU General Public License along with
//	VCC (Virtual Color Computer). If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <vcc/util/logger.h>

static HANDLE hLog_Out = nullptr;
static const auto LogFileName = "VccLog.txt";

// PrintLogC - Put formatted string to the console
void PrintLogC(const char* fmt, ...)
{
    va_list args;
    char msg[512];
    va_start(args, fmt);
    vsnprintf(msg, 512, fmt, args);
    va_end(args);

    if (hLog_Out == nullptr) {
        AllocConsole();
        hLog_Out = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTitle("Logging Window");
    }
    WriteFile(hLog_Out,msg,strlen(msg),nullptr,nullptr);
}

// PrintLogF - Put formatted string to the log file
void PrintLogF(const char* fmt, ...)
{
    va_list args;
    char msg[512];
    va_start(args, fmt);
    vsnprintf(msg, 512, fmt, args);
    va_end(args);

    int oflag = _O_CREAT | _O_APPEND | _O_RDWR;
    int pmode = _S_IREAD | _S_IWRITE;
    int flog = _open(LogFileName,oflag,pmode);
    _write(flog, msg, strlen(msg));
    _close(flog);
}
