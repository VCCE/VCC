#pragma once
#include <string>
#include "../ModuleDefs.h"

#ifdef GMC_EXPORTS
#define GMC_EXPORT extern "C" __declspec(dllexport)
#else
#define GMC_EXPORT
#endif

std::string SelectROMFile();
std::string ExtractFilename(std::string path);

