// NOTE: The following include is here to force exporting implicit definitions.
#include <vcc/utils/dll_deleter.h>
#include <Windows.h>


BOOL APIENTRY DllMain(
	[[maybe_unused]] HMODULE hModule,
	[[maybe_unused]] DWORD  ul_reason_for_call,
	[[maybe_unused]] LPVOID lpReserved)
{
	return TRUE;
}