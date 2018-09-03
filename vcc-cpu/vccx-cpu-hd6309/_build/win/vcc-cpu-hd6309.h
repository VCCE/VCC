// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the  VCCCPUHD6309_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// VCCCPUHD6309_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef VCCCPUHD6309_EXPORTS
#define VCCCPUHD6309_API __declspec(dllexport)
#else
#define VCCCPUHD6309_API __declspec(dllimport)
#endif

// This class is exported from the vcc-cpu-hd6309.dll
class VCCCPUHD6309_API CVCCCPUHD6309 {
public:
	CVCCCPUHD6309(void);
	// TODO: add your methods here.
};

extern VCCCPUHD6309_API int nvcccpuHD6309;

VCCCPUHD6309_API int fnvcccpuHD6309(void);
