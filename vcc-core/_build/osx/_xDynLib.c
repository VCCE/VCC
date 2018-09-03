/*****************************************************************************/

#include "xDynLib.h"

#include "file.h"

#include "_xDynLib.h"

#include "xDebug.h"

/*
	system specific
 */
#include <dlfcn.h>

/*****************************************************************************/
/**
	Dynamic library load
 
	Caller provides a pointer to receive the handle (pointer) to the library.
 
	@param pcszPathname Pathname to the dynamic library
	@param ppxdynlib Pointer to a dynamic library handle pointer to receive
						the newly allocated and loaded library handle
	
	@return XERROR_NONE is returned on success
			XERROR_INVALID_PARAMETER is returned if either parameter is incorrect
			XERROR_OUT_OF_MEMORY on allocation failure
			XERROR_NOT_FOUND is returned if the module could not be loaded
 
	@sa xDynLibUnload xDynLibGetSymbolAddress
			
*/
XAPI result_t XCALL xDynLibLoad(const char * pPathname, pxdynlib_t * ppxdynlib)
{
	result_t		errResult = XERROR_INVALID_PARAMETER;
	char			Filename[256];
    char            Extension[64];
    const char *    dyLibExtension  = ".dylib";
    
	if (    pPathname != NULL
		 && ppxdynlib != NULL 
	   )
	{
		//pathGetPathname(pPathname,Filename,sizeof(Filename));
        strcpy(Filename,pPathname);
        
        // make sure extension is correct
        fileGetExtension(Filename, Extension, sizeof(Extension));
        if (strcasecmp(Extension,dyLibExtension)!=0)
        {
            char * s = strrchr(Filename,'.');
            if ( s != NULL )
            {
                *s = 0;
            }
            strcat(Filename, dyLibExtension);
        }
        
		/*
			allocate dynamic library structure
		*/
		errResult = XERROR_OUT_OF_MEMORY;
		*ppxdynlib = calloc(1,sizeof(struct xdynlib_t));
		if ( *ppxdynlib != NULL )
		{
			errResult = XERROR_NONE;

			/*
				load the module without resolving any symbols
			*/
            // loaded already?
            (*ppxdynlib)->hModule = dlopen(Filename, RTLD_NOLOAD);
            if ( (*ppxdynlib)->hModule == NULL )
            {
                (*ppxdynlib)->hModule = dlopen(Filename, RTLD_NOW);
            }
			
			/*
				Check for load error
			*/
			if ( (*ppxdynlib)->hModule == NULL )
			{
				errResult = XERROR_NOT_FOUND;
				
				/*
					release memory allocated above and clear user's pointer
				*/
				free(*ppxdynlib);
				*ppxdynlib = NULL;
			}
		}
	}
	
	return errResult;
}

/*****************************************************************************/
/**
	Dynamic library unload
 
	@param ppxdynlib Pointer to the library handle/pointer

	@return XERROR_NONE on success
			XERROR_INVALID_PARAMETER if ppxdynlib appears to be incorrect
			XERROR_GENERIC is returned if the internal module handle is NULL
							or the module could not be unloaded
*/
XAPI result_t XCALL xDynLibUnload(pxdynlib_t * ppxdynlib)
{
	result_t		errResult = XERROR_INVALID_PARAMETER;
	int				iResult;
	
	if (    ppxdynlib != NULL 
		 && *ppxdynlib != NULL
	   )
	{
		errResult = XERROR_GENERIC;
		
		if ( (*ppxdynlib)->hModule != NULL )
		{
			iResult = dlclose( (*ppxdynlib)->hModule );
			assert( iResult == 0 );
			if ( iResult == 0 )
			{
				errResult = XERROR_NONE;
			}
		}
		
		/*
			release module structure
		*/
		free(*ppxdynlib);
		*ppxdynlib = NULL;
	}
	
	return errResult;
}

/*****************************************************************************/
/**
	Dynamic library get symbol address
*/
XAPI result_t XCALL xDynLibGetSymbolAddress(pxdynlib_t pxdynlib, const char * pcszSymbol, void ** ppvSymbol)
{
	result_t		errResult = XERROR_INVALID_PARAMETER;

	if (    pxdynlib != NULL
		 && pcszSymbol != NULL
		 && ppvSymbol != NULL
	   )
	{
		errResult = XERROR_NOT_FOUND;
		
		*ppvSymbol = dlsym(pxdynlib->hModule, pcszSymbol);
		if ( *ppvSymbol != NULL )
		{
			errResult = XERROR_NONE;
		}
	}
	
	return errResult;
}

/*****************************************************************************/
