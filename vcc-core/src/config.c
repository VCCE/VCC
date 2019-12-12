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

#include "config.h"

#include "file.h"
#include "path.h"

#include "_config.h"

#include <string.h>
#include <assert.h>
#include <limits.h>

/**
 */
config_t * confCreate(const char * documentPath)
{
    config_t * config = calloc(1,sizeof(config_t));
    if ( config != NULL )
    {
        config->absolutePaths = false;
        config->documentPath = documentPath != NULL ? strdup(documentPath) : NULL;
        if ( config->documentPath != NULL )
        {
            char temp[PATH_MAX];
            
            pathGetPathname(config->documentPath, temp, sizeof(temp)-1);
            
            config->documentDirectory = strdup(temp);
        }
        config->conf = _confCreate(documentPath);
    }
    
    return config;
}

/**
 */
result_t confDestroy(config_t * config)
{
    result_t result = XERROR_INVALID_PARAMETER;
    
    assert(config != NULL);
    if (config != NULL)
    {
        _confDestroy(config->conf);
        
        if ( config->documentPath != NULL )
        {
            free(config->documentPath);
        }
        
        if ( config->documentDirectory != NULL )
        {
            free(config->documentDirectory);
        }
        
        free(config);
        
        result = XERROR_NONE;
    }
    
    return result;
}

/**
 */
result_t confSave(config_t * config, const char * pathname)
{
    assert(config != NULL);
    
    return _confSave(config->conf,pathname);
}

/**
 */
result_t confSetString(config_t * config, const char * section, const char * key, const char * value)
{
    assert(config != NULL);
    
    return _confSetString(config->conf,section,key,value);
}

/**
 */
result_t confGetString(config_t * config, const char * section, const char * key, char * string, size_t stringSize)
{
    assert(config != NULL);
    
    return _confGetString(config->conf,section,key,string,stringSize);
}


/**
 */
result_t confSetInt(config_t * config, const char * section, const char * key, int value)
{
    assert(config != NULL);
    
    return _confSetInt(config->conf,section,key,value);
}

/**
 */
result_t confGetInt(config_t * config, const char * section, const char * key, int * outValue)
{
    assert(config != NULL);
    
    return _confGetInt(config->conf,section,key,outValue);
}


/**
 */
result_t confSetPath(config_t * config, const char * section, const char * key, const char * pathname, bool absolute)
{
    result_t result = XERROR_NONE;
    
    assert(config != NULL);
    
    if ( pathname != NULL )
    {
        char * relPath = NULL;
        const char * usePath = NULL;
        
        if ( absolute )
        {
            usePath = pathname;
        }
        else
        {
            pathGetRelativePath(config->documentDirectory, pathname, &relPath);
            usePath = relPath;
        }
        
        result = _confSetPath(config->conf,section,key,usePath);
        
        if (relPath!= NULL) free(relPath);
    }
    
    return result;
}

/**
 */
result_t confGetPath(config_t * config, const char * section, const char * key, char ** outPathname, bool absolute)
{
    assert(config != NULL);
    assert(outPathname != NULL);
    
    char * tmpPath = NULL;
    result_t result = _confGetPath(config->conf,section,key,&tmpPath);
    if (result == XERROR_NONE)
    {
        if ( pathIsAbsolute(tmpPath) )
        {
            *outPathname = calloc(1,PATH_MAX);
            strcpy(*outPathname,tmpPath);
        }
        else
        {
            result = pathCombine(config->documentDirectory, tmpPath, outPathname);
        }
        
        free(tmpPath);
    }
    
    return result;
}

