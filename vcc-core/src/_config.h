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

/*****************************************************************************/
//
//  _config.h
//
/*****************************************************************************/

#ifndef __config_h_
#define __config_h_

#include "config.h"

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
    
    confhandle_t    _confCreate(const char * pathname);
    result_t        _confDestroy(confhandle_t);

    result_t        _confSave(confhandle_t confHandle, const char * pathname);
    
    result_t        _confSetString(confhandle_t confHandle, const char * section, const char * key, const char * pValue);
    result_t        _confGetString(confhandle_t confHandle, const char * section, const char * key, char * pszString, size_t stringSize);
    
    result_t        _confSetInt(confhandle_t confHandle, const char * section, const char * key, int value);
    result_t        _confGetInt(confhandle_t confHandle, const char * section, const char * key, int * outValue);
    
    result_t        _confSetPath(confhandle_t confHandle, const char * section, const char * key, const char * pathname);
    result_t        _confGetPath(confhandle_t confHandle, const char * section, const char * key, char ** outPathname);

#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif /* __config_h_ */
