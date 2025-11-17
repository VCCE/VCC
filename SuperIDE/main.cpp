#include "SuperIDE.h"
#include "vcc/bus/cartridge_factory.h"
#include <Windows.h>


static HINSTANCE gModuleInstance;


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		gModuleInstance = hinstDLL;
	}

	return true;
}

//
extern "C" __declspec(dllexport) ::vcc::bus::CreatePakFactoryFunction GetPakFactory()
{
	return [](
		std::unique_ptr<::vcc::bus::cartridge_context> context) -> std::unique_ptr<::vcc::bus::cartridge>
		{
			return std::make_unique<superide_cartridge>(move(context), gModuleInstance);
		};
}

static_assert(
	std::is_same_v<decltype(&GetPakFactory), ::vcc::bus::GetPakFactoryFunction>,
	"SuperIDE GetPakFactory does not have the correct signature.");
