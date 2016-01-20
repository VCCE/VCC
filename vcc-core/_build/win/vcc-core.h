#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the VCCCORE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// VCCCORE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef VCCCORE_EXPORTS
#define VCCCORE_API __declspec(dllexport)
#else
#define VCCCORE_API __declspec(dllimport)
#endif

// This class is exported from the vcc-core.dll
class VCCCORE_API Cvcccore {
public:
	Cvcccore(void);
	// TODO: add your methods here.
};

extern VCCCORE_API int nvcccore;

VCCCORE_API int fnvcccore(void);
