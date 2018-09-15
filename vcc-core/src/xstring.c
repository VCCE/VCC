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

#include "xstring.h"

const char * strrpbrk(const char *s, const char *accept)
{
    const char *p = s;
    
    // go to last char in string
    while (*p)
    {
        p++;
    }
    
    // go from end of string to beginning
    while (p>s)
    {
        // walk the accept string
        const char *a = accept;
        while (*a != '\0')
            if (*a++ == *p)
                return p;
        p--;
    }
    
    return NULL;
}

/**
    Get a pointer to the end of a string (the terminator)
 */
const char * strend(const char * s)
{
    const char * p = s;
    
    while ( p && *p )
    {
        p++;
    }
    
    return p;
}

/**
    Get a pointer to the first occurrance of any character contained in chrs
 */
const char * strchrs(const char * s, const char * chrs)
{
    const char * p = s;
    
    while ( *p++ != 0 )
    {
        if ( strchr(chrs,*p) != NULL )
        {
            return p;
        }
    }
    
    return NULL;
}

/**
 Get a pointer to the last occurrance of any character contained in chrs
 */
const char * strrchrs(const char * s, const char * chrs)
{
    const char * p = strend(s);
    
    while ( --p >= s )
    {
        if ( strchr(chrs,*p) != NULL )
        {
            return p;
        }
    }
    
    return NULL;
}

#if (defined _WIN32)
int strcasecmp(const char * s, const char * p)
{
    return stricmp(s,p);
}
#endif

char * strdupsafe(const char * s)
{
    char * p = NULL;
    
    if ( s != NULL )
    {
        p = strdup(s);
    }
    
    return p;
}

/**
    Reallocate a string
 
    @param s original string
    @param r replacement string
 */
char * strreallocsafe(char * s, const char * r)
{
    char * p = NULL;
    
    // if the replacement string is the same
    if (    s != NULL
         && r != NULL
         && strcmp(s,r) == 0
        )
    {
        // keep original string
        p = s;
    }
    
    if ( p == NULL )
    {
        // free original string
        strfreesafe(s);
        // make a copy of the replacement string
        p = strdupsafe(r);
    }
    
    return p;
}

void strfreesafe(char * s)
{
    if ( s != NULL )
    {
        free(s);
    }
}

