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

#ifndef _cpuplugin_h_
#define _cpuplugin_h_

/*****************************************************************************/

#include "cpu.h"

/*****************************************************************************/

#define VCCPLUGIN_FUNCTION_CPUCREATE			"vcccpuModuleCreate"

/*****************************************************************************/

typedef cpu_t * (*cpucreatefn_t)(void);

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
    
    cpu_t *     cpuLoad(const char * pPathname);

#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif // _cpuplugin_h_

