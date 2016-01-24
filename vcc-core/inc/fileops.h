#ifndef __FILEOPS_H__
#define __FILEOPS_H__
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

#include "xTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

	VCCCORE_API void			PathStripPath(char *);
	VCCCORE_API void			ValidatePath(char *Path);
	VCCCORE_API int				CheckPath(char *);
	VCCCORE_API bool_t			PathRemoveFileSpec(char *);
	VCCCORE_API bool_t			PathRemoveExtension(char *);
	VCCCORE_API char*			PathFindExtension(char *);

	/// TODO: move
	VCCCORE_API u32_t			WritePrivateProfileInt(const char * SectionName, const char * KeyName, int KeyValue, const char * IniFileName);

#ifdef __cplusplus
	}
#endif

#endif // __FILEOPS_H__
