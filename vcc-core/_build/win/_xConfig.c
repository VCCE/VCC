/*****************************************************************************/
/**
	_xConfig.c
	
	Win32 implementation of VCC config API which is based on the original Win32 API
*/
/*****************************************************************************/

#include "config.h"
#include "file.h"

#if !(defined _WIN32)
#	error "WINDOWS implementation"
#endif

#include "xDebug.h"

#include <Windows.h>
#include <assert.h>

confhandle_t _confCreate(const char * pathname)
{
	assert(0);
	return NULL;
}

result_t _confDestroy(confhandle_t confHandle)
{
	assert(0);
	return XERROR_GENERIC;
}

result_t _confSave(confhandle_t confHandle, const char * pathname)
{
	assert(0);
	return XERROR_GENERIC;
}

result_t _confSetString(confhandle_t confHandle, const char * section, const char * key, const char * pValue)
{
	assert(0);
	return XERROR_GENERIC;
}

result_t _confGetString(confhandle_t confHandle, const char * section, const char * key, char * pszString, size_t stringSize)
{
	assert(0);
	return XERROR_GENERIC;
}

result_t _confSetInt(confhandle_t confHandle, const char * section, const char * key, int value)
{
	assert(0);
	return XERROR_GENERIC;
}

result_t _confGetInt(confhandle_t confHandle, const char * section, const char * key, int * outValue)
{
	assert(0);
	return XERROR_GENERIC;
}

result_t _confSetPath(confhandle_t confHandle, const char * section, const char * key, const char * pathname)
{
	assert(0);
	return XERROR_GENERIC;
}

result_t _confGetPath(confhandle_t confHandle, const char * section, const char * key, char ** outPathname)
{
	assert(0);
	return XERROR_GENERIC;
}

/*****************************************************************************/

