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

/*************************************************************************************/
/*
	OFile.m
*/
/*************************************************************************************/
/*
	TODO: add exception handling to catch file i/o errors
 */
/*************************************************************************************/

#import "OFile.h"

#include "file.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************/
/**
    @param fileTypes Types array terminated with the type COCO_FILE_NONE
 */
NSArray * convertFileTypesArray(filetype_e * fileTypes)
{
    NSMutableArray * types = NULL;
    filetype_t *    curType;
    
    if (fileTypes != NULL)
    {
        types = [[NSMutableArray alloc] init];
        
        // walk user specified types
        int i = 0;
        while (fileTypes[i]!=COCO_FILE_NONE)
        {
            // look for all entries that match
            curType = &g_filetypelist[0];
            while (   curType->type != COCO_FILE_NONE
                   && curType->type != 0
                   )
            {
                filetype_e fileType = fileTypes[i];
                
                if ( fileType == curType->type )
                {
                    // add the extension for this type
                    NSString * ext = [NSString stringWithUTF8String:curType->pExt];
                    [types addObject:ext];
                }
                
                // next
                curType++;
            }
            
            i++;
        }
    }
    
    return types;
}

/*************************************************************************************/
/**
	Using platform specific UI, ask the user for a file (platform specific pathname 
	object).  
 
	Used for loading external ROM, ROM Pak, device Pak, etc
 */
char * sysGetPathnameFromUser(filetype_e * fileTypes, const char * pStartPath)
{
	NSInteger		result;
	NSOpenPanel *	oPanel		= [NSOpenPanel openPanel];
	//NSArray *		fileTypes	= [NSArray arrayWithObject:@"td"];
	NSArray *		types	    = convertFileTypesArray(fileTypes);
	char *		    pPathname	= NULL;
	//NSString *		pFirstFolder;
	
#if 0
	if ( pStartPath != nil )
	{
		pFirstFolder = [(NSURL *)pStartPath path];
		NSHomeDirectory()
	}
#endif
	
	/* 
        initialize the open panel
	 */
	[oPanel setAllowsMultipleSelection:NO];
	[oPanel setCanChooseDirectories:NO];
	[oPanel setResolvesAliases:YES];
	[oPanel setCanChooseFiles:YES];
    //[oPanel setDirectoryURL: ];
    [oPanel setAllowedFileTypes:types];
  
	/*
	 show open panel
	 */
	result = [oPanel runModal];
	if (result == NSModalResponseOK)
	{
		NSArray *	filesToOpen = [oPanel URLs];
		NSInteger	i;
		NSInteger	count = [filesToOpen count];
		NSURL *		pURL;
		
		assert(count == 1);
		
        // TODO: memory leak if more than one
		for (i=0; i<count; i++) 
		{
			pURL = [filesToOpen objectAtIndex:i];
            //NSString * p = [pURL absoluteString];
            char temp[256];
            /* bool b = */ [pURL getFileSystemRepresentation:temp maxLength:sizeof(temp)];
			pPathname = strdup(temp);
		}
		
		//[filesToOpen release];
	}
	
	return pPathname;
}

/**
 */
char * sysGetSavePathnameFromUser(filetype_e * fileTypes, const char * pStartPath)
{
    NSInteger        result;
    NSSavePanel *    oPanel        = [NSSavePanel savePanel];
    NSArray *        types        = convertFileTypesArray(fileTypes);
    char *            pPathname    = NULL;
    //NSString *        pFirstFolder;
    
#if 0
    if ( pStartPath != nil )
    {
        pFirstFolder = [(NSURL *)pStartPath path];
        NSHomeDirectory()
    }
#endif
    
    /*
     initialize the open panel
     */
    oPanel.canCreateDirectories = YES;
    oPanel.allowsOtherFileTypes = YES;
    [oPanel setAllowedFileTypes:types];
    
   [oPanel setNameFieldStringValue:@"vccScreenShot.tiff"];
    
    /*
     show open panel
     */
    result = [oPanel runModal];
    if (result == NSModalResponseOK)
    {
        NSURL *     pURL = [oPanel URL];
        char        temp[PATH_MAX];
        
        /* bool b = */ [pURL getFileSystemRepresentation:temp maxLength:sizeof(temp)];
        pPathname = strdup(temp);
    }
    
    return pPathname;
}

/*************************************************************************************/

