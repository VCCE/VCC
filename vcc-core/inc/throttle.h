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

#ifndef _throttle_h_
#define _throttle_h_

/*********************************************************************************/

#include "xTimer.h"

/*********************************************************************************/

typedef struct throttle_t throttle_t;
struct throttle_t
{
	xtime_t			StartTime;
//	double			TargetTime;
	int 			FrameCount;
	double			fps;
//	xtime_t			fNow;
//	xtime_t			fLast;
	xtime_t			FPSTimer;
} ;

/*********************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	void	thrInit(throttle_t * pThrottle);

	void	thrStartRender(throttle_t * pThrottle);
	void	thrEndRender(throttle_t * pThrottle);
	void	thrFrameWait(throttle_t * pThrottle);

	double	thrCalculateFPS(throttle_t * pThrottle);

#ifdef __cplusplus
}
#endif
	
/*********************************************************************************/

#endif
