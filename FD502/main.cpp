#include "fd502.h"
#include <vcc/bus/cartridge_factory.h>


HINSTANCE gModuleInstance;


BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpReserved )  // reserved
{
	if (fdwReason == DLL_PROCESS_ATTACH) //Clean Up
	{
		gModuleInstance = hinstDLL;
	}

	return TRUE;
}


extern "C" __declspec(dllexport) ::vcc::bus::CreatePakFactoryFunction GetPakFactory()
{
	return [](
		std::unique_ptr<::vcc::bus::cartridge_context> context) -> std::unique_ptr<::vcc::bus::cartridge>
		{
			return std::make_unique<fd502_cartridge>(move(context), gModuleInstance);
		};
}

static_assert(
	std::is_same_v<decltype(&GetPakFactory), ::vcc::bus::GetPakFactoryFunction>,
	"FD502 GetPakFactory does not have the correct signature.");

