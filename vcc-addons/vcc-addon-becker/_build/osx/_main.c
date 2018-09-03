/*
 *  _main.c
 *  VCC-X
 *
 *  Created by Wes Gale on 11-04-22.
 *  Copyright 2011 Magellan Interactive Inc. All rights reserved.
 *
 */

#include "xDebug.h"

/*
 Called when dylib is loaded
 */
__attribute__((constructor)) static void initializer()
{
	//XTRACE("VCC core .dylib initialization\n");
}

/*
 called when dlib is unloaded
 */
__attribute__((destructor)) static void finalizer()
{
	//XTRACE("VCC core .dylib destruction\n");
}
