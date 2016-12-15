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

#include <windows.h>
#include "throttle.h"
#include "audio.h"
#include "defines.h"
#include "vcc.h"

static int64_t oneFrame;
static int64_t oneMs;
static int64_t startTime;
static int64_t currentTime;
static int64_t targetTime;
static uint8_t frameSkip = 0;
static float   fMasterClock=0;

/**
Get the current moment from the performance counters as a
64 bit signed int
*/
inline int64_t getCurrentPerfCount() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return now.QuadPart;
}

void CalibrateThrottle()
{
    //Needed to get max resolution from the timer normally its 10Ms
	timeBeginPeriod(1);	

    // Get the Master Clock resolution
    LARGE_INTEGER masterClock;
	QueryPerformanceFrequency(&masterClock); // Get CPS of Performance Clock
	
    // Compute duration of various constants based on Master Clock
    oneFrame = masterClock.QuadPart / TARGETFRAMERATE;
	oneMs = masterClock.QuadPart / 1000;
	fMasterClock = (float)masterClock.QuadPart;
}

void StartRender() 
{
    startTime = getCurrentPerfCount();
}

void EndRender(const uint8_t skip)
{
	frameSkip = skip;
    // Figure out when the frame should end
	targetTime = (startTime + (oneFrame * frameSkip) ); 
}

// TODO: eliminate busy waiting
void FrameWait()
{
    currentTime = getCurrentPerfCount();
	while ( (targetTime - currentTime) > (oneMs*2))	//If we have more that 2Ms till the end of the frame
	{
		Sleep(1);	//Give about 1Ms back to the system
        currentTime = getCurrentPerfCount(); //And check again
	}

	if (GetSoundStatus())	//Lean on the sound card a bit for timing
	{
		PurgeAuxBuffer();
		if (frameSkip == 1)
		{
			if (GetFreeBlockCount()>AUDIOBUFFERS/2)	return;	//Dont let the buffer get lest that half full				
			while (GetFreeBlockCount() < 1);	// Dont let it fill up either 
		}
	}

    while (currentTime < targetTime) //Poll Until frame end.
    {
        currentTime = getCurrentPerfCount();
    }
}

float CalculateFPS() //Done at end of render;
{
	static unsigned short FrameCount=0;
    static float fps = 0;
    static float fLast = 0;

    // Do not recalculate until enough frames have passed
	if (++FrameCount != FRAMEINTERVAL)
		return fps;

    // Get the current moment and extract the value   
	static float fNow = (float)getCurrentPerfCount();
	    
    fps = (fNow - fLast) / fMasterClock;
	fLast = fNow;
	FrameCount = 0;
	fps = FRAMEINTERVAL / fps;
	return fps;
}
		