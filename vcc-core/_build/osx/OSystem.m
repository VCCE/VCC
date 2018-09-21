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

#include "file.h"

#include "xstring.h"
#include "slinklist.h"
#include "path.h"
#include "file.h"
#include "xsystem.h"

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#import "OSystem.h"

@implementation OSystem

@end

/*************************************************************************************/

void sysInit(void)
{
    
}

void sysNanoSleep(uint64_t nanoseconds)
{
    struct timespec spec;
    struct timespec end;
    spec.tv_sec = 0;
    spec.tv_nsec = nanoseconds;
    nanosleep(&spec,&end);
}


void sysSleep(uint32_t timeInMs)
{
    uint64_t nanoseconds = timeInMs * 1000000;
    sysNanoSleep(nanoseconds);
}

/*************************************************************************************/

void sysShowError(const char * pcszText)
{
    NSString *    pString    = [NSString stringWithCString:pcszText encoding:NSASCIIStringEncoding];
    NSAlert *    pAlert    = [NSAlert alertWithMessageText:@"Error" defaultButton:@"OK" alternateButton:nil otherButton:nil informativeTextWithFormat:@"%@",pString];
    
    [pAlert runModal];
    
    //[pAlert release];
}

/*************************************************************************************/

/**
 Get the path to the app for loading internal modules
 returned string must be released
 */
result_t sysGetAppPath(char ** ppPath)
{
    result_t result = XERROR_GENERIC;
    
    if ( ppPath != NULL )
    {
        NSString * str = [[NSBundle mainBundle].bundlePath stringByDeletingLastPathComponent];
        const char * pstr = [str cStringUsingEncoding:NSUTF8StringEncoding];
        *ppPath = strdup(pstr);
        result = XERROR_NONE;
    }
    
    return result;
}

result_t sysGetPluginPath(char ** ppPath)
{
    result_t result = XERROR_GENERIC;
    
    if ( ppPath != NULL )
    {
        NSString * str = [[NSBundle mainBundle] privateFrameworksPath];
        const char * pstr = [str cStringUsingEncoding:NSUTF8StringEncoding];
        *ppPath = strdup(pstr);
        result = XERROR_NONE;
    }
    
    return result;
}

/**
 returned string must be released
 */
result_t sysGetResourcePath(char ** ppPath)
{
    result_t result = XERROR_GENERIC;
    
    if ( ppPath != NULL )
    {
        NSString * str = [[NSBundle mainBundle] resourcePath];
        const char * pstr = [str cStringUsingEncoding:NSUTF8StringEncoding];
        *ppPath = strdup(pstr);
        result = XERROR_NONE;
    }
    
    return result;
}

/**
    pDst is returned
 */
char * sysGetPathForResource(char * filename, char * pDst, size_t sizeDst)
{
    char cFilename[PATH_MAX];
    char cExtension[PATH_MAX];
    
    fileGetFilenameNoExtension(filename, cFilename, sizeof(cFilename)-1);
    fileGetExtension(filename, cExtension, sizeof(cExtension)-1);
    
    NSBundle * myBundle = [NSBundle mainBundle];
    NSString * nsFilename = [[NSString alloc] initWithUTF8String:cFilename];
    NSString * nsExtension = [[NSString alloc] initWithUTF8String:cExtension];
    NSString * absPath= [myBundle pathForResource:nsFilename ofType:nsExtension];
    
    if ( absPath != nil )
    {
        const char * path = [absPath cStringUsingEncoding:NSUTF8StringEncoding];
        
        if ( path != NULL )
        {
            strncpy(pDst , path, sizeDst-1);
        
            return pDst;
        }
    }
    
    return NULL;
}

filelist_t * sysFileListCreate()
{
    return calloc(1, sizeof(filelist_t));
}

/**
 */
filelist_t * sysFileListGetFolderContents(const char * path, bool includeSubFolders)
{
    filelist_t * fileList = NULL;
    
    if ( path != NULL )
    {
        int count = -1;
        DIR * dp;
        struct dirent * ep;
        
        // count
        dp = opendir (path);
        if (dp != NULL)
        {
            count = 0;
            
            while ( (ep = readdir(dp)) != NULL )
            {
                if (    strcasecmp(ep->d_name,".") == 0
                     || strcasecmp(ep->d_name,"..") == 0
                    )
                {
                    continue;
                }
                
                count++;
            }
            
            (void)closedir (dp);
        }
        
        // copy
        dp = opendir (path);
        if (dp != NULL)
        {
            if ( count >= 0 )
            {
                fileList = sysFileListCreate();
                fileList->path = strdup(path);
                
                while ( (ep = readdir(dp)) != NULL )
                {
                    if (   strcasecmp(ep->d_name,".") == 0
                        || strcasecmp(ep->d_name,"..") == 0
                        )
                    {
                        continue;
                    }
                    
                    fileentry_t * file = calloc(1,sizeof(fileentry_t));
                    if ( file )
                    {
                        file->name = strdup(ep->d_name);
                        file->isFolder = ((ep->d_type & DT_DIR) != 0);
                        
                        slinklistAddToTail(&fileList->list, &file->link);
                        
                        // also scan sub-folders?
                        if (    file->isFolder
                             && includeSubFolders
                            )
                        {
                            char * subPath = NULL;
                            
                            pathCombine(path, file->name, &subPath);
                            file->contents = sysFileListGetFolderContents(subPath,includeSubFolders);
                            file->parent = fileList;
                            
                            strfreesafe(subPath);
                        }
                    }
                }
            }
            
            (void)closedir (dp);
        }
    }
    
    return fileList;
}

void sysFileListDestroy(filelist_t * fileList)
{
    if ( fileList != NULL )
    {
        assert(false);
    }
}

fileentry_t * sysFileListEntryCopy(fileentry_t * fileEntry)
{
    assert(false);
}

void sysFileListEntryDestroy(fileentry_t * fileEntry)
{
    assert(false);
}



void sysSetPreference(const char * key, const char * value)
{
    NSString * nsKey = [[NSString alloc] initWithUTF8String:key];
    NSString * nsValue = [[NSString alloc] initWithUTF8String:value];

    NSUserDefaults * defaults= [NSUserDefaults standardUserDefaults];
    [defaults setValue:nsValue forKey:nsKey];
}

const char * sysGetPreference(const char * key)
{
    NSString * nsKey = [[NSString alloc] initWithUTF8String:key];
    NSUserDefaults *defaults= [NSUserDefaults standardUserDefaults];
    NSDictionary * defaultsDictionary = [defaults dictionaryRepresentation];
    
    if ( [[defaultsDictionary allKeys] containsObject:nsKey] )
    {
        NSString * nsValue = [defaults valueForKey:nsKey];
        if ( nsValue != NULL )
        {
            return [nsValue cStringUsingEncoding:NSUTF8StringEncoding];
        }
    }

    return NULL;
}


/*************************************************************************************/

