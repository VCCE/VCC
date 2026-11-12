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

//----------------------------------------------------------------------
// October 2026 EJJ added bare DLOG, USE_LOGFILE, LOG_STAMPED macros
// for detailed logging of I/O ports to debugger, console, or file
//----------------------------------------------------------------------

#pragma once

#include <string>

// Print to Console or debugger
void PrintLogC(const char* fmt, ...);
inline void PrintLogC(std::string log) {
	PrintLogC("%s",log.c_str());
}

// Print to File
void PrintLogF(const char* fmt, ...);
inline void PrintLogF(std::string log) {
	PrintLogF("%s",log.c_str());
}

// Timestamp log entries with duplicate squashing with friendly overloads
void PrintStampedLog(bool tofile, unsigned int ts, const char *fmt, ...);
inline void PrintStampedLog(bool tofile, unsigned int ts, std::string log) {
	PrintStampedLog(tofile, ts, "%s", log.c_str() );
}

// Debug logging if USE_LOGGING is defined
#ifdef USE_LOGGING
// DLOG and friends
#ifdef USE_LOGFILE
#define DLOG(...) PrintLogF(__VA_ARGS__)
#else
#define DLOG(...) PrintLogF(__VA_ARGS__)
#endif
#define DLOG_C(...) PrintLogC(__VA_ARGS__)
#define DLOG_F(...) PrintLogF(__VA_ARGS__)
// Time stamped logging
#ifdef USE_LOGFILE
#define LOG_STAMPED(ts,...) PrintStampedLog(true,ts,__VA_ARGS__)
#else
#define LOG_STAMPED(ts,...) PrintStampedLog(false,ts,__VA_ARGS__)
#endif
// void macros if USE_LOGGING not defined
#else
#define DLOG_C(...) ((void)0)
#define DLOG_F(...) ((void)0)
#define DLOG(...) ((void)0)
#define LOG_STAMPED(ts,...) ((void)0)
#endif

