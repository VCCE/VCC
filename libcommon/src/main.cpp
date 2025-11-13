// NOTE: The following two includes are here to force exporting implicit definitions.
#include <vcc/utils/dll_deleter.h>
#include <Windows.h>


HINSTANCE gModuleInstance = nullptr;


BOOL APIENTRY DllMain(HMODULE module_handle, DWORD  reason, LPVOID reserved)
{
    if(reason == DLL_PROCESS_ATTACH)
    {
		gModuleInstance = module_handle;
    }

	return TRUE;
}