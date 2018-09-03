// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the VCCPAKFD502_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// VCCPAKFD502_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef VCCPAKFD502_EXPORTS
#define VCCPAKFD502_API __declspec(dllexport)
#else
#define VCCPAKFD502_API __declspec(dllimport)
#endif

// This class is exported from the vcc-pak-fd502.dll
class VCCPAKFD502_API Cvccpakfd502 {
public:
	Cvccpakfd502(void);
	// TODO: add your methods here.
};

extern VCCPAKFD502_API int nvccpakfd502;

VCCPAKFD502_API int fnvccpakfd502(void);
