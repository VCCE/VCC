/*************************************************************************************/
/*
*/
/*************************************************************************************/
/*
 */
/*************************************************************************************/

#include "file.h"
#include "pakInterface.h"
#include "xAssert.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************/

result_t sysGetPathString(const ppathname_t pPathname, char * pFilename, size_t szBufferSize)
{
	result_t	errResult	= XERROR_NONE;
	
	if (    pPathname != NULL 
		 && pFilename != NULL
		 && szBufferSize > 0
		)
	{
		assert(0);
	}
	
	return errResult;
}

/*************************************************************************************/
/**
	Using platform specific UI, ask the user for a file (platform specific pathname 
	object).  
 
	Used for loading external ROM, ROM Pak, device Pak, etc
 */
ppathname_t sysGetPathnameFromUser(void * pFileTypes, ppathname_t pStartPath)
{
	ppathname_t		pPathname	= NULL;

	assert(0);

	return pPathname;
}

/*************************************************************************************/
/**
	Get a copy of a platform specific pathname
 */
ppathname_t	sysGetPathCopy(ppathname_t pPathname)
{
	ppathname_t		pPathCopy	= NULL;
	
	assert(0);

	return pPathCopy;
}

/*************************************************************************************/
/**
	Destroy a platform specific pathname object
 */
void sysPathDestroy(ppathname_t pPathname)
{
	free(pPathname);
}

/*************************************************************************************/

result_t sysPathGetFilename(ppathname_t pPathname, char * pDst, size_t szDst)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	
	if ( pPathname != NULL )
	{
		errResult = XERROR_GENERIC;

		assert(0);
	}
	
	return errResult;
}

/*************************************************************************************/
/**
 Get the 'file id'
 
 COCO_PAK_ROM			= bin or rom
 COCO_PAK_PLUGIN		= plug-in pak
 COCO_PAK_ADDONMOD		= add-on mod
 COCO_PERIPHERAL		= peripheral
 COCO_CASSETTE_CAS		= cassette .cas format file
 COCO_CASSETTE_WAVE		= cassette in Wave sound file format
 COCO_VDISK_FLOPPY_XXX
 COCO_VDISK_VHD_XXX
 COCO_VDISK_ISO
*/

typedef struct pair_t pair_t;
struct pair_t
{
	size_t			offset;
	unsigned char	value;
};

typedef struct filetype_t filetype_t;
struct filetype_t
{
	int_t			iType;
	const char *	pExt;
	pair_t			sig[4];
	
} ;

filetype_t g_filetypelist[] = 
{
	{ COCO_PAK_ROM,			"ROM",			
		{	{	0,	0 },	
			{	0,	0 },
			{	0,	0 },
			{	0,	0 }
		},	
	},
	{ COCO_PAK_ROM,			"BIN",			
		{	{	0,	0 },		
			{	0,	0 },
			{	0,	0 },
			{	0,	0 }
		},	
	},
	{ COCO_PAK_PLUGIN,		"dylib",	// also could be add-on mod		
		{	{	0,	0 },		
			{	0,	0 },
			{	0,	0 },
			{	0,	0 }
		},	
	},
	{ COCO_CASSETTE_CAS,	"cas",			
		{	{	0,	0 },		
			{	0,	0 },
			{	0,	0 },
			{	0,	0 }
		},	
	},
	{ COCO_CASSETTE_CAS,	"cassette",			
		{	{	0,	0 },		
			{	0,	0 },
			{	0,	0 },
			{	0,	0 }
		},	
	},
	{ COCO_CASSETTE_WAVE,	"wav",			
		{	{	0,	0 },		
			{	0,	0 },
			{	0,	0 },
			{	0,	0 },
		},	
	},
	{ COCO_CASSETTE_WAVE,	"wave",			
		{	{	0,	0 },		
			{	0,	0 },
			{	0,	0 },
			{	0,	0 }
		},	
	},
		
	{ COCO_FILE_NONE,		NULL,		{	{	0,	0 }	}	}
} ;

int_t sysGetFileType(ppathname_t pPathname)
{
	int_t			iType	= COCO_FILE_NONE;

	assert(0);

#if 0
	NSURL *			pURL;
	NSFileHandle *	pFile;
	NSError *		pError;
	NSData *		pData;
	filetype_t *	pCurType;
	pair_t *		pCurPair;
	unsigned char	uc;
	NSString *		pExt;
	NSString *		pCheckExt;
	NSString *		pTemp;
	
	pURL = (NSURL *)pPathname;
	assert(pURL != NULL);
	if ( pURL != NULL )
	{
		pFile = [NSFileHandle fileHandleForReadingFromURL:pURL error:&pError];
		if ( pFile != NULL )
		{
			pTemp = [pURL lastPathComponent];
			pExt = [pTemp pathExtension];
			
			pCurType = &g_filetypelist[0];
			while (    iType == COCO_FILE_NONE
					&& pCurType ->iType != 0 
				  )
			{
				if (    pExt != NULL
					 && pCurType->pExt != NULL
					)
				{
					// compare extension
					pCheckExt = [NSString stringWithCString:pCurType->pExt encoding:NSASCIIStringEncoding];
					if ( [pExt compare:pCheckExt options:NSCaseInsensitiveSearch] == NSOrderedSame )
					{
						iType = pCurType->iType;
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
						[pFile seekToFileOffset:pCurPair->offset];
					
						pData = [pFile readDataOfLength:1];
						if ( pData != NULL )
						{
							[pData getBytes:&uc length:1];
						
							if ( uc == pCurPair->value )
							{
								iType = pCurType->iType;
								break;
							}
							
							[pData release];
						}
						
						// next
						pCurPair++;
					}
				}

				// next
				pCurType++;
			}
		
			[pFile closeFile];
		}
	}
#endif

	return iType;
}

/*************************************************************************************/
/**
	Load a file's contents into memory
 */
result_t fileLoad(ppathname_t pPathname, void ** ppBuffer, size_t * pszBufferSize)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;

	assert(0);

#if 0
	NSData *	pData;
	NSURL *		pURL		= (NSURL *)pPathname;
	size_t		szBytes;
	
	if (    pURL != NULL 
		&& ppBuffer != NULL
		)
	{
		errResult = XERROR_NOT_FOUND;
		
		// load the file
		pData	= [NSData dataWithContentsOfURL:pURL];
		szBytes	= [pData length];
		
		if (    pData != NULL
			&& szBytes > 0
			)
		{
			errResult = XERROR_OUT_OF_MEMORY;
			
			// allocate buffer for the caller
			*ppBuffer = malloc(szBytes);
			if ( *ppBuffer != NULL )
			{
				// copy the bytes
				[pData getBytes:*ppBuffer];
				
				*pszBufferSize = szBytes;
				
				errResult = XERROR_NONE;
			}
		}
	}
#endif

	return errResult;
}

/*************************************************************************************/
/**
 */
pfile_t fileOpen(ppathname_t pPathname, int_t iMode)
{
	assert(0);

	return (pfile_t)NULL;
}

/*************************************************************************************/
/**
 */
result_t fileRead(pfile_t pFile, void * pBuffer, size_t szReadBytes, size_t * pszBytesRead)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;

	assert(0);

#if 0
	NSFileHandle *	pFileHandle	= (NSFileHandle *)pFile;
	NSData *		pData;

	@try 
	{
		pData = [pFileHandle readDataOfLength:szReadBytes];
	}
	@catch (NSException * e) 
	{
		assert(0);
	}
	@finally 
	{
		//assert(0);
	}
	if ( pData != NULL )
	{
		//assert([pFileHandle retainCount] == 2);
		
		// default to read error
		errResult = XERROR_READ;
		
		// get the data for the caller
		[pData getBytes:pBuffer length:szReadBytes];
		
		// was entire read successful?
		if ( [pData length] == szReadBytes )
		{
			errResult = XERROR_NONE;
		}

		// update caller with amount of data read in
		if ( pszBytesRead != NULL )
		{
			*pszBytesRead = [pData length];
		}
	}
#endif

	return errResult;
}

/*************************************************************************************/
/**
 */
result_t fileWrite(pfile_t pFile, void * pBuffer, size_t szWriteBytes, size_t * pszBytesWritten)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;

	assert(0);

#if 0
	NSFileHandle *	pFileHandle	= (NSFileHandle *)pFile;
	NSData *		pData;
	
	if ( pFileHandle != NULL )
	{
		//assert([pFileHandle retainCount] == 2);
		
		pData = [NSData dataWithBytes:pBuffer length:szWriteBytes];
		assert(pData != NULL);

		@try 
		{
			[pFileHandle writeData:pData];
		}
		@catch (NSException * e) 
		{
			assert(0);
		}
		@finally 
		{
			//assert(0);
		}
		
		errResult = XERROR_NONE;
	}
#endif

	return errResult;
}

/*************************************************************************************/
/**
 */
result_t fileSeek(pfile_t pFile, int_t iOffset, int_t iRef)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;

	assert(0);

#if 0
	NSFileHandle *	pFileHandle	= (NSFileHandle *)pFile;
	
	if ( pFileHandle != NULL )
	{
		//assert([pFileHandle retainCount] == 2);
		
		@try 
		{
			switch ( iRef )
			{
				case FILE_SEEK_BEGIN:
					//[pFileHandle seekToFileOffset:/* (unsigned long long) */ iOffset];
					lseek([pFileHandle fileDescriptor],iOffset,SEEK_SET);
					
					errResult = XERROR_NONE;
					break;
					
				case FILE_SEEK_CURRENT:
				{
					lseek([pFileHandle fileDescriptor],iOffset,SEEK_CUR);
					
					errResult = XERROR_NONE;
				}
					break;
					
				case FILE_SEEK_END:
				{
					lseek([pFileHandle fileDescriptor],0,SEEK_END);
				}
					break;
					
				default:
					assert(0 && "invalid file reference");
					break;
			}		
		}
		@catch (NSException * e) 
		{
			assert(0);
		}
		@finally 
		{
			//assert(0);
		}
	}
#endif

	return errResult;
}

/*************************************************************************************/
/**
 */
result_t filePos(pfile_t pFile, size_t * pszPos)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;

	assert(0);

#if 0
	NSFileHandle *	pFileHandle	= (NSFileHandle *)pFile;
	
	if (    pFileHandle != NULL
		 && pszPos != NULL
		)
	{
		//assert([pFileHandle retainCount] == 2);

		@try 
		{
			*pszPos = [pFileHandle /* (unsigned long long) */ offsetInFile];
		}
		@catch (NSException * e) 
		{
			assert(0);
		}
		@finally 
		{
			//assert(0);
		}
		
		errResult = XERROR_NONE;
	}
#endif

	return errResult;
}

/*************************************************************************************/
/**
 */
result_t fileClose(pfile_t pFile)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;

	assert(0);

#if 0
	NSFileHandle *	pFileHandle	= (NSFileHandle *)pFile;
	
	if ( pFileHandle != NULL )
	{
		//assert([pFileHandle retainCount] == 2);
		//[pFileHandle synchronizeFile];
		
		@try 
		{
			[pFileHandle closeFile];
		}
		@catch (NSException * e) 
		{
			assert(0);
		}
		@finally 
		{
			//assert(0);
		}
		
		[pFileHandle release];
		
		errResult = XERROR_NONE;
	}
#endif

	return errResult;
}

#if 0
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
 *	i)	path	absolute path
 *	i)	base	base directory (must be absolute path)
 *	o)	result	result buffer
 *	i)	size	size of result buffer
 *	r)		!= NULL: relative path
 *			== NULL: error
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
 *	i)	path	relative path
 *	i)	base	base directory (must be absolute path)
 *	o)	result	result buffer
 *	i)	size	size of result buffer
 *	r)		!= NULL: absolute path
 *			== NULL: error
 */
char * rel2abs(const char * path, const char * base, char * result, const size_t size)
{
	const char *pp, *bp;
	/*
	 * endp points the last position which is safe in the result buffer.
	 */
	const char *endp = result + size - 1;
	char *rp;
	int length;
	
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

/*************************************************************************************/

#endif
