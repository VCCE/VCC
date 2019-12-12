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
/**
	OConfig.m
	
	Mac OS X / Cocoa implementation of VCC config API which is vaguely
        based on the original Win32 API that was used.
 
    This version will save VCC config files in plist format.
*/
/*****************************************************************************/

#import "OConfig.h"

#include "config.h"

#include "xDebug.h"

#include <stdio.h>
#include <string.h>

#if 0
CFURLRef CFURLCreateWithFileSystemPathRelativeToBase (
													  CFAllocatorRef allocator,
													  CFStringRef filePath,
													  CFURLPathStyle pathStyle,
													  Boolean isDirectory,
													  CFURLRef baseURL
													  );

CFURLRef CFURLCreateAbsoluteURLWithBytes (
										  CFAllocatorRef alloc,
										  const UInt8 *relativeURLBytes,
										  CFIndex length,
										  CFStringEncoding encoding,
										  CFURLRef baseURL,
										  Boolean useCompatibilityMode
										  );

CFURLRef CFURLCreateFromFileSystemRepresentationRelativeToBase (
																CFAllocatorRef allocator,
																const UInt8 *buffer,
																CFIndex bufLen,
																Boolean isDirectory,
																CFURLRef baseURL
																);
#endif

/*****************************************************************************/
/*****************************************************************************/
/**
	Add / get a 'section' from the config - minics WIN32 API for INI save/load
 */

/*****************************************************************************/
/**
 */
NSMutableDictionary * getSection(OConfig * pConfig, NSString * pStrSection)
{
	NSMutableDictionary *	section;

	assert(pConfig != NULL);
	assert(pStrSection != NULL);
	
	section = [pConfig valueForKey:pStrSection];
	
	return section;
}

/*****************************************************************************/
/**
 */
NSMutableDictionary * addSection(OConfig * pConfig, NSString * pStrSection)
{
	NSMutableDictionary *	section;
	
	assert(pConfig != NULL);
	assert(pStrSection != NULL);
	
	section = [pConfig valueForKey:pStrSection];
	if ( section == nil )
	{
		section = [[NSMutableDictionary alloc] init];
		
		[pConfig setObject:(id)section forKey:pStrSection];
		
		assert(section == [pConfig valueForKey:pStrSection]);
	}
	
	return section;
}

/*****************************************************************************/
/*****************************************************************************/
/**
	Create platform specific 'config' object that the application
	can read/write settings from/to, as well as actually save or load
	it to/from a file
 */
confhandle_t _confCreate(const char * pathname)
{
	OConfig *		pConfig		= NULL;
	confhandle_t	hConf       = NULL;
    NSURL *         url         = NULL;
    bool            exists      = false;
    
    if ( pathname != nil )
    {
        url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:pathname]];
        
        NSString * filename = [url relativePath];
        exists = [[NSFileManager defaultManager] fileExistsAtPath:filename];
    }
    
	if (    pathname == nil
         || !exists
        )
	{
		// create a blank dictionary
		pConfig = [[OConfig alloc] init];
		assert(pConfig != NULL);
	}
	else 
	{
		// load it from the specified file
		pConfig = [OConfig dictionaryWithContentsOfURL:url];
	}

	assert(pConfig != nil);
	
	hConf = (confhandle_t)CFBridgingRetain(pConfig);
		
	return hConf;
}

/*****************************************************************************/
/**
 */
result_t _confDestroy(confhandle_t hConf)
{
	result_t	errResult	= XERROR_NONE;
	//OConfig *	pConfig		= (OConfig *)CFBridgingRelease(hConf);

	//pConfig = nil;
	
	return errResult;
}

/*****************************************************************************/
/**
 */
result_t _confSave(confhandle_t hConf, const char * pathname)
{
	result_t	errResult	= XERROR_NONE;
	OConfig *	pConfig		= (__bridge OConfig *)hConf;
    NSString *  str         = [NSString stringWithUTF8String:pathname];
    NSURL *     url         = [NSURL fileURLWithPath:str];

	if ( [pConfig writeToURL:url atomically:TRUE] == FALSE )
	{
		errResult = XERROR_GENERIC;
	}
	
	pConfig = nil;
	
	return errResult;
}

/*****************************************************************************/
/**
 */
result_t _confSetString(confhandle_t hConf, const char * section, const char * key, const char * pcszValue)
{
	result_t				errResult	= XERROR_NONE;
	OConfig *				pConfig		= (__bridge OConfig *)hConf;
	NSString *				pStrSection	= [NSString stringWithCString:section encoding:NSASCIIStringEncoding];
	NSString *				pStrKey		= [NSString stringWithCString:key encoding:NSASCIIStringEncoding];
	NSString *				pStrValue	= [NSString stringWithCString:pcszValue encoding:NSASCIIStringEncoding];
	NSMutableDictionary *	dict;
	
	dict = addSection(pConfig,pStrSection);
	assert(dict != nil);
	
	[dict setObject:pStrValue forKey:pStrKey];
	
	pConfig = nil;

	return errResult;
}

/*****************************************************************************/
/**
 */
result_t _confGetString(confhandle_t hConf, const char * section, const char * key, char * pszString, int iStringNumChars)
{
	result_t				errResult	= XERROR_NOT_FOUND;
	OConfig *				pConfig		= (__bridge OConfig *)hConf;
	NSString *				pStrSection	= [NSString stringWithCString:section encoding:NSASCIIStringEncoding];
	NSString *				pStrKey		= [NSString stringWithCString:key encoding:NSASCIIStringEncoding];
	NSString *				pStrValue;
	NSMutableDictionary *	dict;
	
	dict = getSection(pConfig,pStrSection);
	if ( dict != nil )
	{
		pStrValue = [dict valueForKey:pStrKey];
		if ( pStrValue != nil )
		{
			[pStrValue getCString:pszString maxLength:iStringNumChars encoding:NSASCIIStringEncoding];
		
			errResult = XERROR_NONE;
		}
	}
	
	pConfig = nil;

	return errResult;
}

/*****************************************************************************/
/**
 */
result_t _confSetPath(confhandle_t hConf, const char * section, const char * key, const char * pathname)
{
	result_t errResult	= XERROR_INVALID_PARAMETER;
    
    if (    hConf != NULL
         && section != NULL
         && key != NULL
         && pathname != NULL
        )
    {
        errResult    = XERROR_NONE;
        
        OConfig *				pConfig		= (__bridge OConfig *)hConf;
        NSURL *                 url         = [NSURL URLWithString:[NSString stringWithUTF8String:pathname]];
        NSString *				pStrSection	= [NSString stringWithCString:section encoding:NSASCIIStringEncoding];
        NSString *				pStrKey		= [NSString stringWithCString:key encoding:NSASCIIStringEncoding];
        NSString *				pStrValue	= [url path];
        NSMutableDictionary *	section;
        
        if ( url == nil )
        {
            pStrValue = [NSString stringWithUTF8String:pathname];
        }
        
        section = addSection(pConfig,pStrSection);
        assert(section != nil);
        
        if ( pStrValue != nil )
        {
            [section setObject:pStrValue forKey:pStrKey];
        }
    }
    
	return errResult;
}

/*****************************************************************************/
/**
 */
result_t _confGetPath(confhandle_t hConf, const char * section, const char * key, char ** outPathname)
{
	result_t				errResult	= XERROR_NOT_FOUND;
	OConfig *				pConfig		= (__bridge OConfig *)hConf;
	NSString *				pStrSection	= [NSString stringWithCString:section encoding:NSASCIIStringEncoding];
	NSString *				pStrKey		= [NSString stringWithCString:key encoding:NSASCIIStringEncoding];
	NSString *				pStrValue;
	NSMutableDictionary *	dict;
	NSURL *					url		    = nil;
    NSString *              path        = nil;
    
	dict = getSection(pConfig,pStrSection);
	if ( dict != nil )
	{
		pStrValue = [dict valueForKey:pStrKey];
		if (   pStrValue != nil
            && [pStrValue length] > 0
           )
		{
			//url = [NSURL fileURLWithPath:pStrValue];
            url = [[NSURL alloc] initWithString:pStrValue];
		}
	
		if (    outPathname != nil
			 && url != nil
			)
		{
            path = [url path];
        }
        else
        {
            path = pStrValue;
        }
        
        if (   outPathname != nil
            && path != nil
            )
        {
            const char * s = [path cStringUsingEncoding:NSUTF8StringEncoding];
			(*outPathname) = strdup(s);
		
			errResult	= XERROR_NONE;
		}
	}
	
	pConfig = nil;
	url = nil;
	
	return errResult;
}

/*****************************************************************************/
/**
 */
result_t _confSetInt(confhandle_t hConf, const char * section, const char * key, int value)
{
	result_t				errResult	= XERROR_NONE;
	OConfig *				pConfig	    = (__bridge OConfig *)hConf;
	NSString *				pStrSection	= [NSString stringWithCString:section encoding:NSASCIIStringEncoding];
	NSString *				pStrKey		= [NSString stringWithCString:key encoding:NSASCIIStringEncoding];
	NSMutableDictionary *	dict;
	NSString *				pIntValue	= [NSString stringWithFormat:@"%d", value];
	
	dict = addSection(pConfig,pStrSection);
	assert(dict != nil);
	
	[dict setObject:pIntValue forKey:pStrKey];
	
	pConfig = nil;

	return errResult;
}

/*****************************************************************************/
/**
 */
result_t _confGetInt(confhandle_t hConf, const char * section, const char * key, int * outValue)
{
	result_t				errResult	= XERROR_NOT_FOUND;
	OConfig *				pConfig	= (__bridge OConfig *)hConf;
	NSString *				pStrSection	= [NSString stringWithCString:section encoding:NSASCIIStringEncoding];
	NSString *				pStrKey		= [NSString stringWithCString:key encoding:NSASCIIStringEncoding];
	//NSString *		    pStrValue	= [NSString stringWithCString:pcszValue encoding:NSASCIIStringEncoding];
	NSMutableDictionary *	dict;
	NSString *				pValue;
	
	assert(outValue != nil);
	
	dict = getSection(pConfig,pStrSection);
	if ( dict != nil )
	{
		pValue = [dict valueForKey:pStrKey];
		if ( pValue != nil )
		{
			*outValue = [pValue intValue];
		
			errResult	= XERROR_NONE;
		}
	}
	
	pConfig = nil;

	return errResult;
}

/*****************************************************************************/
/*****************************************************************************/

