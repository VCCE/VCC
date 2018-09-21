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

#ifndef xSystem_h
#define xSystem_h

#include "xTypes.h"
#include "slinklist.h"

typedef struct filelist_t filelist_t;

/**
 Folder directory list
 */
typedef struct fileentry_t
{
    slink_t link;
    
    char * name;
    
    filelist_t * parent;
    bool isFolder;
    filelist_t * contents;  // if a directory
    
    char * UserDesc;
    int userType;
} fileentry_t;

/**
 Linked list of fileentry_t
 */
typedef struct filelist_t
{
    slinklist_t list;
    
    char * path;
} filelist_t;

#ifdef __cplusplus
extern "C"
{
#endif
    
    void sysInit(void);
	void sysTerm(void);

    void sysNanoSleep(uint64_t nanoseconds);

	void sysShowError(const char * pcszText);

    result_t sysGetAppPath(char ** ppPath);
    result_t sysGetResourcePath(char ** ppPath);
    result_t sysGetPluginPath(char ** ppPath);
	char * sysGetPathForResource(char * filename, char * pDst, size_t sizeDst);

    filelist_t * sysFileListCreate(void);
    filelist_t * sysFileListGetFolderContents(const char * path, bool includeSubFolders);
    void sysFileListDestroy(filelist_t * fileList);
    fileentry_t * sysFileListEntryCopy(fileentry_t * fileEntry);
    void sysFileListEntryDestroy(fileentry_t * fileEntry);

    void sysSetPreference(const char * key, const char * value);
    const char * sysGetPreference(const char * key);
    
#ifdef __cplusplus
}
#endif


#endif /* xSystem_h */
