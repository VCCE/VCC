/*****************************************************************************/
/**
	xAssert.c
 
	uEng - micro cross platform engine
	Copyright 2010 by Wes Gale All rights reserved.
	Used by permission in VccX
*/
/*****************************************************************************/

#include "xAssert.h"
#include "xDebug.h"

/*****************************************************************************/

XAPI void XCALL _xAssert(void * pFunc, void * pFilename, unsigned uLine)
{
	xDbgBreak();
}

/*****************************************************************************/
