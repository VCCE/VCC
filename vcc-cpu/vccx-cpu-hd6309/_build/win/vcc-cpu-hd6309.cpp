// vcc-cpu-HD6309.cpp : Defines the entry point for the DLL application.
//

//#include "stdafx.h"
#include "vcc-cpu-HD6309.h"

#ifdef _MANAGED
#pragma managed(push, off)
#endif

#include <Windows.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif

// This is an example of an exported variable
VCCCPUHD6309_API int nvcccpuHD6309 = 0;

// This is an example of an exported function.
VCCCPUHD6309_API int fnvcccpuHD6309(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see vcc-fd502.h for the class definition
CVCCCPUHD6309::CVCCCPUHD6309()
{
	return;
}
