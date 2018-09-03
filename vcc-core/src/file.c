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

#include "path.h"
#include "pakInterface.h"

#include "xstring.h"

#include <stdio.h>
#include <math.h>
#include <assert.h>

/**
 */
result_t fileGetExtension(const char * pPathname, char * pDst, size_t szDst)
{
    result_t result = XERROR_INVALID_PARAMETER;
    
    if (    pPathname != NULL
         && pDst != NULL
         && szDst > 0
        )
    {
        result = XERROR_NOT_FOUND;
        
        *pDst = 0;
        
        const char * ext = strrchr(pPathname,'.');
        if (ext != NULL)
        {
            size_t length = min(strlen(ext+1)+1,szDst-1);
            strncpy(pDst,ext+1,length);
            
            result = XERROR_NONE;
        }
    }
    
    return result;
}

/**
 */
result_t fileGetFilenameNoExtension(const char * pPathname, char * pDst, size_t szDst)
{
    result_t result = XERROR_INVALID_PARAMETER;
    
    if (    pPathname != NULL
        && pDst != NULL
        && szDst > 0
        )
    {
        result = XERROR_NONE;
        
        *pDst = 0;
        
        const char * filename = strrchrs(pPathname,"\\/");
        if (filename != NULL)
        {
            filename++;
        }
        else
        {
            filename = pPathname;
        }
        
        size_t extLen = 0;
        const char * ext = strrchr(filename,'.');
        if (ext!=NULL)
        {
            extLen = strlen(ext);
        }
        size_t length = min(strlen(filename)-extLen,szDst-1);
        strncpy(pDst,filename,length);
        pDst[length] = 0;
    }
    
    return result;
}

/**
    Create file if it does not exist at @path
 */
result_t fileTouch(const char * path)
{
    result_t result = XERROR_INVALID_PARAMETER;
    
    if ( path != NULL )
    {
        FILE * file = fopen(path,"a+");
        if (file != NULL)
        {
            fclose(file);
            
            result = XERROR_NONE;
        }
        else
        {
            result = XERROR_NOT_FOUND;
        }
    }
    
    return result;
}

/**
    Load a file's contents into memory
 
    @param ppBuffer A pointer to receive the point to the allocated buffer (Cannot be NULL), caller must release it
    @param pszBufferSize A pointer to receive the size of the buffer (can be NULL)
 */
result_t fileLoad(const char * pPathname, void ** ppBuffer, size_t * pszBufferSize)
{
    result_t    errResult    = XERROR_INVALID_PARAMETER;
    size_t      szBytes;
    
    if (    pPathname != NULL
         && ppBuffer != NULL
        )
    {
        errResult = XERROR_NOT_FOUND;
        
        FILE * hFile = fopen(pPathname,"r");
        if ( hFile != NULL )
        {
            errResult = XERROR_EOF;
            
            fseek(hFile,0,SEEK_END);
            szBytes  = ftell(hFile);
            fseek(hFile,0,SEEK_SET);
            if ( szBytes > 0 )
            {
                errResult = XERROR_OUT_OF_MEMORY;

                uint8_t *   pData = malloc(szBytes);
                if ( pData != NULL )
                {
                    if ( fread(pData, szBytes, 1, hFile) == 1 )
                    {
                        errResult = XERROR_NONE;

                        // pass back the buffer
                        *ppBuffer = pData;
                        
                        // optionally pass back the size
                        if ( pszBufferSize != NULL )
                        {
                            *pszBufferSize = szBytes;
                        }
                    }
                    else
                    {
                        errResult = XERROR_READ;
                        
                        free(pData);
                    }
                }
            }
            
            fclose(hFile);
        }
    }
    
    return errResult;
}

/*************************************************************************************/

filetype_t g_filetypelist[] =
{
    { 
		COCO_PAK_ROM,            
		"ROM",
		"CoCo ROM Pak",
        {
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
    { 
		COCO_PAK_ROM,            
		"BIN",
		"CoCo ROM Pak",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
    { 
		COCO_PAK_ROM,
		"CCC",
		"CoCo ROM Pak",
        {
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },

    // TODO: it needs to distinguish between the differetn types
#if (defined _MAC)
    { 
		COCO_PAK_PLUGIN,        
		"dylib",
		"VCC Plugin Pak Module",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
    { 
		COCO_CPU_PLUGIN,        
		"dylib",
		"VCC Plugin CPU Module",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
    {
		COCO_ADDONMOD,        
		"dylib",
		"VCC Plugin Add-on Module",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
#elif (defined _WIN32)
    { 
		COCO_PAK_PLUGIN,        
		"dll",
		"VCC Plugin Pak Module",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
    { 
		COCO_CPU_PLUGIN,        
		"dll",
		"VCC Plugin CPU Module",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
    { 
		COCO_ADDONMOD,        
		"dll",
		"VCC Plugin Add-on Module",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
#endif
    
    { 
		COCO_CASSETTE_CAS,    
		"cas",
		"CoCo Cassette",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
    { 
		COCO_CASSETTE_CAS,    
		"cassette",
		"CoCo Cassette",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
    { 
		COCO_CASSETTE_WAVE,    
		"wav",
		"CoCo Cassette",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
        },
    },
    { 
		COCO_CASSETTE_WAVE,    
		"wave",
		"CoCo Cassette",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },

    {	COCO_VDISK_FLOPPY,    
		"dsk",
		"Virtual disk",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
    { 
		COCO_VDISK_FLOPPY,    
		"os9",
		"Virtual disk",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
    { 
		COCO_VDISK_FLOPPY,    
		"vdk",
		"Virtual disk",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
    { 
		COCO_VDISK_FLOPPY,    
		"dmk",
		"Virtual disk",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
    { 
		COCO_VDISK_FLOPPY,    
		"jvc",
		"Virtual disk",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
    { 
		COCO_VDISK_FLOPPY,    
		"raw",  // TODO: raw access to floppy device that supports 256 byte sectors
		"Virtual disk",
		{
            {    0,    0 },
            {    0,    0 },
            {    0,    0 },
            {    0,    0 }
        },
    },
    
    { 
		COCO_FILE_NONE,        
		NULL,
        NULL,
		{
            {    0,    0 }
        }
    }
} ;

const char * sysGetFileTypeExtension(filetype_e fileType)
{
    filetype_t *    curType;
    
    curType = &g_filetypelist[0];
    while (   curType->type != COCO_FILE_NONE
           && curType->type != 0
           )
    {
        if ( fileType == curType->type)
        {
            return curType->pExt;
        }
        
        // next
        curType++;
    }
    
    return NULL;
}

filetype_e sysGetFileType(const char * pPathname)
{
    filetype_t *    pCurType;
    pair_t *        pCurPair;
    unsigned char   uc;
    int             iType    = COCO_FILE_NONE;
    char            temp[PATH_MAX];
    char            ext[64];
    
    if ( pPathname != NULL )
    {
        FILE * hFile = fopen(pPathname,"r");
        if ( hFile != NULL )
        {
            pathGetFilename(pPathname, temp, sizeof(temp));
            fileGetExtension(temp, ext, sizeof(ext));
            
            pCurType = &g_filetypelist[0];
            while (    iType == COCO_FILE_NONE
                   && pCurType ->type != 0
                   )
            {
                if (   strlen(ext) > 0
                    && pCurType->pExt != NULL
                    )
                {
                    // compare extension
                    if ( strcasecmp(ext, pCurType->pExt) == 0 )
                    {
                        iType = pCurType->type;
                        break;
                    }
                }
                else
                {
                    // if no extension, check all valid signature offset/value pairs
                    pCurPair = &pCurType->sig[0];
                    while (    pCurPair->offset != 0
                           || pCurPair->value != 0
                           )
                    {
                        fseek(hFile, pCurPair->offset, SEEK_SET);
                        
                        if ( fread(&uc,1,1,hFile) == 1 )
                        {
                            if ( uc == pCurPair->value )
                            {
                                iType = pCurType->type;
                                break;
                            }
                            
                            //[pData release];
                        }
                        
                        // next
                        pCurPair++;
                    }
                }
                
                // next
                pCurType++;
            }
            
            fclose(hFile);
        }
    }
    
    return iType;
}

