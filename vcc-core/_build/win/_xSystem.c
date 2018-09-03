/*****************************************************************************/
/*
	System functions
*/
/*****************************************************************************/

#include "xSystem.h"

#if !(defined _WIN32)
#	error "WINDOWS implementation"
#endif


#include <Windows.h>
#include <assert.h>

void sysInit(void)
{
	// Docs: https://docs.microsoft.com/en-ca/windows/desktop/api/timeapi/nf-timeapi-timebeginperiod

	// Windows Sleep() defaults to taking about 15ms
	// use the WinMM lib to change the default period
	// Ref: https://stackoverflow.com/questions/23258650/sleep1-and-sdl-delay1-takes-15-ms

	// TODO: this may not be the best way of dealing with this...

	/* wait for some event to start this thread code */
	timeBeginPeriod(1);                 /* set period to 1ms */
	Sleep(128);                         /* wait for it to stabilize */
}

void sysTerm(void)
{
	// restore
	timeEndPeriod(1);
}

void sysSleep(uint32_t timeInMs)
{
	Sleep(timeInMs);
}

void sysShowError(const char * pcszText)
{
	assert(0);
}


char * sysGetPathResources(char * filename, char * pDst, size_t sizeDst)
{
	assert(0);
	return NULL;
}

