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

#import "OSystem.h"

@implementation OSystem

@end

/*************************************************************************************/

void sysInit(void)
{
    
}

void sysSleep(uint32_t timeInMs)
{
    sleep(timeInMs);
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

char * sysGetPathResources(char * filename, char * pDst, size_t sizeDst)
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

/*************************************************************************************/

