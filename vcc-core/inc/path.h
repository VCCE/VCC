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

#ifndef path_h
#define path_h

/*****************************************************************************/

#include "xTypes.h"

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
    
    bool            pathIsAbsolute(const char * path);
    bool            pathIsFile(const char * path);
    bool            pathIsDirectory(const char * path);

    result_t        pathGetRelativePath(const char * base, const char * path, char ** outPath);
    result_t        pathGetAbsolutePath(const char * base, const char * path, char ** outPath);
    result_t        pathCombine(const char * base, const char * subPath, char ** outPath);
    
    bool            pathIsAbsolute(const char * pathanme);
    
    void            pathCanonicalize(char *path);
    //int             pathcanon(const char * srcpath, char * dstpath, size_t sz);

    result_t        pathGetFilename(const char * pPathname, char * pDst, size_t szDst);
    result_t        pathGetPathname(const char * pPathname, char * pDst, size_t szDst);
    
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif /* path_h */
