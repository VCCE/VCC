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

/*********************************************************************************/
/*
	Frame rate limiter
 */
/*********************************************************************************/

#include "throttle.h"

#include "Vcc.h"

#include "xSystem.h"
#include "xDebug.h"

/*********************************************************************************/

void thrInit(throttle_t * pThrottle)
{
	assert(pThrottle != NULL);
	
    pThrottle->StartTime    = 0;
	//pThrottle->TargetTime	= 0;
	pThrottle->FrameCount	= 0;
	pThrottle->fps			= 0;
	//pThrottle->fNow			= 0;
	//pThrottle->fLast		= 0;
	
	//pThrottle->StartTime	= xTimeGetNanoseconds();
	//pThrottle->TargetTime	= (double)pThrottle->StartTime;

	pThrottle->FPSTimer	    = xTimeGetNanoseconds();
}

/*********************************************************************************/
/**
 */
void thrStartRender(throttle_t * pThrottle)
{
	assert(pThrottle != NULL);

    pThrottle->StartTime = xTimeGetNanoseconds();
}

/*********************************************************************************/
/**
 */
void thrEndRender(throttle_t * pThrottle)
{
	assert(pThrottle != NULL);
	
	pThrottle->FrameCount++;	
}

/*********************************************************************************/
/**
 */
void thrFrameWait(throttle_t * pThrottle)
{
    static double    targetFrameRate = 0;
    
    if ( targetFrameRate == 0 )
    {
        targetFrameRate = FRAMESPERSECOND;
    }
    
    // nanoseconds per frame
	const double	dNSPerFrame	= (1000000000.0 / targetFrameRate);
	
	assert(pThrottle != NULL);
	
	//
	// wait until the end of the this frame
	//
    xtime_t targetTime = pThrottle->StartTime + dNSPerFrame;
	while ( xTimeGetNanoseconds() < targetTime )
	{
		sysSleep(0);
	}
}

/*********************************************************************************/

double thrCalculateFPS(throttle_t * pThrottle)
{
	xtime_t		tmNow  = xTimeGetNanoseconds();
	
	assert(pThrottle != NULL);
	
    // update once per second
	if ( pThrottle->FPSTimer + 250 <= tmNow )
	{
		pThrottle->fps = pThrottle->FrameCount * (1000000000.0/(double)(tmNow-pThrottle->FPSTimer));
		
		pThrottle->FrameCount = 0;
		pThrottle->FPSTimer = xTimeGetNanoseconds();
	}
	
	return (double)pThrottle->fps;
}
		
/*********************************************************************************/
