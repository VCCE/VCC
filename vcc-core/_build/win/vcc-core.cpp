#include "vcc-core.h"

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

/////////////////////////////////////////////////////////////////////////////
//
// Very temporary until more is pulled into the core from the main VCC EXE
//

extern "C" VCCCORE_API vccapi_setcart_t				vccCoreSetCart = NULL;
extern "C" VCCCORE_API vccapi_dynamicmenucallback_t	vccCoreDynamicMenuCallback = NULL;
extern "C" VCCCORE_API vccpakapi_assertinterrupt_t	vccCoreAssertInterrupt = NULL;
extern "C" VCCCORE_API vccpakapi_portread_t			vccCorePortRead = NULL;
extern "C" VCCCORE_API vccpakapi_portwrite_t		vccCorePortWrite = NULL;
extern "C" VCCCORE_API vcccpu_read8_t				vccCoreMemRead = NULL;
extern "C" VCCCORE_API vcccpu_write8_t				vccCoreMemWrite = NULL;

extern "C" VCCCORE_API vccapi_setresetpending_t		vccCoreSetResetPending = NULL;
extern "C" VCCCORE_API vccapi_getinifilepath_t		vccCoreGetINIFilePath = NULL;
extern "C" VCCCORE_API vccapi_loadpack_t			vccCoreLoadPack = NULL;;

extern "C" VCCCORE_API void *						g_vccCorehWnd = NULL;

/////////////////////////////////////////////////////////////////////////////
//
// Example exporting C++
//
/////////////////////////////////////////////////////////////////////////////

#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Example exporting C/C++
//
/////////////////////////////////////////////////////////////////////////////

#if 0

/////////////////////////////////////////////////////////////////////////////

// This is an example of an exported variable
VCCCORE_API int nvcccore=0;

/////////////////////////////////////////////////////////////////////////////

// This is an example of an exported function.
VCCCORE_API int fnvcccore(void)
{
	return 42;
}

/////////////////////////////////////////////////////////////////////////////

// This is the constructor of a class that has been exported.
// see vcc-core.h for the class definition
Cvcccore::Cvcccore()
{
	return;
}

/////////////////////////////////////////////////////////////////////////////

#endif

/////////////////////////////////////////////////////////////////////////////
