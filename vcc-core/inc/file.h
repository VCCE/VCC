#ifndef _file_h_
#define _file_h_

/*****************************************************************************/

#include "xTypes.h"

/*****************************************************************************/

#ifndef _DEBUG
#	define sysPathTrace(x)
#endif

#define MAX_PATH_LENGTH		256

/*****************************************************************************/

typedef struct filehandle_t * pfile_t;

#define FILE_SEEK_BEGIN		0
#define FILE_SEEK_CURRENT	1
#define FILE_SEEK_END		2

#define FILE_READ			1
#define FILE_WRITE			2
#define FILE_READWRITE		3

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
		
	ppathname_t sysGetPathnameFromUser(void * pFileTypes, ppathname_t pStartPath);
	result_t	sysGetPathString(const ppathname_t pPathname, char * pFilename, size_t szBufferSize);
	ppathname_t	sysGetPathCopy(ppathname_t pPathname);
	void		sysPathDestroy(ppathname_t pPathname);
	int_t		sysGetFileType(ppathname_t pPathname);
#ifndef sysPathTrace
	void		sysPathTrace(ppathname_t pPathname);
#endif
	result_t	sysPathGetFilename(ppathname_t pPathname, char * pDst, size_t szDst);
	
	result_t	fileLoad(ppathname_t pPathname, void ** ppBuffer, size_t * pszBufferSize);
	pfile_t		fileOpen(ppathname_t pPathname, int_t iMode);
	result_t	fileRead(pfile_t pFile, void * pBuffer, size_t szReadBytes, size_t * pszBytesRead);
	result_t	fileWrite(pfile_t pFile, void * pBuffer, size_t szWriteBytes, size_t * pszBytesWritten);
	result_t	fileSeek(pfile_t pFile, int_t iOffset, int_t iRef);
	result_t	filePos(pfile_t pFile, size_t * pszPos);
	result_t	fileClose(pfile_t pFile);
	
	char *		abs2rel(const char * path, const char * base, char * result, const size_t size);
	char *		rel2abs(const char * path, const char * base, char * result, const size_t size);

#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif // _file_h_

