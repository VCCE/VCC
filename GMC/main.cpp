#include "gmc_cartridge.h"
#include <vcc/core/cartridge_factory.h>
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
extern "C" __declspec(dllexport) CreatePakFactoryFunction GetPakFactory()
{
	return [](
		[[maybe_unused]] std::unique_ptr<::vcc::core::cartridge_context> context,
		[[maybe_unused]] const cartridge_capi_context& capi_context) -> std::unique_ptr<::vcc::core::cartridge>
	{
		return std::make_unique<gmc_cartridge>(move(context), gModuleInstance);
	};
}
