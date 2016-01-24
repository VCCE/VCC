/*****************************************************************************/
/**
	_xDynLib.c - WIN32 specific support for dynamic libraries
*/
/*****************************************************************************/

#include "xDynLib.h"

// vcc-core private headers
#include "_xDynLib.h"

// vcc-core
#include "xDebug.h"

//
//
//
#include <assert.h>

//
// system specific
//
#include <Windows.h>

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
VCCCORE_API result_t VCCCORE_CALL xDynLibLoad(ppathname_t pPathname, pxdynlib_t * ppxdynlib)
{
	result_t		errResult = XERROR_INVALID_PARAMETER;
	
	if (    pPathname != NULL
		 && ppxdynlib != NULL 
	   )
	{
		/*
			allocate dynamic library structure
		*/
		errResult = XERROR_OUT_OF_MEMORY;
		*ppxdynlib = malloc(sizeof(struct xdynlib_t));
		if ( *ppxdynlib != NULL )
		{
			errResult = XERROR_NONE;

			/*
				load the module without resolving any symbols
			*/
			(*ppxdynlib)->hModule = LoadLibrary(pPathname);
			
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
VCCCORE_API result_t VCCCORE_CALL xDynLibUnload(pxdynlib_t * ppxdynlib)
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
			iResult = FreeLibrary( (*ppxdynlib)->hModule );
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
VCCCORE_API result_t VCCCORE_CALL xDynLibGetSymbolAddress(pxdynlib_t pxdynlib, const char_t * pcszSymbol, void ** ppvSymbol)
{
	result_t		errResult = XERROR_INVALID_PARAMETER;

	if (    pxdynlib != NULL
		 && pcszSymbol != NULL
		 && ppvSymbol != NULL
	   )
	{
		errResult = XERROR_NOT_FOUND;
		
		*ppvSymbol = GetProcAddress(pxdynlib->hModule, pcszSymbol);
		if ( *ppvSymbol != NULL )
		{
			errResult = XERROR_NONE;
		}
	}
	
	return errResult;
}

/*****************************************************************************/
