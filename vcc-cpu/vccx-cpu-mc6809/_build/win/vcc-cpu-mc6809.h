// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the  VCCCPUMC6809_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// VCCCPUMC6809_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef VCCCPUMC6809_EXPORTS
#define VCCCPUMC6809_API __declspec(dllexport)
#else
#define VCCCPUMC6809_API __declspec(dllimport)
#endif

// This class is exported from the vcc-cpu-mc6809.dll
class VCCCPUMC6809_API CVCCCPUMC6809 {
public:
	CVCCCPUMC6809(void);
	// TODO: add your methods here.
};

extern VCCCPUMC6809_API int nvcccpuMC6809;

VCCCPUMC6809_API int fnvcccpuMC6809(void);
