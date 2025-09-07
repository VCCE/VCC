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

#include <Windows.h>
#include "throttle.h"
#include "audio.h"
#include "defines.h"
#include "Vcc.h"

static _LARGE_INTEGER StartTime,EndTime,OneFrame,CurrentTime,SleepRes,TargetTime,OneMs;
static _LARGE_INTEGER MasterClock,Now;
static unsigned char FrameSkip=0;
static float fMasterClock=0;

void CalibrateThrottle(void)
{
	timeBeginPeriod(1);	//Needed to get max resolution from the timer normally its 10Ms
	QueryPerformanceFrequency(&MasterClock);
	OneFrame.QuadPart = MasterClock.QuadPart / TARGETFRAMERATE;
	OneMs.QuadPart=MasterClock.QuadPart/1000;
	fMasterClock=(float)MasterClock.QuadPart;
}


void StartRender(void)
{
	QueryPerformanceCounter(&StartTime);
	return;
}

void EndRender(unsigned char Skip)
{
	FrameSkip=Skip;
	TargetTime.QuadPart=( StartTime.QuadPart+(OneFrame.QuadPart*FrameSkip));
	return;
}

void CheckSound()
{
	// Lean on the sound card a bit for timing
	if (GetSoundStatus())
	{
		PurgeAuxBuffer();
		if (FrameSkip == 1)
		{
			// Dont let the buffer get lest that half full
			if (GetFreeBlockCount() > AUDIOBUFFERS / 2)
				return;

			// Dont let it fill up either
			int count = 100; // loop limit
			while (GetFreeBlockCount() < 1 && count > 0)
			{
Sleep(1);
				--count;
			}
		}
	}
}

void FrameWait(void)
{
	//If we have more that 2Ms till the end of the frame
	QueryPerformanceCounter(&CurrentTime);
	while ( (TargetTime.QuadPart-CurrentTime.QuadPart)> (OneMs.QuadPart*2))	
	{
		Sleep(1);	//Give about 1Ms back to the system
		QueryPerformanceCounter(&CurrentTime);	//And check again
	}

	// Bug#281
	// moved to its own function so that final poll until frame end is always performed,
	// otherwise this sound check would exit early.
	CheckSound();

	//Poll Untill frame end.
	while ( CurrentTime.QuadPart< TargetTime.QuadPart)	
		QueryPerformanceCounter(&CurrentTime);

	return;
}

//Done at end of render;
float CalculateFPS(void) 
{
	const int frameUpdateRate = FRAMEINTERVAL;
	static unsigned int frameCount=0;
	static float fps=0;
	static _LARGE_INTEGER lastNow;

	if (++frameCount != frameUpdateRate)
		return fps;

	lastNow = Now;
	QueryPerformanceCounter(&Now);

	// interval between FrameInterval frames in milliseconds as long long
	auto intervalMS = (Now.QuadPart - lastNow.QuadPart) / OneMs.QuadPart;
	auto intervalSeconds = (float)intervalMS / 1000.0f;
	fps = (float)frameUpdateRate / intervalSeconds;

	frameCount = 0;
	return fps;
}
		