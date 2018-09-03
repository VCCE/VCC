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

#ifndef _xTypes_h_
#define _xTypes_h_

/*****************************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

/*****************************************************************************/

// TODO: remove this
#if (defined __GNUC__ && defined __APPLE__)
#	define OSX
#	define _MAC
#endif

#if (defined _MAC)
#	ifdef CONFIGURATION_Debug			// NOTE: Configuration name must be "Debug"
#		if !(defined _DEBUG)
#			define _DEBUG
#		elif CONFIGURATION_Release		// NOTE: Configuration name must be "Release"
#			define NDEBUG
#		else
#			error "Configuration name is not defined"
#		endif
#	endif
#endif

#if (defined _WIN32)
#	if (defined _DEBUG)
#		define DEBUG
#	endif
#endif

/*****************************************************************************/
/*
	cross platform defines
*/

#if (defined _LIB)
#	if (defined _WIN32)
#		define XAPI __declspec(dllexport)
#		define XLOCAL __declspec(dllimport)
#	elif (defined OSX)
#		define XAPI __attribute__((visibility("public")))
#		define XLOCAL  __attribute__((visibility("hidden")))
#	else
#		error "platformed undefined for library build"
#	endif
#else
#	define XAPI
#	define XLOCAL
#endif

#if (defined _WIN32)
#	define XCALL __fastcall
#	define XSTDCALL
#	define XCDECL
#	define XAPI_EXPORT __declspec(dllexport)

#	pragma warning (disable:4068)

#else
#	define XCALL
#	define XSTDCALL
#	define XCDECL
#	define XAPI_EXPORT 
#endif

#define XINLINE
#define XCALLBACK

#if (defined _WIN32) && !(defined PATH_MAX)
#define PATH_MAX _MAX_PATH
#endif


/*****************************************************************************/

#ifndef TRUE
#    define TRUE (1)
#endif

#ifndef FALSE
#    define FALSE (0)
#endif

#ifndef NULL
#	ifdef __cplusplus
#		define NULL    0
#	else
#		define NULL    ((void *)0)
#	endif
#endif

#if !(defined MAX)
#   define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif
#if !(defined MIN)
#   define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#if !(defined max)
#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })
#endif

#if !(defined min)
#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b; })
#endif

/*****************************************************************************/
/*
	handy types
 */

typedef void *				handle_t;

/*****************************************/
/*
	result_t error codes
*/

typedef enum result_t
{
    XERROR_NONE                 = 0,
    XERROR_GENERIC              = (0-1),
    XERROR_INVALID_PARAMETER    = (0-2),
    XERROR_BUFFER_OVERFLOW      = (0-3),
    XERROR_OUT_OF_MEMORY        = (0-4),
    XERROR_NOT_FOUND            = (0-5),
    XERROR_EOF                  = (0-6),
    XERROR_READ                 = (0-7),
    XERROR_WRITE                = (0-8)
} result_t;

/*****************************************/

typedef enum signal_e
{
    Low = 0,
    High
}  signal_e;

/*****************************************/
/*
 TODO: remove use of this type
 Four character constants
 */
typedef uint32_t					fcc_t;
#if (defined _WIN32)
typedef uint32_t					id_t;
#endif

/*
 Note: The byte order of four-character constants (4CCs) must be the
 same, regardless of whether it's read in from a file and treated as
 a stream of bytes or if it's stored in a 32-bit variable.
 
 * IMPORTANT *
 
 The byte order of 4CCs must NOT be switched upon reading in from a
 file. The following two macros are defined so swapping of bytes for
 a 4CC is not needed.
 */

#if (defined X_BIG_ENDIAN)
#define XFOURCC(c, c1, c2, c3)				\
(fcc_t)	(       (uint32_t)(uint8_t)(c3)			\
			| ( (uint32_t)(uint8_t)(c2) << 8 )	\
			| ( (uint32_t)(uint8_t)(c1) << 16 )	\
			| ( (uint32_t)(uint8_t)(c)  << 24 )	\
)
#else
#define XFOURCC(c, c1, c2, c3)				\
(fcc_t)	(       (uint32_t)(uint8_t)(c)			\
			| ( (uint32_t)(uint8_t)(c1) << 8 )	\
			| ( (uint32_t)(uint8_t)(c2) << 16 )	\
			| ( (uint32_t)(uint8_t)(c3) << 24 )	\
)
#endif

/*****************************************************************************/

#endif
