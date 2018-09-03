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

#include "path.h"
#include "xstring.h"

#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#if !(defined _WIN32)
#	include <unistd.h>
#else
// gotta love microsoft

#define strtok_r strtok_s

// Copied from linux libc sys/stat.h:
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

char * abs2rel(const char * path, const char * base, char * result, const size_t size);
char * rel2abs(const char * path, const char * base, char * result, const size_t size);
void pathCanonicalize(char *path);

/**
 */
bool pathIsAbsolute(const char * path)
{
    if ( path != NULL )
    {
        if ( *path == '/' )
        {
            return true;
        }
    }
    
    return false;
}

/**
 */
bool pathIsDirectory(const char * path)
{
    struct stat statbuf;
    
    if (stat(path, &statbuf) != 0)
    {
        return false;
    }
    
    return S_ISDIR(statbuf.st_mode);
}

/**
 */
bool pathIsFile(const char * path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    
    return S_ISREG(path_stat.st_mode);
}

/**
 */
result_t pathGetFilename(const char * pPathname, char * pDst, size_t szDst)
{
    result_t result = XERROR_INVALID_PARAMETER;
    
    if (    pPathname != NULL
        && pDst != NULL
        && szDst > 0
        )
    {
        result = XERROR_NOT_FOUND;
        
        const char * s = strrpbrk(pPathname,"\\/:");
        if (s != NULL)
        {
            result = XERROR_BUFFER_OVERFLOW;
            
            size_t length = strlen(s+1)+1;
            if ( length < szDst )
            {
                strncpy(pDst,s+1,length);
                result = XERROR_NONE;
            }
        }
        else
        {
            size_t length = min(strlen(pPathname)+1,szDst-1);
            strncpy(pDst,pPathname,length);
        }
    }
    
    return result;
}

/**
 */
result_t pathGetPathname(const char * pPathname, char * pDst, size_t szDst)
{
    result_t result = XERROR_INVALID_PARAMETER;
    
    if ( pPathname != NULL && pDst != NULL && szDst > 0 )
    {
        result = XERROR_NOT_FOUND;
        const char * s = strrpbrk(pPathname,"\\/:");
        
        if (s != NULL)
        {
            result = XERROR_BUFFER_OVERFLOW;
            
            size_t length = s-pPathname;
            if ( length < szDst )
            {
                strcpy(pDst,pPathname);
                pDst[length] = 0;
                
                result = XERROR_NONE;
            }
        }
        else
        {
            size_t length = min(strlen(pPathname),szDst-1);
            strncpy(pDst,pPathname,length);
        }
    }
    
    return result;
}

/**
    Get relative path from a reference path and a base path
 
 Note:
    - each path can point to directory or a file
    - directories or files must exist or it will be treated as if it were a directory
 
    4 conditions
        base is a directoty, path is a directoty
        base is a file, path is a directoty
        base is a directory, path is a file
        base is a file, path is a file
 */
// TODO: Error checking/return
result_t pathGetRelativePath(const char * base, const char * path, char ** outPath)
{
    assert(base != NULL);
    assert(path != NULL);
    assert(outPath != NULL);
    assert(*outPath == NULL);
    
    // base path
    char * basePath = calloc(1,PATH_MAX);
    if ( pathIsFile(base) )
    {
        pathGetPathname(base, basePath, PATH_MAX);
    }
    else
    {
        strcpy(basePath,base);
    }
    
    // target path
    char * filename = calloc(1,PATH_MAX);
    char * targetPath = calloc(1,PATH_MAX);
    if ( pathIsFile(path) )
    {
        pathGetFilename(path,filename,PATH_MAX);
        pathGetPathname(path, targetPath, PATH_MAX);
    }
    else
    {
        strcpy(targetPath,path);
    }

    char * relPath = calloc(1,PATH_MAX);
    
    abs2rel(targetPath,basePath,relPath,PATH_MAX);
    
    char * outRelPath = NULL;
    pathCombine(relPath,filename,&outRelPath);
    
    //char * p = realpath(outRelPath,*outPath);
    *outPath = outRelPath;
    
    free(relPath);
    free(basePath);
    free(targetPath);
    free(filename);

    return XERROR_NONE;
}

/**
 */
result_t pathGetAbsolutePath(const char * base, const char * path, char ** outPath)
{
    assert(false && "TODO: implement");
    
    return XERROR_GENERIC;
}

/**     
 */
result_t pathCombine(const char * base, const char * subPath, char ** outPath)
{
    result_t result = XERROR_INVALID_PARAMETER;
    
    char * tmpPath = calloc(1,PATH_MAX);
    
    strcpy(tmpPath,base);
    const char * e = strend(tmpPath);
    if ( e > tmpPath )
    {
        if ( *(e-1) != '/' )
        {
            strcat(tmpPath,"/");
        }
    }
    
    strcat(tmpPath,subPath);
    
    //*outPath = calloc(1,PATH_MAX);
    //char * s = realpath(tmpPath,*outPath);
    
    pathCanonicalize(tmpPath);

    *outPath = strdup(tmpPath);
    
    result = XERROR_NONE;
    
    free(tmpPath);
    
    return result;
}

// https://stackoverflow.com/questions/28659344/alternative-to-realpath-to-resolve-and-in-a-path

// bugs:
//  "../path/filename.txt" -> "/path/filename.txt"

void pathCanonicalize(char *path)
{
    size_t i;
    size_t j;
    size_t k;
    
    //Move to the beginning of the string
    i = 0;
    k = 0;
    
    //Replace backslashes with forward slashes
    while (path[i] != '\0') {
        //Forward slash or backslash separator found?
        if (path[i] == '/' || path[i] == '\\') {
            path[k++] = '/';
            while (path[i] == '/' || path[i] == '\\')
                i++;
        } else {
            path[k++] = path[i++];
        }
    }
    
    //Properly terminate the string with a NULL character
    path[k] = '\0';
    
    //Move back to the beginning of the string
    i = 0;
    j = 0;
    k = 0;
    
    //Parse the entire string
    do {
        //Forward slash separator found?
        if (path[i] == '/' || path[i] == '\0') {
            //"." element found?
            if ((i - j) == 1 && !strncmp (path + j, ".", 1)) {
                //Check whether the pathname is empty?
                if (k == 0) {
                    if (path[i] == '\0') {
                        path[k++] = '.';
                    } else if (path[i] == '/' && path[i + 1] == '\0') {
                        path[k++] = '.';
                        path[k++] = '/';
                    }
                } else if (k > 1) {
                    //Remove the final slash if necessary
                    if (path[i] == '\0')
                        k--;
                }
            }
            //".." element found?
            else if ((i - j) == 2 && !strncmp (path + j, "..", 2)) {
                //Check whether the pathname is empty?
                if (k == 0) {
                    path[k++] = '.';
                    path[k++] = '.';
                    
                    //Append a slash if necessary
                    if (path[i] == '/')
                        path[k++] = '/';
                } else if (k > 1) {
                    //Search the path for the previous slash
                    for (j = 1; j < k; j++) {
                        if (path[k - j - 1] == '/')
                            break;
                    }
                    
                    //Slash separator found?
                    if (j < k) {
                        if (!strncmp (path + k - j, "..", 2)) {
                            path[k++] = '.';
                            path[k++] = '.';
                        } else {
                            k = k - j - 1;
                        }
                        
                        //Append a slash if necessary
                        if (k == 0 && path[0] == '/')
                            path[k++] = '/';
                        else if (path[i] == '/')
                            path[k++] = '/';
                    }
                    //No slash separator found?
                    else {
                        if (k == 3 && !strncmp (path, "..", 2)) {
                            path[k++] = '.';
                            path[k++] = '.';
                            
                            //Append a slash if necessary
                            if (path[i] == '/')
                                path[k++] = '/';
                        } else if (path[i] == '\0') {
                            k = 0;
                            path[k++] = '.';
                        } else if (path[i] == '/' && path[i + 1] == '\0') {
                            k = 0;
                            path[k++] = '.';
                            path[k++] = '/';
                        } else {
                            k = 0;
                        }
                    }
                }
            } else {
                //Copy directory name
                memmove (path + k, path + j, i - j);
                //Advance write pointer
                k += i - j;
                
                //Append a slash if necessary
                if (path[i] == '/')
                    path[k++] = '/';
            }
            
            //Move to the next token
            while (path[i] == '/')
                i++;
            j = i;
        }
        else if (k == 0) {
            //while (path[i] == '.' || path[i] == '/')
            // PWG: changing this fixed a bug
            // ./filename.txt = ilename.txt (instead of filename.txt)
            // ../filename.txt = filename.txt (instead of ../filename.txt
            while (path[i] == '/')
            {
                j++;
                i++;
            }
        }
    } while (path[i++] != '\0');
    
    //Properly terminate the string with a NULL character
    path[k] = '\0';
}

int pathcanon(const char *srcpath, char *dstpath, size_t sz)
{
    size_t plen = strlen(srcpath) + 1, chk;
	char * wtmp;
	char ** tokv;
	char *s;
	char *tok;
	char *sav;
    int i, ti, relpath;
    
	wtmp = calloc(1, plen * sizeof(char));
	tokv = calloc(1, plen * sizeof(char *));

    relpath = (*srcpath == '/') ? 0 : 1;
    
    /* make a local copy of srcpath so strtok(3) won't mangle it */
    
    ti = 0;
    (void) strcpy(wtmp, srcpath);
    
    tok = strtok_r(wtmp, "/", &sav);
    while (tok != NULL) {
        if (strcmp(tok, "..") == 0) {
            if (ti > 0) {
                ti--;
            }
        } else if (strcmp(tok, ".") != 0) {
            tokv[ti++] = tok;
        }
        tok = strtok_r(NULL, "/", &sav);
    }
    
    chk = 0;
    s = dstpath;
    
    /*
     * Construct canonicalized result, checking for room as we
     * go. Running out of space leaves dstpath unusable: written
     * to and *not* cleanly NUL-terminated.
     */
    for (i = 0; i < ti; i++) {
        size_t l = strlen(tokv[i]);
        
        if (i > 0 || !relpath) {
            if (++chk >= sz) return -1;
            *s++ = '/';
        }
        
        chk += l;
        if (chk >= sz) return -1;
        
        strcpy(s, tokv[i]);
        s += l;
    }
    
    if (s == dstpath) {
        if (++chk >= sz) return -1;
        *s++ = relpath ? '.' : '/';
    }
    *s = '\0';
    
	free(wtmp);
	free(tokv);

    return 0;
}

/*************************************************************************************/
/*
 * Copyright (c) 1997 Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999 Tama Communications Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * abs2rel: convert an absolute path name into relative.
 *
 *    i)    path    absolute path
 *    i)    base    base directory (must be absolute path)
 *    o)    result    result buffer
 *    i)    size    size of result buffer
 *    r)        != NULL: relative path
 *            == NULL: error
 */
char * abs2rel(const char * path, const char * base, char * result, const size_t size)
{
    const char *pp, *bp, *branch;
    
    /*
     * endp points the last position which is safe in the result buffer.
     */
    const char *endp = result + size - 1;
    char *rp;
    
    if (*path != '/') {
        if (strlen(path) >= size)
            goto erange;
        strcpy(result, path);
        goto finish;
    } else if (*base != '/' || !size) {
        errno = EINVAL;
        return (NULL);
    } else if (size == 1)
        goto erange;
    /*
     * seek to branched point.
     */
    branch = path;
    for (pp = path, bp = base; *pp && *bp && *pp == *bp; pp++, bp++)
        if (*pp == '/')
            branch = pp;
    if ((*pp == 0 || (*pp == '/' && *(pp + 1) == 0)) &&
        (*bp == 0 || (*bp == '/' && *(bp + 1) == 0))) {
        rp = result;
        *rp++ = '.';
        if (*pp == '/' || *(pp - 1) == '/')
            *rp++ = '/';
        if (rp > endp)
            goto erange;
        *rp = 0;
        goto finish;
    }
    if ((*pp == 0 && *bp == '/') || (*pp == '/' && *bp == 0))
        branch = pp;
    /*
     * up to root.
     */
    rp = result;
    for (bp = base + (branch - path); *bp; bp++)
        if (*bp == '/' && *(bp + 1) != 0) {
            if (rp + 3 > endp)
                goto erange;
            *rp++ = '.';
            *rp++ = '.';
            *rp++ = '/';
        }
    if (rp > endp)
        goto erange;
    *rp = 0;
    /*
     * down to leaf.
     */
    if (*branch) {
        if (rp + strlen(branch + 1) > endp)
            goto erange;
        strcpy(rp, branch + 1);
    } else
        *--rp = 0;
finish:
    return result;
erange:
    errno = ERANGE;
    return (NULL);
}

/*
 * Copyright (c) 1997 Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999 Tama Communications Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * rel2abs: convert an relative path name into absolute.
 *
 *    i)    path    relative path
 *    i)    base    base directory (must be absolute path)
 *    o)    result    result buffer
 *    i)    size    size of result buffer
 *    r)        != NULL: absolute path
 *            == NULL: error
 */
char * rel2abs(const char * path, const char * base, char * result, const size_t size)
{
    const char *pp, *bp;
    /*
     * endp points the last position which is safe in the result buffer.
     */
    const char *endp = result + size - 1;
    char *rp;
    size_t length;
    
    if (*path == '/') {
        if (strlen(path) >= size)
            goto erange;
        strcpy(result, path);
        goto finish;
    } else if (*base != '/' || !size) {
        errno = EINVAL;
        return (NULL);
    } else if (size == 1)
        goto erange;
    
    length = strlen(base);
    
    if (!strcmp(path, ".") || !strcmp(path, "./")) {
        if (length >= size)
            goto erange;
        strcpy(result, base);
        /*
         * rp points the last char.
         */
        rp = result + length - 1;
        /*
         * remove the last '/'.
         */
        if (*rp == '/') {
            if (length > 1)
                *rp = 0;
        } else
            rp++;
        /* rp point NULL char */
        if (*++path == '/') {
            /*
             * Append '/' to the tail of path name.
             */
            *rp++ = '/';
            if (rp > endp)
                goto erange;
            *rp = 0;
        }
        goto finish;
    }
    bp = base + length;
    if (*(bp - 1) == '/')
        --bp;
    /*
     * up to root.
     */
    for (pp = path; *pp && *pp == '.'; ) {
        if (!strncmp(pp, "../", 3)) {
            pp += 3;
            while (bp > base && *--bp != '/')
                ;
        } else if (!strncmp(pp, "./", 2)) {
            pp += 2;
        } else if (!strncmp(pp, "..\0", 3)) {
            pp += 2;
            while (bp > base && *--bp != '/')
                ;
        } else
            break;
    }
    /*
     * down to leaf.
     */
    length = bp - base;
    if (length >= size)
        goto erange;
    strncpy(result, base, length);
    rp = result + length;
    if (*pp || *(pp - 1) == '/' || length == 0)
        *rp++ = '/';
    if (rp + strlen(pp) > endp)
        goto erange;
    strcpy(rp, pp);
finish:
    return result;
erange:
    errno = ERANGE;
    return (NULL);
}

