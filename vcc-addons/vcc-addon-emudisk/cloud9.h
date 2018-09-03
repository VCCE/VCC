#ifndef _CLOUD9_H_
#define _CLOUD9_H_

/***************************************************************************/

//#include "xTypes.h"

/***************************************************************************/

#define CLOCK_PORT	0x78

/***************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

	extern int bCloud9RTCEnable;
	
	unsigned char ReadTime(unsigned short);

#ifdef __cplusplus
}
#endif

/***************************************************************************/

#endif // _CLOUD9_H_
