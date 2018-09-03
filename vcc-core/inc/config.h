/*
 Copyright 2015 by Joseph Forgione
 This file is part of VCC (Virtual Color Computer).
 
 VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _config_h_
#define _config_h_

/*****************************************************************************/

#include "xTypes.h"

/*****************************************************************************/

/**
    system specific config handle
 */
typedef void *    confhandle_t;

/**
 */
typedef struct config_t
{
    confhandle_t    conf;               // system specific config handle
    char *          documentPath;       // full document path
    char *          documentDirectory;  // document folder
    bool            absolutePaths;      // use absolute or relative paths
} config_t;

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	config_t *	    confCreate(const char * documentPath);
	result_t		confSave(config_t * config, const char * pathname);
	result_t		confDestroy(config_t * config);
	
	result_t		confSetString(config_t * config, const char * section, const char * key, const char * pValue);
	result_t		confGetString(config_t * config, const char * section, const char * key, char * pszString, size_t stringSize);
	
	result_t		confSetInt(config_t * config, const char * section, const char * key, int value);
	result_t		confGetInt(config_t * config, const char * section, const char * key, int * outValue);

	result_t		confSetPath(config_t * config, const char * section, const char * key, const char * pathname, bool absolute);
	result_t		confGetPath(config_t * config, const char * section, const char * key, char ** outPathname, bool absolute);
	
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif // _config_h_
