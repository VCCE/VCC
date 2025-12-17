#pragma once
#include <string>
#include "../CartridgeMenu.h"
#include <vcc/bus/cpak_cartridge_definitions.h>

#ifdef GMC_EXPORTS
#define GMC_EXPORT extern "C" __declspec(dllexport)
#else
#define GMC_EXPORT
#endif

extern HINSTANCE gModuleInstance;
std::string SelectROMFile();
std::string ExtractFilename(std::string path);

