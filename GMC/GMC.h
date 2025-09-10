#pragma once
#include <string>

#ifdef GMC_EXPORTS
#define GMC_EXPORT extern "C" __declspec(dllexport)
#else
#define GMC_EXPORT
#endif

// FIXME: These typedefs are duplicated across more if not all projects and
// need to be consolidated in one place.
typedef void(*SETCART)(unsigned char);
typedef void(*SETCARTPOINTER)(SETCART);
typedef void(*DYNAMICMENUCALLBACK)(const char *, int, int);

std::string SelectROMFile();
std::string ExtractFilename(std::string path);

