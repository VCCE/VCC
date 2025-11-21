// NOTE: The following two includes are here to force exporting implicit definitions.
#include "vcc/utils/dll_deleter.h"
#include <Windows.h>


HINSTANCE gModuleInstance = nullptr;


BOOL APIENTRY DllMain(
	HINSTANCE module_instance,
	DWORD call_reason,
	[[maybe_unused]] LPVOID reserved)
{
    if(call_reason == DLL_PROCESS_ATTACH)
    {
		gModuleInstance = module_instance;
    }

	return TRUE;
}