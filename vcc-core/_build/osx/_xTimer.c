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

/*****************************************************************************/

#include "xTimer.h"

#include <mach/mach.h>
#include <mach/mach_time.h>

/*****************************************************************************/

#include "xTimer.h"

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <CoreServices/CoreServices.h>	// requires link to CoreService.framework

/*****************************************************************************/

static mach_timebase_info_data_t    sTimebaseInfo;

/*****************************************************************************/

XAPI xtime_t XCALL xTimeGetNanoseconds()
{
    uint64_t                            u64Nanoseconds;
    uint64_t                            t;
    
    // get abolute time
    t = mach_absolute_time();
    
    if ( sTimebaseInfo.denom == 0 ) {
        (void) mach_timebase_info(&sTimebaseInfo);
    }
    
    // Do the maths. We hope that the multiplication doesn't
    // overflow; the price you pay for working in fixed point.
    
    u64Nanoseconds = t * sTimebaseInfo.numer / sTimebaseInfo.denom;
    
    return u64Nanoseconds;
}

/*****************************************************************************/
/**
	return current milliseconds
 */
XAPI xtime_t XCALL xTimeGetMilliseconds()
{
    uint64_t		u64Milliseconds;
    uint64_t        t;
    
    // get abolute time
    t = mach_absolute_time();
    
    if ( sTimebaseInfo.denom == 0 ) {
        (void) mach_timebase_info(&sTimebaseInfo);
    }
    
    // Do the maths. We hope that the multiplication doesn't
    // overflow; the price you pay for working in fixed point.
    
    u64Milliseconds = (t * sTimebaseInfo.numer / sTimebaseInfo.denom) / (uint64_t)1000000;
    
    return u64Milliseconds;
}

/*****************************************************************************/

