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

// PrintLogC - Put a formatted debug string to the screen
void PrintLogC(const char* fmt, ...)
{
    va_list args;
    char msg[512];
    va_start(args, fmt);
    vsnprintf(msg, 512, fmt, args);
    va_end(args);

    // If running under a debugger use it.
    if (IsDebuggerPresent()) {
        OutputDebugStringA(msg);
        return;
    }

    // If not running debugger use console
    if (hLog_Out == nullptr) {
        AllocConsole();
        hLog_Out = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTitle("Logging Window");
        if (hLog_Out == NULL || hLog_Out == INVALID_HANDLE_VALUE) {
            OutputDebugStringA(msg);
            return;
        }
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

//-------------------------------------------------------------
//  Time stamped and compressed logging to console or debugger
//  duplicate entries are compressed to two lines, second has count
//  Timestamp rolls over at 32bits (4294967295)
//-------------------------------------------------------------
void PrintStampedLog(bool tofile, unsigned int ts, const char * fmt, ...) {

	static int dup_count=0;
	static std::string last_log{};

    va_list args;
    char msg[512];
    va_start(args, fmt);
    vsnprintf(msg, 512, fmt, args);
    va_end(args);

	std::string log = msg;

	char stamp[16];
	snprintf(stamp,16,"%10u ",static_cast<uint32_t>(ts));

	if (log == last_log) {
		dup_count++;
	} else {
		if (dup_count > 1) {
			std::string d = " dup(" + std::to_string(dup_count+1) + ")\n";
			std::string s = stamp + last_log + d;
			if (tofile)
				PrintLogF(s.c_str());
			else
				PrintLogC(s.c_str());
		}
		// empty log string only flushes dups
		if (!log.empty()) {
			std::string s = stamp + log + "\n";
			if (tofile)
				PrintLogF(s.c_str());
			else
				PrintLogC(s.c_str());
		}
		last_log = log;
		dup_count = 0;
	}
}

