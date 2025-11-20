// NOTE: The following include is here to force exporting implicit definitions.
#include <vcc/utils/dll_deleter.h>
#include <Windows.h>

HINSTANCE gModuleInstance = nullptr;

#ifdef USE_STATIC_LIB
// Static library has no DLLmain but may require caller's hInstance
void InitStaticModuleInstance(HINSTANCE hInstance) {
    gModuleInstance = hInstance;
}
#else
BOOL APIENTRY DllMain(HMODULE module_handle, DWORD  reason, LPVOID reserved)
{
    if(reason == DLL_PROCESS_ATTACH)
    {
		gModuleInstance = module_handle;
    }

	return TRUE;
}
#endif
