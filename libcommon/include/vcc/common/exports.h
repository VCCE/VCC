#pragma once

#ifdef LIBCOMMON_EXPORTS
#define LIBCOMMON_EXPORT __declspec(dllexport)
#else
#define LIBCOMMON_EXPORT __declspec(dllimport)
#endif
