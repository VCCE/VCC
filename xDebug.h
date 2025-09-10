#ifndef _xDebug_h_
#define _xDebug_h_

/*****************************************************************************/

/*
	Debug trace macros
*/
#if (defined _DEBUG)
#	define XTRACE(x,...)			_xDbgTrace( __FILE__, __LINE__, x, ##__VA_ARGS__ )
#else
#	define XTRACE(x,...)
#endif

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	void		_xDbgTrace(const char* pFile, const int iLine, const char* pFormat, ...);
	
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif
