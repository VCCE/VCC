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
#pragma once
#include <vcc/core/detail/exports.h>

LIBCOMMON_EXPORT void PrintLogC(const char* fmt, ...);
LIBCOMMON_EXPORT void PrintLogF(const char* fmt, ...);

// Debug logging if USE_LOGGING is defined

#ifdef USE_LOGGING
#define DLOG_C(...) PrintLogC(__VA_ARGS__)
#define DLOG_F(...) PrintLogF(__VA_ARGS__)
#else
#define DLOG_C(...) ((void)0)
#define DLOG_F(...) ((void)0)
#endif
