#ifndef __xDynLib_h_
#define __xDynLib_h_

/*****************************************************************************/
/*
 uEng - micro cross platform engine
 Copyright 2010-2011 by Wes Gale All rights reserved.
 */
/*****************************************************************************/

#include "xTypes.h"

/*****************************************************************************/

struct xdynlib_t
{
	/** OS specific module handle */
	void *	hModule;
};

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif
