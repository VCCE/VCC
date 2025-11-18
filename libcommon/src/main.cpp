// NOTE: The following include is here to force exporting implicit definitions.
#include <vcc/utils/dll_deleter.h>
#include <Windows.h>

HINSTANCE gModuleInstance = nullptr;

#ifndef USE_STATIC_LIB
BOOL APIENTRY DllMain(HMODULE module_handle, DWORD  reason, LPVOID reserved)
{
    if(reason == DLL_PROCESS_ATTACH)
    {
		gModuleInstance = module_handle;
    }

	return TRUE;
}
#else
void InitStaticModuleInstance(HINSTANCE hInstance) {
    gModuleInstance = hInstance;
}
#endif
