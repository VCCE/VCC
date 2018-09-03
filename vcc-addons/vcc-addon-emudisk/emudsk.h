#ifndef _EMUDSK_H_
#define _EMUDSK_H_

/***************************************************************************/

#include "xTypes.h"

/***************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

	extern char			g_szModuleName[MAX_LOADSTRING];
	extern char			g_szCatNumber[MAX_LOADSTRING];

	void EmuDskInit();
	void EmuDskTerm();
	void EmuDskConfig();

	void SaveConfig(void);
	unsigned char EnableCloud9RTC(unsigned char Tmp);

#ifdef __cplusplus
}
#endif

/***************************************************************************/

#endif // _EMUDSK_H_
