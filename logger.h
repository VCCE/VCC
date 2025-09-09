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

#ifndef __LOGGER_H__
#define __LOGGER_H__

#define TOCONS 0
#define TOFILE 1

void WriteLog(const char *, unsigned char);
void PrintLogC(const void * fmt, ...);
void PrintLogF(const void * fmt, ...);
void OpenLogFile(const char * filename);

// Debug logging if USE_LOGGING is defined

#ifdef USE_LOGGING
#define _DLOG(...) PrintLogC(__VA_ARGS__)
#define _LOGF(...) PrintLogF(__VA_ARGS__)
#else
#define _DLOG(...)
#define _LOGF(...)
#endif

#endif
