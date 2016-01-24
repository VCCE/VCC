#ifndef __VCCCORE_H_
#define __VCCCORE_H_

/////////////////////////////////////////////////////////////////////////////

#include "vccPakAPI.h"

/////////////////////////////////////////////////////////////////////////////
//
// Functions we need from VCC
//
// These are temporary until more is pulled into the library
//

typedef void(*vccapi_setresetpending_t)(int);
typedef void(*vccapi_getinifilepath_t)(char *);
typedef void(*vccapi_loadpack_t)(void);

#ifdef __cplusplus
extern "C" {
#endif

	VCCCORE_API vccapi_setcart_t				vccCoreSetCart;
	VCCCORE_API vccapi_dynamicmenucallback_t	vccCoreDynamicMenuCallback;
	VCCCORE_API vccpakapi_assertinterrupt_t		vccCoreAssertInterrupt;
	VCCCORE_API vccpakapi_portread_t			vccCorePortRead;
	VCCCORE_API vccpakapi_portwrite_t			vccCorePortWrite;
	VCCCORE_API vcccpu_read8_t					vccCoreMemRead;
	VCCCORE_API vcccpu_write8_t					vccCoreMemWrite;

	VCCCORE_API vccapi_setresetpending_t		vccCoreSetResetPending;
	VCCCORE_API vccapi_getinifilepath_t			vccCoreGetINIFilePath;
	VCCCORE_API vccapi_loadpack_t				vccCoreLoadPack;

	VCCCORE_API void *							g_vccCorehWnd;

#ifdef __cplusplus
}
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Example exporting C++
//
/////////////////////////////////////////////////////////////////////////////

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the VCCCORE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// VCCCORE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
//#ifdef VCCCORE_EXPORTS
//#define VCCCORE_API __declspec(dllexport)
//#else
//#define VCCCORE_API __declspec(dllimport)
#//endif

#if 0

// This class is exported from the vcc-core.dll
class VCCCORE_API Cvcccore {
public:
	Cvcccore(void);
	// TODO: add your methods here.
};

extern VCCCORE_API int nvcccore;

VCCCORE_API int fnvcccore(void);

#endif

#endif // __VCCCORE_H_

