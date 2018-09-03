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

/********************************************************************/
/*
 *  cpuplugin.c
 */
/********************************************************************/

#include "cpuplugin.h"

#include "xDynLib.h"
#include "xDebug.h"

/********************************************************************/

cpu_t * cpuLoad(const char * pPathname)
{
    pxdynlib_t          hModule;
    result_t            errResult;
    cpucreatefn_t       pfnCPUCreate;
    cpu_t *             pCPU            = NULL;
    
    assert(pPathname != NULL);
    
    errResult = xDynLibLoad(pPathname,&hModule);
    if (    errResult == XERROR_NONE
        || hModule != NULL
        )
    {
        errResult = xDynLibGetSymbolAddress(hModule, VCCPLUGIN_FUNCTION_CPUCREATE, (void **)&pfnCPUCreate);
        if (    errResult == XERROR_NONE
            && pfnCPUCreate != NULL
            )
        {
            // create custom CPU
            pCPU = (*pfnCPUCreate)();
        }
    }
    
    return pCPU;
}

/********************************************************************/
