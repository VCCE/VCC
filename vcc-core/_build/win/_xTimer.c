/*****************************************************************************/
/**
	xTimer.c
*/
/*****************************************************************************/

#include "xTimer.h"

#if !(defined _WIN32)
#	error "WINDOWS implementation"
#endif

#include <Windows.h>
#include <assert.h>

/*****************************************************************************/
/**
	https://docs.microsoft.com/en-us/windows/desktop/sysinfo/acquiring-high-resolution-time-stamps
	http://blog.quasardb.net/a-portable-high-resolution-timestamp-in-c/
*/
/*****************************************************************************/

static uint64_t timestamp(void)
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	// there is an imprecision with the initial value, 
	// but what matters is that timestamps are monotonic and consistent 
	return (uint64_t)(li.QuadPart);
}

static uint64_t updatefrequency(void)
{
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li) || !li.QuadPart)
	{
		// log something 
		// GetLastError()
	}
	return (uint64_t)(li.QuadPart);
}

uint64_t filetimeoffset(void)
{
	FILETIME ft;
	uint64_t tmpres = 0;
	// 100-nanosecond intervals since January 1, 1601 (UTC) 
	// which means 0.1 us 
	GetSystemTimeAsFileTime(&ft);
	tmpres |= ft.dwHighDateTime;
	tmpres <<= 32;
	tmpres |= ft.dwLowDateTime;
	// January 1st, 1970 - January 1st, 1601 UTC ~ 369 years 
	// or 116444736000000000 us 
	static const uint64_t deltaepoch = 116444736000000000;
	tmpres -= deltaepoch;
	return tmpres;
}
// offset in microseconds 
static uint64_t zerotime(void)
{
	return filetimeoffset();
}

static uint64_t _zerotime = 0;
static uint64_t _offset = 0;
static uint64_t _frequency = 0;

static uint64_t now(void)
{
	if (_offset == 0)
	{
		_zerotime = zerotime();
		_offset = timestamp();
		_frequency = updatefrequency();
	}

	const uint64_t delta = timestamp() - _offset;
	// because the frequency is in update per seconds, we have to multiply the delta by 10,000,000 
	const uint64_t deltainus = delta * 10000000u / _frequency;
	return deltainus + _zerotime;
}

XAPI xtime_t XCALL xTimeGetMilliseconds()
{
	return GetTickCount();
}

XAPI xtime_t XCALL xTimeGetNanoseconds() 
{
	return now();
}

/*****************************************************************************/
