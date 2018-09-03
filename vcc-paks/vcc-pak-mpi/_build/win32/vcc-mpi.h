// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the VCCMPI_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// VCCMPI_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef VCCMPI_EXPORTS
#define VCCMPI_API __declspec(dllexport)
#else
#define VCCMPI_API __declspec(dllimport)
#endif

// This class is exported from the vcc-mpi.dll
class VCCMPI_API Cvccmpi {
public:
	Cvccmpi(void);
	// TODO: add your methods here.
};

extern VCCMPI_API int nvccmpi;

VCCMPI_API int fnvccmpi(void);
