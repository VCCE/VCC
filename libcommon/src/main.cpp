// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>
#include <vcc/util/exports.h>
// NOTE: The following includes are here to force exporting implicit
// definitions.
#include <vcc/bus/null_cartridge.h>
#include <vcc/bus/dll_deleter.h>


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

