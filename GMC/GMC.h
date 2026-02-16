#pragma once
#include <string>
#include <vcc/bus/cpak_cartridge_definitions.h>
#include <vcc/bus/cartridge_menuitem.h>

#ifdef GMC_EXPORTS
#define GMC_EXPORT extern "C" __declspec(dllexport)
#else
#define GMC_EXPORT
#endif

extern HINSTANCE gModuleInstance;
std::string SelectROMFile();
std::string ExtractFilename(std::string path);

