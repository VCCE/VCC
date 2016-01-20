#ifndef _xTypes_h_
#define _xTypes_h_

/*****************************************************************************/

#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/*****************************************************************************/

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

/*****************************************************************************/
/*
	cross platform defines
*/

#if (defined _LIB)
#	if (defined WIN32)
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

#if (defined _WINDOWS)
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

/*****************************************************************************/

#ifndef NULL
#	ifdef __cplusplus
#		define NULL    0
#	else
#		define NULL    ((void *)0)
#	endif
#endif

/*****************************************************************************/
/*
	cross platform types
 */
typedef long				bool_t;
typedef long				int_t;
typedef unsigned long		uint_t;
typedef unsigned long		result_t;

typedef signed char			s8_t;
typedef signed short		s16_t;
typedef signed int			s32_t;
typedef signed long long	s64_t;

typedef unsigned char		u8_t;
typedef unsigned short		u16_t;
typedef unsigned int		u32_t;
typedef unsigned long long	u64_t;

typedef float				f32_t;
typedef double				f64_t;

typedef unsigned char		byte_t;
typedef unsigned char *		pbyte_t;
typedef double				real_t;

typedef void *				pvoid_t;
typedef void *				handle_t;

typedef char				char_t;

typedef char				nchar_t;
//typedef unsigned short	whar_t;	// defined in wchar.h

typedef f32_t				decimal_t;

/*
	platform specific object pointers
 */
typedef void *				ppathname_t;

/*
	string and character wrappers for Unicode build
*/
#define XCHAR(t)	t
#define XTEXT(t)	t

#define XNCHAR(t)	t
#define XNTEXT(t)	t

#define XWCHAR(t)	L ## t
#define XWTEXT(t)	L ## t

/* 
	number of characters in string less terminator 
 (does not work with dynamically alloc'd strings) 
*/
#define XTEXT_NUM_CHARS(s)		( (sizeof(s) - 1) / sizeof(char_t) )
#define XTEXT_NUM_BYTES(s)		( sizeof(s) - sizeof(char_t) )

#ifndef TRUE
#	define TRUE (1)
#endif

#ifndef FALSE
#	define FALSE (0)
#endif

/*****************************************/
/*
	result_t error codes
*/
#define XERROR_NONE					(0)
#define XERROR_GENERIC				(0-1)
#define XERROR_INVALID_PARAMETER	(0-2)
#define XERROR_BUFFER_OVERFLOW		(0-3)
#define XERROR_OUT_OF_MEMORY		(0-4)
#define XERROR_NOT_FOUND			(0-5)
#define XERROR_EOF					(0-6)
#define XERROR_READ					(0-7)
#define XERROR_WRITE				(0-8)

/*****************************************/
/*
 Four character constants
 */
typedef u32_t					fcc_t;
#if (defined _WINDOWS)
typedef u32_t					id_t;
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
(fcc_t)	(       (u32_t)(u8_t)(c3)			\
			| ( (u32_t)(u8_t)(c2) << 8 )	\
			| ( (u32_t)(u8_t)(c1) << 16 )	\
			| ( (u32_t)(u8_t)(c)  << 24 )	\
)
#else
#define XFOURCC(c, c1, c2, c3)				\
(fcc_t)	(       (u32_t)(u8_t)(c)			\
			| ( (u32_t)(u8_t)(c1) << 8 )	\
			| ( (u32_t)(u8_t)(c2) << 16 )	\
			| ( (u32_t)(u8_t)(c3) << 24 )	\
)
#endif

#if (defined WIN32)
#	define Stringize( L )			#L
#	define MakeString( M, L )		M(L)
#	define $Line					\
		MakeString( Stringize, __LINE__ )
#	define REMINDER				\
		__FILE__ "(" $Line ") : Reminder: "
#	define TODO				\
		__FILE__ "(" $Line ") : TODO: "

// To use include the following line somewhere in your code
// #pragma message(REMINDER "Fix this problem!")
// #pragma message(TODO "Fix this problem!")
#endif

/*****************************************************************************/

#endif
